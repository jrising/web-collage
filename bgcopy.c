#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PUT_DIR "/home/jrising/tmp/web-collage/"

int main(int argc, char *argv[]) {
  char buff[1024];

  if (argc != 2)
    return 0;

  srand(time(NULL));

  /* Move (mv) the file to PUT_DIR */
  sprintf(buff, "mv \"%s\" \"%s/%ld%s\"", argv[1], PUT_DIR, rand(),
	  strrchr(argv[1], '/') + 1);
  system(buff);
}

