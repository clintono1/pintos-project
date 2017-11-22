#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

int get_next_cache_block_to_evict (void);
void cache_write_block (block_sector_t sector, void *buffer);
void cache_get_block (block_sector_t sector, void *buffer);
/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start[100];          /* Direct pointers to sectors. */
    block_sector_t doubly_indirect;     /* Doubly indirect pointer. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[25];                /* Not used. */
  };

// bool inode_resize (struct inode_disk id, off_t size);

struct cache_entry
  {
    bool dirty; // Used for write-back
    bool valid; // Check if this entry is valid
    int reference; // Used for clock algorithm
    block_sector_t sector; // Sector number of the block below.
    struct semaphore sector_lock; // Lock the sector to prevent concurrent read/writes.
    void *block; // Should be malloc'd to be BLOCK_SECTOR_SIZE bytes
  };

int clock_hand; // Default initialized to 0. Used for the clock algorithm.
struct cache_entry *cache[64]; // Cache sector blocks.
struct lock cache_lock; // Lock actions that check the cache.

/* Uses the clock algorithm and returns the index of the cache block that
   should be evicted. This function is NOT thread-safe. You must acquire
   the cache_lock before calling this. */

/* Write all cache entries to disk */
void
flush_cache (void)
{
  int i;
  for (i = 0; i < 64; i++)
    {
      if (cache[i] && cache[i]->valid && cache[i]->dirty)
        block_write (fs_device, cache[i]->sector, cache[i]->block);
    }
}


int
get_next_cache_block_to_evict (void)
{
  // Might need to account for the case where all blocks are locked
  // Currently skips over blocks that are locked. Uses the clock algorithm.
  clock_hand++;
  while (1)
    {
      if (clock_hand == 64)
        clock_hand = 0;
      if (!cache[clock_hand])
        return clock_hand;
      if (cache[clock_hand]->reference && cache[clock_hand]->sector_lock.value)
        cache[clock_hand]->reference = 0;
      else
        return clock_hand;
      clock_hand++;
    }
}

void
cache_get_block (block_sector_t sector, void *buffer)
{
  int i;
  lock_acquire (&cache_lock);
  // Check if sector is already in the cache and return if it is.
  for (i = 0; i < 64; i++)
    {
      if (cache[i] && cache[i]->sector == sector)
        {
          sema_down (&cache[i]->sector_lock);
          cache[i]->reference = 1;
          lock_release (&cache_lock);
          memcpy (buffer, cache[i]->block, BLOCK_SECTOR_SIZE);
          sema_up (&cache[i]->sector_lock);
          return;
        }
    }
  int index = get_next_cache_block_to_evict ();
  void *block;
  struct cache_entry *entry;
  if (cache[index])
    {
      sema_down (&cache[index]->sector_lock);
      if (cache[index]->dirty)
        block_write (fs_device, cache[index]->sector, cache[index]->block);
      entry = cache[index];
      block = cache[index]->block;
    }
  else
    {
      entry = malloc(sizeof(struct cache_entry));
      block = malloc(BLOCK_SECTOR_SIZE);
      sema_init (&entry->sector_lock, 0);
      cache[index] = entry;
      entry->block = block;
    }
  entry->dirty = 0;
  entry->valid = 1;
  entry->reference = 1;
  entry->sector = sector;
  lock_release (&cache_lock);
  block_read (fs_device, sector, block);
  sema_up (&entry->sector_lock);
  memcpy(buffer, block, BLOCK_SECTOR_SIZE);
}

void
cache_write_block (block_sector_t sector, void *buffer)
{
  int i;
  lock_acquire (&cache_lock);
  // Check if sector is already in the cache and return if it is.
  for (i = 0; i < 64; i++)
    {
      if (cache[i] && cache[i]->sector == sector)
        {
          sema_down (&cache[i]->sector_lock);
          cache[i]->reference = 1;
          lock_release (&cache_lock);
          memcpy (cache[i]->block, buffer, BLOCK_SECTOR_SIZE);
          cache[i]->dirty = 1;
          sema_up (&cache[i]->sector_lock);
          return;
        }
    }
  int index = get_next_cache_block_to_evict ();
  void *block;
  struct cache_entry *entry;
  if (cache[index])
    {
      sema_down (&cache[index]->sector_lock);
      if (cache[index]->dirty)
        block_write (fs_device, cache[index]->sector, cache[index]->block);
      entry = cache[index];
      block = cache[index]->block;
    }
  else
    {
      entry = malloc(sizeof(struct cache_entry));
      block = malloc(BLOCK_SECTOR_SIZE);
      sema_init (&entry->sector_lock, 0);
      cache[index] = entry;
      entry->block = block;
    }
  entry->dirty = 1;
  entry->valid = 1;
  entry->reference = 1;
  entry->sector = sector;
  lock_release (&cache_lock);
  block_read (fs_device, sector, block);
  memcpy(block, buffer, BLOCK_SECTOR_SIZE);
  sema_up (&entry->sector_lock);
}


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  block_sector_t ret_val;
  sema_down (&inode->inode_lock);
  if (pos < inode->data.length)
    ret_val = inode->data.start + pos / BLOCK_SECTOR_SIZE;
  else
    ret_val = -1;
  sema_up (&inode->inode_lock);
  return ret_val;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;
