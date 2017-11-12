Design Document for Project 3: File System
==========================================

## Group Members

* Aaron Xu <aaxu@berkeley.edu>
* Yena Kim <yenakim@berkeley.edu>
* Aidan Meredith <aidan_meredith@berkeley.edu>
* Louise Feng <louise.feng@berkeley.edu>


# Part 1: Buffer Cache

## Data structures and functions



## Algorithms




## Synchronization


## Rationale

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

bool
inode_resize (inode_disk *id, off_t size)
{
  block_sector_t sector;
  block_sector_t buffer[128];
  if (id->start == 0)
  	{
  	  memset (buffer, 0, 512);
  	  if (!free_map_allocate (1, sector))
  	  	{
  	  	  inode_resize (id, id->length);
  	  	  return false;
  	  	}
  	  id->start = sector;
  	}
  else
  	{
  	  block_read (fs_device, sector, buffer);
  	}
  for (int i = 0; i < 128; i++)
  	{
  	  block_sector_t buffer2[128];
  	  if (id->start[i] == 0)
  	    {
  	      memset (buffer2, 0, 512);
  	      if (!free_map_allocate (1, sector))
  	      	{
  	      	  inode_resize (id, id->length);
  	      	  return false;
  	      	}
  	      id->start[i] = sector;
  	    }
  	  else
  	  	{
  	  	  block_read (fs_device, id->start[i], buffer2);
  	  	}
  	  for (int j = 0; j < 128; j++)
  	  	{
  	  	  if (size <= (i * powf (2, 16)) + (j * BLOCK_SECTOR_SIZE) && buffer2[j] != 0)
  	  	  	{
  	  	  	  free_map_release (buffer2[j], 1);
  	  	  	  buffer2[j] = 0;
  	  	  	}
  	  	  if (size > (i * powf (2, 16)) + (j * BLOCK_SECTOR_SIZE) && buffer2[j] == 0)
  	  	  	{
  	  	  	  if (!free_map_allocate (1, sector))
  	  	  	  	{
  	  	  	  	  inode_resize (id, id->length);
  	  	  	  	  return false;
  	  	  	  	}
  	  	  	  buffer2[j] = sector;
  	  	  	}
  	  	}
  	  if (size <= i * powf (2, 16))
  	  	{
  	  	  free_map_release (id->start[i], 1);
  	  	  id->start[i] = 0;
  	  	}
  	  else
  	  	{
  	  	  block_write (fs_device, id->start[i], buffer2);
  	  	}
  	}
  id->length = size;
  return true;
}
```
To write past the end of a file, when we write we need to check if we're writing past the end. If we are, we call `inode resize` to resize the file to a length extending to the position we finish the write at. We do this in `inode_write_at` 

```
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
   ...
   off_t inode_left = inode_length (inode) - offset;

   if (inode_left < size)
   	 {
   	 	inode_resize(inode->data, size);
   	 }

   ...
}
```

## Algorithms

When the user writes past the end of the file, we call `inode_resize` in order to extend the file. We do this by extending the data that the `inode_disk` struct can access by changing one of its attributes to be a doubly-indirect pointer. This allows the inode to have 2^23 bytes of data (1 doubly-indirect pointer * 2^7 indirect pointers * 2^7 direct pointers * 2^9 BLOCK_SECTOR_SIZE). `inode_resize` goes through all the pointers accessible by the `start` doubly-indirect pointer. It checks that each indirect pointer is allocated or not. If it is, it is stored in a buffer, otherwise it is set to 0. Then, it iterates through all the direct pointers from that indirect pointer and frees those data blocks if it doesn't fit within the new size. Otherwise, it allocates a new block and puts it into another buffer. After iterating through all the direct pointers of an indirect pointer, it checks to see if the new size includes that indirect pointer. If it does, it will write the buffer into the indirect pointer block. Otherwise, we set the indirect pointer to 0, indicating that it is unallocated. Finally, we set the new size to be the length of the inode.


## Synchronization


## Rationale

We add a doubly-indirect block to `inode_disk` because it allows us to access 2^14 data blocks and also perfectly utilizes the thr `BLOCK_SECTOR_SIZE` long space available in the struct. In resizing the inode, we decided to keep the zeros at the end of the file explicit for ease of understanding and simpliity.


# Part 3: Subdirectories

## Data structures and functions



## Algorithms




## Synchronization


## Rationale

