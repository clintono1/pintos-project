Design Document for Project 3: File System
==========================================

## Group Members

* Aaron Xu <aaxu@berkeley.edu>
* Yena Kim <yenakim@berkeley.edu>
* Aidan Meredith <aidan_meredith@berkeley.edu>
* Louise Feng <louise.feng@berkeley.edu>


# Part 1: Buffer Cache

## Data structures and functions

```
struct cache_entry {
	bool dirty; // Used for write-back
	bool valid // Check if this entry is valid
	int reference; // Used for clock algorithm
	block_sector_t sector; // Sector number of the block below.
	void *block; // Should be malloc'd to be 512 bytes
}
```

`int clock_hand; // The clock hand of the clock algorithm`

## Algorithms

For existing calls of `block_read` in `inode_read_at` and `inode_write_at`,
we will first load it into the cache as a new cache entry. This will take the
place of both the existing bounce buffer and the temporary buffer so that we can
also load part of a block into the caller's buffer. For existing calls of
`block_write` in `inode_write_at`, we will remove these calls and either write
directly to the cache block if it is already in the cache, or create a new entry
if it does not exist.

To add and get a sector to and from the cache, we will define a function
`cache_get_block` which takes in a sector number and a buffer that can contain
512 bytes and checks if the sector is in the cache, adds it as a new cache
entry if it isn't, and then copies the block's data into the passed in buffer.
If the sector is not in the cache, it will need to call `block_read` and put that in
the new cache entry.

To write to a sector, we will define a function `cache_write_block` which takes
in a sector number and a buffer containing 512 bytes of data to write to. If
the sector is not in the cache, we load it into the cache. Then we copy the data
over into the cache entry's data block. Since this takes in a buffer that contains
all data that will be written to the block, we will need to do what is done
in the skeleton code through the bounce buffer. Since we need to read the cache
before we write to it and prevent other processes from accessing the same
socket, we decided to disable interrupts once we determine what sector number
we need to access. We will reenable interrupts once we reach the end
of the while loop, specifically after we finish all the reading and writing
for that sector. We do this in both `inode_write_at` and `inode_read_at`.
```
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

      old_level = intr_disable ();
      ...
      ... // Do read/writes to cache

  	  intr_set_level (old_level);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}
```

```
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

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

      old_level = intr_disable ();
      ...
      ... // Do read/writes to cache

  	  intr_set_level (old_level);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}
```
we will use the clock algorithm for our cache replacement policy.

## Synchronization
For synchronization, we want to make it so that when two processes check if
a sector is in a cache, there are no race conditions. We do disabling interrupts
at the beginning of the `cache_get_block` function and will be reset it at the
end of the function. Then, we also want to make it so that no two sectors
can be modified at the same time. We can do this by disabling interrupts
for the `cache_get_block` and `cache_write_block` functions, and since they modify one
sector at a time, this will make modifying a sector atomic and thus no race
conditions will occur. Since cache entries are only evicted during these two
function calls and they have interrupts disabled, an entry cannot be
evicted while another file is actively reading or writing to it. Similarly,
no other processes can also access the block as we are evicting a block and
no other processes can also access the cache or insert new entries during
this function call.

## Rationale
We considered using multiple levels of locks to lock the cache to allow concurrent writes to different
sectors, but we found that it wasn't feasible. We had to make operations on any
one cache entry atomic because if not, the cache entry could be evicted before we
read/write but after we check that the entry is in the cache. Thus, we found that
we always need to check if a sector is in the cache and then read or write from
it atomically. By disabling interrupts once we determine what sector we want,
this prevents any other processes from accessing the cache before the current operation
finishes. This ensures that it will read the right data and that an entry
will never be evicted while a process is actively using it.

# Part 2: Extensible Files

## Data structures and functions

To allow for extending files, we keep the `inode_disk` struct as is, but we change `start` to be a doubly indirect block pointer to allow for more data to be stored per file.
```
struct inode_disk
  {
  	block_sector_t start;				/* Doubly indirect pointer. */
  }
```

