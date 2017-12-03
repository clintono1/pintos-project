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

  char name[3][READDIR_MAX_LEN + 1];
  char file_name[] = "test";
  char contents[512];
  int fd;
  int writesize;

  CHECK (create (file_name, 65536), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  int before_test = diskwrites ();
  clearcache ();

  int i = 0;
  while (i < 128)
    {
      write (fd, contents, 512);
      i++;
    }

  int j = 0;
  while (j < 128)
    {

      read (fd, contents + j, 512);

      j++;
    }

  CHECK (diskwrites () - before_test < 200, "num actual writes: \"%d\"", diskwrites () - before_test);

  close (fd);

}