struct lock *inode_list_lock;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
  lock_init (&cache_lock);
  lock_init (&inode_list_lock);
  clock_hand = -1;
}

void
lock_inode_list (void)
{
  lock_acquire (&inode_list_lock);
}

void
release_inode_list (void)
{
  lock_release (&inode_list_lock);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (free_map_allocate (sectors, &disk_inode->start))
        {
          cache_write_block (sector, disk_inode);
          if (sectors > 0)
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;

              for (i = 0; i < sectors; i++)
                cache_write_block (disk_inode->start + i, zeros);
            }
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  lock_inode_list ();
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      sema_down (&inode->inode_lock);
      if (inode->sector == sector)
        {
          inode->open_cnt++;
          sema_up (&inode->inode_lock);
          release_inode_list ();
          return inode;
        }
      sema_up (&inode->inode_lock);
    }
  release_inode_list ();

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  sema_init (&inode->inode_lock, 0);
  lock_inode_list ();
  list_push_front (&open_inodes, &inode->elem);
  release_inode_list ();
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  sema_up (&inode->inode_lock);
  cache_get_block (inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    {
      sema_down (&inode->inode_lock);
      inode->open_cnt++;
      sema_up (&inode->inode_lock);
    }
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  sema_down (&inode->inode_lock);
  block_sector_t sector = inode->sector;
  sema_up (&inode->inode_lock);
  return sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  sema_down (&inode->inode_lock);
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      lock_inode_list ();
      list_remove (&inode->elem);
      release_inode_list ();

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          free_map_release (inode->data.start,
                            bytes_to_sectors (inode->data.length));
        }
      sema_up (&inode->inode_lock);
      free (inode);
      return;
    }
  sema_up (&inode->inode_lock);
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  sema_down (&inode->inode_lock);
  inode->removed = true;
  sema_up (&inode->inode_lock);
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          cache_get_block (sector_idx, (void *) (buffer + bytes_read));
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          cache_get_block (sector_idx, (void *) bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  sema_down (&inode->inode_lock);
  if (inode->deny_write_cnt)
    {
      sema_up (&inode->inode_lock);
      return 0;
    }
  sema_up (&inode->inode_lock);

  // if (!inode_resize (inode->data, size))
  //   return 0;

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          cache_write_block (sector_idx, (void *) (buffer + bytes_written));
        }
      else
        {
          /* We need a bounce buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left)
            cache_get_block (sector_idx, (void *) bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          cache_write_block (sector_idx, (void *) bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Resizes INODE to be of SIZE size. */
// bool
// inode_resize (struct inode_disk id, off_t size)
// {
//   block_sector_t sector;
//   int i;
//   for (i = 0; i < 100; i++)
//   {
//     if (size <= BLOCK_SECTOR_SIZE * i && id.start[i] != 0)
//       {
//         free_map_release (id.start[i], 1);
//         id.start[i] = 0;
//       }
//     if (size > BLOCK_SECTOR_SIZE * i && id.start[i] == 0)
//       {
//         if (!free_map_allocate (1, sector))
//           {
//             inode_resize (id, id.length);
//             return false;
//           }
//         id.start[i] = sector;
//       }
//   }

//   block_sector_t buffer[128];
//   if (id.doubly_indirect == 0)
//     {
//       memset (buffer, 0, 512);
//       if (!free_map_allocate (1, sector))
//         {
//           inode_resize (id, id.length);
//           return false;
//         }
//       id.doubly_indirect = sector;
//     }
//   else
//     {
//       cache_get_block (id.doubly_indirect, buffer);
//     }
//   for (i = 0; i < 128; i++)
//     {
//       block_sector_t buffer2[128];
//       if (buffer[i] == 0)
//         {
//           memset (buffer2, 0, 512);
//           if (!free_map_allocate (1, sector))
//             {
//               inode_resize (id, id.length);
//               return false;
//             }
//           buffer[i] = sector;
//         }
//       else
//         {
//           cache_get_block (buffer[i], buffer2);
//         }
//       int j;
//       for (j = 0; j < 128; j++)
//         {
//           if (size <= 512 * 100 + (i * (int) powf (2, 16)) + (j * BLOCK_SECTOR_SIZE) && buffer2[j] != 0)
//             {
//               free_map_release (buffer2[j], 1);
//               buffer2[j] = 0;
//             }
//           if (size > 512 * 100 + (i * (int) powf (2, 16)) + (j * BLOCK_SECTOR_SIZE) && buffer2[j] == 0)
//             {
//               if (!free_map_allocate (1, sector))
//                 {
//                   inode_resize (id, id.length);
//                   return false;
//                 }
//               buffer2[j] = sector;
//             }
//         }
//       if (size <= 512 * 100 + i * (int) powf (2, 16))
//         {
//           free_map_release (buffer[i], 1);
//           buffer[i] = 0;
//         }
//       else
//         {
//           cache_write_block (buffer[i], buffer2);
//         }
//     }
//   cache_write_block (id.doubly_indirect, buffer);
//   id.length = size;
//   return true;
// }

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  sema_down (&inode->inode_lock);
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  sema_up (&inode->inode_lock);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  sema_down (&inode->inode_lock);
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
  sema_up (&inode->inode_lock);
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  sema_down (&inode->inode_lock);
  int length = inode->data.length;
  sema_up (&inode->inode_lock);
  return length;;
}
