/* Test your buffer cache’s effectiveness by measuring its cache hit rate. First, reset the buffer cache.
Open a file and read it sequentially, to determine the cache hit rate for a cold cache. Then, close
it, re-open it, and read it sequentially again, to make sure that the cache hit rate improves.
   */

#include <string.h>
#include <stdio.h>
#include <syscall.h>

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  char file_name[] = "test";
  char contents[512];
  int fd;

  /* Create file. */
  CHECK (create (file_name, 0), "create \"%s\"", file_name);

  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  int i = 0;
  int k = 0;
  while (i < 50)
    {
      for (k = 0; k < 512; k++)
        write (fd, contents + k, 1);
      i++;
    }

  msg ("seek \"%s\" to 0", file_name);
  seek (fd, 0);
  clearcache ();
  int disk_reads0 = diskreads ();
  msg ("reading \"%s\"", file_name);

  // Read first time
  int j = 0;
  while (j < 50)
    {
      for (k = 0; k < 512; k++)
        read (fd, contents + k, 1);
      j++;
    }
  int disk_reads1 = diskreads();
  int cache_misses1 = disk_reads1 - disk_reads0;
  msg("closing \"%s\"", file_name);
  close (fd);
  CHECK ((fd = open (file_name)) > 1, "reopening \"%s\"", file_name);

  // Read second time
  j = 0;
  while (j < 50)
    {
      for (k = 0; k < 512; k++)
        read (fd, contents + k, 1);
      j++;
    }

  msg ("reading \"%s\"", file_name);
  int disk_reads2 = diskreads ();
  int cache_misses2 = disk_reads2 - disk_reads1;
  CHECK (cache_misses1 > cache_misses2, "should have less cache misses on 2nd read");
  close (fd);
}