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
	struct lock block_lock; // Lock this sector
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
before we write to it, we must lock the cache entry before we read it. Then,
after we finish writing, we can release the lock. We do not need to lock the
sector in `cache_get_block` or `inode_read_at`, only in `inode_write_at`,
since reading does not actually modify the cache entry data and the function is atomic.


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
one cache entry because if not, the cache entry could be evicted before we
read/write but after we check that the entry is in the cache. Thus, we found that
we always need to check if a sector is in the cache and then read or write from
it atomically.

# Part 2: Extensible Files

## Data structures and functions



## Algorithms




## Synchronization


## Rationale

# Part 3: Subdirectories

## Data structures and functions



## Algorithms




## Synchronization


## Rationale

