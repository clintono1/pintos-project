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
  CHECK (mkdir ("start"), "mkdir \"start\"");
  CHECK (chdir ("start"), "chdir \"start\"");

  char name[3][READDIR_MAX_LEN + 1];
  char file_name[16], dir_name[16];
  char contents[2048];
  int fd;

  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  int i = 0;
  while (i < 2048)
    {
      write(fd, contents + i, 1);
      i++;
    }
  CHECK(diskwrites () < 200, "num actual writes");

  close (fd);


}