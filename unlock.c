#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };
  int fd;

  if (argc != 2) {
    printf("Usage: unlock <file>\n");
    exit(1);
  }
  
  fl.l_pid = getpid();

  if ((fd = open(argv[1], O_RDWR)) == -1) {
    perror("open");
    exit(1);
  }

  if (fcntl(fd, F_GETLK, &fl) == -1) {
    perror("fcntl");
    exit(1);
  }

  printf("Lock found.\n");
  printf("%ld (READ = %ld, WRITE = %ld, UNLCK = %ld), %ld, %ld, %ld, %ld\n",
	 fl.l_type, F_WRLCK, F_RDLCK, F_UNLCK, fl.l_whence, fl.l_start,
	 fl.l_len, fl.l_pid);

  fl.l_type = F_UNLCK;  /* set to unlock same region */

  if (fcntl(fd, F_SETLK, &fl) == -1) {
    perror("fcntl");
    exit(1);
  }

  printf("Unlocked.\n");

  close(fd);
}