When a write is made off the end of the file, the following function is called to resize the file.
```
#include <math.h>

...

block_sector_t
inode_add_block (inode_disk *id, off_t size)
{
  uint32_t indirect_pointer_ind = (uint32_t) (size / powf (2, 16)); // index of the indirect pointer in the doubly indirect block
  uint32_t sector_ind = (uint32_t) (size - indirect_pointer_ind * powf (2, 16) / BLOCK_SECTOR_SIZE); // index of the sector in the indirect block, should not yet be allocated

  void *doubly_indirect_block = cache_get_block (id->start);
  block_sector_t indirect_block_sector = doubly_indirect_block[indirect_pointer_ind];
  void *indirect_block = cache_get_block (indirect_block_sector);

  block_sector_t *buffer[128];
  memset (buffer, 0, 512);
  cache_write_block (buffer, indirect_block[sector_ind]);

  if (!free_map_allocate (1, &indirect_block[sector_ind]))
    return 0;

  return indirect_block[sector_ind];
}
```
To write past the end of a file, when we write we need to check if we're writing past the end. If we are, we call `inode_resize` to resize the file to a length extending to the position we finish the write at. We do this in `inode_write_at`

```
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  ...

  off_t bytes_written = 0;

  ...

  off_t length = inode_length (inode);
  off_t inode_left = length - offset;

  if (inode_left < 0) // past end of file
    {
      offset -= length;
      if (offset < 512 - (length % 512)) //still within the same block
        {
          uint32_t indirect_pointer_ind = (uint32_t) (length + offset / powf (2, 16)); // index of the indirect pointer in the doubly indirect block
          uint32_t sector_ind = (uint32_t) (length + offset - indirect_pointer_ind * powf (2, 16) / BLOCK_SECTOR_SIZE); // index of the sector in the indirect block

          void *doubly_indirect_block = cache_get_block (inode->start);
          block_sector_t indirect_block_sector = doubly_indirect_block[indirect_pointer_ind];
          void *indirect_block = cache_get_block (indirect_block_sector);
          void *block = cache_get_block (indirect_block[sector_ind]);
          if (size < 512 - offset + (length % 512))                    //if we only need to write to the one block
            memcpy (block[offset + (length % 512)], buffer, size);
            cache_write_block (block, block_num);
            return size;
          else            //if we need more than the current block
            {
              memcpy (block + offset + (length % 512), buffer, 512 - offset + (length % 512));
              size -= 512 - offset + (length % 512);
              bytes_written += 512 - offset + (length % 512);
              int buffer_offset = 512 - offset + (length % 512);
              while (size > 512)                      // keep grabbing ech new block we'll need
                {
                  block_sector_t block_num = inode_add_block (inode->data);
                  if (block_sector_t == 0)           //if we run out of space
                    return bytes_written;
                  void *block = cache_get_block (block_num);
                  memcpy (block, buffer + buffer_offset, 512);
                  cache_write_block (block, block_num);
                  size -= 512;
                  bytes_written += 512;
                  buffer_offset += 512;
                }
              block_sector_t block_num = inode_add_block (inode->data);
              if (block_sector_t == 0)
                return bytes_written;
              void *block = cache_get_block (block_num);
              memcpy (block, buffer + buffer_offset, size);
              cache_write_block (block, block_num);
              bytes_written += size;
            }
        }
      else  //if we start at a block later than the block of the EOF
        {
          offset -= 512 - (length % 512);
          while (offset > 512)                  //find the block
            {
              block_sector_t block_num = inode_add_block (inode->data);
              if (block_sector_t == 0)
                return bytes_written;
              offset -= 512;
            }
          int buffer_offset = 0;
          block_sector_t block_num = inode_add_block (inode->data);
          if (block_sector_t == 0)
            return bytes_written;
          void *block = cache_get_block (block_num);
          memcpy (block + offset, buffer + buffer_offset, size);
          cache_write_block (block, block_num);
          bytes_written += size;
          while (size > 512) //if we need to keep writing into other blocks
            {
              block_sector_t block_num = inode_add_block (inode->data);
              if (block_sector_t == 0)
                return bytes_written;
              void *block = cache_get_block (block_num);
              memcpy (block, buffer + buffer_offset, 512);
              cache_write_block (block, block_num);
              size -= 512;
              bytes_written += 512;
              buffer_offset += 512;
            }
          block_sector_t block_num = inode_add_block (inode->data);
          if (block_sector_t == 0)
            return bytes_written;
          void *block = cache_get_block (block_num);
          memcpy (block, buffer + buffer_offset, size);
          cache_write_block (block, block_num);
          bytes_written += size;
        }
    }

  ...

  return bytes_written;
}
```

