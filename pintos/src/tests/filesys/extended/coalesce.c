/*
Test your buffer cache’s ability to coalesce writes to the same sector. Each block device keeps
a read_cnt counter and a write_cnt counter. Write a large file byte-by-byte (make the total
file size at least 64KB, which is twice the maximum allowed buffer cache size). Then, read it in
byte-by-byte. The total number of device writes should be on the order of 128 (because 64KB is
128 blocks).

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

  CHECK (create (file_name, 65536), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  clearcache ();
  int before_test = diskwrites ();

  int i = 0;
  int k = 0;
  while (i < 128)
    {
      for (k = 0; k < 512; k++)
        write (fd, contents + k, 1);
      i++;
    }

  msg ("seek \"%s\" to 0", file_name);
  seek (fd, 0);

  int j = 0;
  while (j < 128)
    {
      for (k = 0; k < 512; k++)
        read (fd, contents + k, 1);
      j++;
    }

  int after_test = diskwrites ();
  CHECK (after_test - before_test == 128, "number of disk writes: \"%d\"", after_test - before_test);

  close (fd);

}