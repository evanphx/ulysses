#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/syscall.h>

char big_stuff[30000];


int con = 0;

void kputs(const char* str) {
  __syscall(SYS_kprint, str);
}

void test_read_raw() {
  int fd = open("/dev/ada", 0);

  char* buf = big_stuff;
  buf[2000] = 0;
  read(fd, big_stuff, 100);

  read(fd, big_stuff + 100, 3000 - 100);

  if(buf[2000] != 0) {
    buf[2010] = 0;
    kputs("read works!!\n");
    /* kputs(buf + 1986, 24); */
  } else {
    kputs("read doesn't works!!\n");
  }
}

void test_read_file() {
  int fd = open("/data/words", 0);

  char* buf = big_stuff;
  read(fd, big_stuff, 100);
  read(fd, big_stuff+100, 3000-100);

  buf[410] = 0;
  kputs(buf + 403);
  buf[815] = 0;
  kputs(buf + 802);

  kputs("\n");

  if(buf[2000] != 0) {
    kputs("read works!!\n");
  } else {
    kputs("read doesn't works!!\n");
  }
}

void test_read_file_13th_block() {
  kputs("read from 13th block\n");
  int fd = open("/data/words", 0);

  char* buf = big_stuff;
  read(fd, big_stuff, 14016);

  buf[14017] = 0;
  kputs(buf + 14000);

  if(buf[2000] != 0) {
    kputs("read works!!\n");
  } else {
    kputs("read doesn't works!!\n");
  }
}

void test_read_file_300th_block() {
  kputs("read from 300th block\n");
  int fd = open("/data/words", 0);

  char* buf = big_stuff;

  __syscall(SYS_seek, fd, 300100, SEEK_SET);
  read(fd, big_stuff, 24);

  buf[25] = 0;
  kputs(buf);

  if(buf[0] != 0) {
    kputs("read works!!\n");
  } else {
    kputs("read doesn't works!!\n");
  }
}

int new_main(int argc, char** argv) {
  /* const char* msg = "console from userspace!\n"; */
  char buf[256];

  /* int tmp2 = open("/tmp/blah", 0); */
  /* read(tmp2, buf, strlen(msg)+1); */

  /* printf("tmpfs read: %s\n", buf); */
  printf("done!\n");
  return 0;
}

int main(int argc, char** argv, char** environ) {
  big_stuff[2000] = 1;

  /* __syscall(SYS_mount, "/dev", "devfs", 0); */
  __syscall(SYS_mount, "/data", "ext2", "/dev/ada");
  __syscall(SYS_mount, "/tmp", "tmpfs", 0);

  int sin = open("/dev/console", 0);
  int sout = open("/dev/console", 0);

  int tmp = open("/tmp/blah", 0);

  printf("argc=%d, argv=%p, env=%p\n", argc, argv, environ);

  const char* msg = "console from userspace!\n";
  write(sout, msg, strlen(msg));

  /* printf("tmp file is %d\n", tmp); */
  /* printf("stack is at %x\n", &sin); */

  write(tmp, msg, strlen(msg)+1);

  char buf[256];

  int tmp2 = open("/tmp/blah", 0);
  read(tmp2, buf, strlen(msg)+1);

  printf("tmpfs read: %s\n", buf);

  int tmp3 = open("/tmp/foo", 0);

  write(tmp3, msg, strlen(msg)+1);

  /*
  DIR* dir = opendir("/tmp");
  struct dirent* de = readdir(dir);

  while(de) {
    printf("read: %s\n", de->d_name);
    de = readdir(dir);
  }*/

  printf("argc=%d, argv=%p, env=%p\n", argc, argv, environ);
  printf("argv[0]=%p (%s)\n", argv[0], argv[0]);
  printf("argv[1]=%p (%s)\n", argv[1], argv[1]);

  kputs("hello from user land!\n");
  /* kputs("hello again from user land, sleeping now.\n"); */
  /* __syscall_sleep(2); */
  /* kputs("done sleeping! opening file...\n"); */

  /* kputs("test 1\n"); */
  /* test_read_file(); */

  /* test_read_file_13th_block(); */
  /* test_read_file_300th_block(); */
  test_read_file_300th_block();

  /* kputs("test 2\n"); */
  /* test_read_file(); */

  printf("exit 0\n");
  exit(1);

  return;
}