To implement the syscall `inumber` we add the following `else if` case to `syscall_handler`.
```
static void
syscall_handler (struct intr_frame *f UNUSED)
{
  ...
  else if (args[0] == SYS_INUMBER)
    {
      int fd = args[1];
      struct file *file = get_file (fd);
      f->eax = file->inode->sector;
    }
  ...
}
```

## Algorithms

When the user writes past the end of the file, we call `inode_add_block` in order to extend the file. We do this by extending the data that the `inode_disk` struct can access by changing one of its attributes to be a doubly-indirect pointer. This allows the inode to have 2^23 bytes of data (1 doubly-indirect pointer * 2^7 indirect pointers * 2^7 direct pointers * 2^9 `BLOCK_SECTOR_SIZE`). `inode_add_block` figures out where the last block of allocated data is and allocates a new block after it. We will continuously allocate blocks in `inode_write_at` until we have reached the size we require. If we have run out of free blocks and `free_map_allocate` returns `false`, then we just stop trying to allocate blocks and return the number of bytes we have managed to write to data so far. We read and write to the blocks by using the cache functions `cache_get_block` and `cache_write_block`, which check to see if the desired block is in the cache. If it isn't it'll add it to the cache. These functions are more thoroughly explained in part 1. The cases we account for are the case where we still only need to write to the same block as the last block, when we need to write to the last block as well as allocating more blocks, and when we start writing past the current block and need to allocate more blocks to get to where we start writing. Once we have the block we want to be in, we `memcpy` from the buffer and write to the cache.

For the syscall `inumber`, we get the file associated with the given file descriptor using the current thread's `fd_table` and the method `get_file`. The returned `struct file` then has an inode that contains the unique sector it occupies, which is its inode number.

## Synchronization

When we write to a file, there is a chance that we will need to change the size of the file to make sure everything we have written to the file is within the space it extends. In this case, we shouldn't have 2 processes simultaneously trying to call `inode_add_block` and possibly have the one that finishes later delete anything the first process tried to write. This is being dealt with by the cache in part 1.

## Rationale

We add a doubly-indirect block to `inode_disk` because it allows us to access 2^14 data blocks and also perfectly utilizes the the `BLOCK_SECTOR_SIZE` long space available in the struct. In resizing the inode, we decided to keep the zeros at the end of the file between the EOF and the offset passed in `inode_write_at` explicit for ease of understanding and simplicity

# Part 3: Subdirectories

## Data structures and functions

We will add the following syscalls: chdir, mkdir, readdir, and isdir
adding them to the if check in syscall.c syscall_handler():
syscall.c
```
	...
	} else if (args[0] == SYS_CHDIR) {

	} else if (args[0] == SYS_MKDIR) {

	} else if (args[0] == SYS_READDIR) {

	} else if (args[0] == SYS_ISDIR) {

	} else if (args[0] == SYS_INUMBER) {
```


In directory.c dir_add(), need to resize directory...

We will add a cwd field to the thread struct.

```
struct thread {
	...
	char cwd[14]; // Current working directory of the thread.
}
```

This will be passed down from the parent in `process_execute` into the `*aux`
parameter in `thread_create`. If there is no parent (ie. the first process), the
directory will be set to '/' to represent the root directory. We currently pass
in an array into the `*aux` parameter, so we can add a new element which contains
the parent's cwd. We must make sure that we create a copy of this string so that
if this is modified, we do not modify the parent's current working directory.

In order to tell whether an inode is for a file or a directory, we will
add a boolean to the inode_disk struct.

```
struct inode_disk {
	...
	bool is_dir;
	...
}
```

