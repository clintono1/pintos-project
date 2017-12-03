/* Test your buffer cache’s effectiveness by measuring its cache hit rate. First, reset the buffer cache.
Open a file and read it sequentially, to determine the cache hit rate for a cold cache. Then, close
it, re-open it, and read it sequentially again, to make sure that the cache hit rate improves.
   */

#include <string.h>
#include <stdio.h>
#include <syscall.h>
// #include "filesys/inode.h"
// #include "filesys/inode.c"

// #include "filesys/filesys.c"

#include "tests/lib.h"
#include "tests/main.h"

static char buf1[2048];

void
test_main (void)
{
  // CHECK (mkdir ("start"), "mkdir \"start\"");
  // CHECK (chdir ("start"), "chdir \"start\"");

  char name[3][READDIR_MAX_LEN + 1];
  char file_name[] = "test";
  char contents[2048];
  int fd;

  /* Create file. */
  CHECK (create (file_name, 0), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  // msg ("fd: %d", fd);
  if (write (fd, contents, strlen (contents)) != (int) strlen (contents))
    {
      CHECK (remove (file_name), "remove \"%s\"", file_name);
    }
  close (fd);

  clearcache ();

  fd = open (file_name);
  read (fd, buf1, 2048);
  unsigned long long cache_misses1 = diskreads ();
  msg ("1st time: %llu", cache_misses1);
  read (fd, buf1, 2048);
  unsigned long long cache_misses2 = diskreads ();  
  // msg("2nd time: %llu", cache_misses2);
  CHECK (cache_misses1 > cache_misses2, "should have less cache misses on 2nd read");

}