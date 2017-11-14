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

To write to a sector, we will define a funciton `cache_write_block` which takes
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

## Synchronization
For synchronization, we want to make it so that when two processes check if
a sector is in a cache, there are no race conditions. We do disabling interrupts
at the beginning of the `cache_get_block` function and will be reset it at the
end of the function. Then, we also want to make it so that no two sectors
can be modified at the same time. We can do this by disabling interrupts
for the `cache_get_block` and `cache_write_block` functions, and since they modify one
sector at a time, this will make modifying a sector atomic and thus no race
conditions will occur. Since cache entires are only evicted during these two
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
To write past the end of a file, when we write we need to check if we're writing past the end. If we are, we call `inode_resize` to resize the file to a length extending to the position we finish the write at. We do this in `inode_write_at`

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

When we write to a file, there is a chance that we will need to change the size of the file to make sure everything we have written to the file is within the space it extends. In this case, we shouldn't have 2 processes simultaneously trying to call `inode_resize` and possibly have the one that finishes later delete anything the first process tried to write. We therefore want to lock the Write syscall to ensure that the same file is not being accessed by more than 1 program at one time. However, we do want to allow 2 operations acting on different disk sectors, different files, and different directories to run simultaneously.

## Rationale

We add a doubly-indirect block to `inode_disk` because it allows us to access 2^14 data blocks and also perfectly utilizes the the `BLOCK_SECTOR_SIZE` long space available in the struct. In resizing the inode, we decided to keep the zeros at the end of the file explicit for ease of understanding and simpliity. We also use 2 buffers - `buffer` and `buffer2` because we use a doubly-indirect block so we use each buffer for each layer.


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


## Synchronization



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
