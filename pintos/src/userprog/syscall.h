#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct dir;

void syscall_init (void);
void syscall_exit (void);
struct dir * get_last_directory (const char *path);
struct dir * get_last_directory_absolute (const char *path);
struct dir * get_last_directory_relative (const char *path);

#endif /* userprog/syscall.h */
