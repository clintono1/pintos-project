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



## Algorithms




## Synchronization


## Rationale

# Part 3: Subdirectories

## Data structures and functions

	Relevant Files: inode.c, thread.c, filesys.c, directory.c, syscall.c

	In directory.c dir_add(), need to resize directory....
	-> check if e is in use

	Maintain a separate current directory for each process.
	each thread will have a field:
		dir *cwd
	its field will be set in process.c start_process():
		cwd = dir_reopen(cwd_parent)
	
	for the syscalls that include a filename: tokenize the filename, and reduce it to its relative form (open, remove, create, exec)

	for the case in which ../ or ./ are provided, look at the cwd of the currently running process....

	in filesys.c, modify the function filesys_open to open the directory of the currently running process.....

		dir *dir = dir_open(current_inode) //how do we access cur_inode

	modify thread.c to add directories to fd table (maybe inodes instead of file * or dir *)

		insert_dir_to_fd_table(dir *dir) // returns the fd of the dir
	add function:

		dir* get_dir(int fd) // returns the the dir at location fd

	check before dir_close is called, that dir is empty except for .. and ., that the dir is not root, and that no process's cwd is that dir.


## Algorithms




## Synchronization


## Rationale