We have to change the fd table to support opening directories as well. We would
originally change the fd array that we use from a `struct file*` array to a
`struct file_or_dir *` array where this new struct is defined as:
```
struct file_or_dir {
	struct file* file;
	struct dir* dir;
}
```

Thus, we could simply check whichever is not null and that would be the type
of object that is stored in the fd table. However, since we are using staff
solutions, we think that in the `file_descriptor` struct, we would simply need
to add a `struct dir*` element and that would accomplish that same thing.

```
struct file_descriptor
  {
    struct list_elem elem;      /* List element. */
    struct file *file;          /* File. */
    struct dir *dir;			/* Directory */
    int handle;                 /* File handle. */
  };
```

## Algorithms

For the syscalls that take in a file name, (open, remove, create, exec, chdir,
mkdir), we define a new function that will turn the given path and return the
absolute path. We will use the returned path instead of whatever path was given,
so that we only need to deal with absolute paths in the rest of the code.

```
char *
path_to_absolute(const char *path)
{
	/* Returns a string that represents the absolute path of the given path.
	   String must be freed after use. */
	if (*path == '/') {
		// Path is already absolute
		char *absolute = malloc(strlen(path) + 1);
		strcpy(absolute, path);
		return absolute;
	}
	char *cpy_path;
	strcpy(cpy_path, path)
	int absolute_length = strlen(path) + strlen(thread_current()->cwd);
	char *absolute = malloc(absolute_length + 1);

	char *part;
	int next_path = get_next_part(part, &cpy_path);
	char *iter = absolute;
	memcpy(iter, thread_current()->cwd, strlen(thread_current()->cwd));
	iter += strlen(thread_current()->cwd);
	while (next_path) {
		if (next_path == -1) {
			printf("File name was too long");
			return NULL;
		} else {
			if (strcmp(path, "..") == 0) {
				// Remove last part of current path.
				// If no last part (ie. we are at root, do nothing).
			} else if (strcmp(path, ".") == 0) {
				// Do nothing
			} else {
				memcpy(iter, path, strlen(path));
				iter += strlen(path);
				*iter = '/'
				iter++;
			}
		}
		next_path = get_next_part(part, &cpy_path);
	}
	*--iter = '\0' // Null terminate path and remove trailing slash.
	return absolute;
}

```

We also define a function to get the last directory object of a absolute path.

```
struct dir *
get_last_directory(const char *path) {
	char *part;
	char *absolute = path;
	strcpy(absolute, path);
	struct dir *dir = dir_open_root();
	struct dir *prev = dir;
	int next_part = get_next_part(part, &absolute);
	struct inode *inode;
	while (next_part) {
		if (next_part == -1) {
			printf("File name too long");
			return NULL;
		} else {
			if (dir_lookup(dir, part, &inode)) {
				next_part = get_next_part(part, &absolute);
				dir_close(prev);
				prev = dir;
				dir = dir_open(inode);
			} else {
				return NULL;
			}
		}
	}
	dir_close(dir);
	return prev;
}

```


We must also change how the open syscall is handled in the syscall handler.
We can do this by getting the absolute path of the requested file, getting the
last directory, and then calling dir_lookup on the actual file's name. If there are
any failures in looking up the part, then we can return -1. Otherwise, we will
check if the final inode returned is a directory or a file by checking its
inode_disk's `is_dir` element. After, we can add this to the fd table and return
the corresponding fd to the user program.

In order to remove a directory, we will define a function `dir_is_empty`

```
bool
dir_is_empty (struct dir *dir)
{
	char tmp[15];
	counter = 0;
	struct dir *directory = dir_reopen(dir);
	while(dir_readdir(directory, tmp)) {
		counter++;
	}
	return counter == 2; // Directory only has . and ..
}

```

To actually remove a directory, we will search for the specified file by using
converting the given path to absolute path. Then, we remove it as if it is a file.
We can use `get_last_directory` and checking if the file exists and then remove it
by calling `dir_remove` with this directory.
If the file is a directory, we make sure that it is first empty through the
`dir_is_empty` function and if it returns true, then we remove it with
`dir_remove`. This should update the remove syscall so that it supports directories.

For the chdir syscall, we can get the absolute path by using the `path_to_absolute`
function we defined and setting the thread's cwd value to this new absolute path.
Then we have to make sure that this is a valid by appending some dummy file name
to the end of the path and calling `get_last_directory` on that. For example,
if switch to directory "/foo/bar", we would pass "/foo/bar/fakeName" and check that
`get_last_directory` does not return NULL (and we must close the returned directory
object). If this is true, then we can change the thread's cwd value to this new
path. Return 1 if successful to the user program, else 0.

For the mkdir syscall, we can do the same thing as the chdir syscall. However,
instead of closing the returned directory object immediately, we try to call
`dir_add` with an unallocated free block.

For the readdir syscall, we can extract the directory object from the `file_descriptor`
struct in the fd table. Then, we can call dir_readdir with the passed in buffer.
However, we must have a check so that "." and ".." are not returned.

For the isdir syscall, we can check the `file_descriptor` struct corresponding
to the passed in file descriptor. If the file struct is not NULL, then we know
that that descriptor corresponds to a file. Otherwise, it is a directory.

For the inumber syscall, we can extract either the file or directory struct from
the file descriptor. From there, we can grab the inode in that struct and call
`inode_get_inumber` to return the corresponding inumber.

Because we handled inode expansion in part 2, this should also apply to directory expansion because all their data is stored in their respective inodes. When we add to a directory by calling `dir_add`, we are able to expand the directory because this function calls `inode_write_at`, where we implemented inode expansion.

## Synchronization

Our implementation of directory deletion allows directories to be deleted even if it is open by a process or is the current working directory of some process. Due to this, we need to ensure that when we delete a direcoty, we should no longer be able to open files or create new files in the deleted directory. In this case we always convert the path to an absolute path and start at the root and follow the path until the correct directory is found and then delete it so that that directory is no longer ever accessible. We will call `dir_remove` which will delete the inode associated with the directory. Therefore when another process tries to access the directory, it will see that the inode does not exist and will not be able to open it. This way we can prevent a process from trying to open or do anything to a directory that is deleted.



## Rationale
We handle relative paths by always converting to an absolute path. By storing
the current working directory in the thread's struct, we can concatenate the
current working directory with the relative path. If we encounter a "..", then
we update the absolute path by removing the last part of our current path. The
user process will be allowed to delete a directory if it is the current
working directory of a runningprocess because we only store that as a string
in the thread struct. Since our implementation makes us concatenate strings,
get the absolute path, and then find the file starting from root, we should run
into no issues with deleting the directory. Finally, given a file descriptor,
we can find the corresponding file struct or directory struct by accessing
the `file_descriptor` struct which will either contain a valid `struct file` or
`struct dir` pointer. From there, we can get the corresponding inode and then
get access to all of its blocks through its doubly indirect pointer.


# Additional Question
For this project, there are 2 optional buffer cache features that you can implement: write-behind
and read-ahead. A buffer cache with write-behind will periodically flush dirty blocks to the filesystem
block device, so that if a power outage occurs, the system will not lose as much data. Without
write-behind, a write-back cache only needs to write data to disk when (1) the data is dirty and
gets evicted from the cache, or (2) the system shuts down. A cache with read-ahead will predict
which block the system will need next and fetch it in the background. A read-ahead cache can
greatly improve the performance of sequential file reads and other easily-predictable file access patterns.
Please discuss a possible implementation strategy for write-behind and a strategy for
read-ahead.

To implement a write-behind cache, we first need to mark data that has been modified. When we first load or modify existing data in the cache, we mark it with a dirty bit. This denotes that the data is new and should be written to disk at the next opportunity. To offload the contents of the cache at regular intervals, we invoke system interrupts. During the interrupt, we move blocks that are marked with a dirty bit to disk.

In order to implement a read-ahead cache, we would have a heap that stores filenames and the amount of times they have been accessed from disk. When we aren't currently loading from memory, we grab the block that corresponds to the most frequently accessed filename, and load it into main memory.
