#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define FILES_DIR "/home/jrising/tmp/tapest/%s/"
#define DISP_CMD "xli -onroot -quiet -fork bgfull.gif" // ""
#define ONLY_PICTS 0
#define THREE_OPTS 1
#define COMB0_CMD "composite -compose atop -geometry +%d+%d \"%s\" data/%s.jpg " FILES_DIR "bgfull2.jpg"
#define COMB1_CMD "composite -compose atop -geometry +%d+%d \"%s\" data/%s.jpg " FILES_DIR "bgfull2.jpg"
#define COMB2_CMD "composite -compose atop -geometry +%d+%d \"%s\" data/%s.jpg " FILES_DIR "bgfull2.jpg"
#define AFTER_COMB FILES_DIR "bgfull2.jpg"
#define CONV_CMD "convert -modulate 99 " FILES_DIR "bgfull2.jpg " FILES_DIR "bgfull3.jpg"
#define NOCV_CMD "mv " FILES_DIR "bgfull2.jpg " FILES_DIR "bgfull3.jpg"
#define CLEAN_CMD "mv " FILES_DIR "bgfull3.jpg data/%s.jpg"
#define OUTPUT_FILE FILES_DIR "bgfull3.jpg"
#define BACK_OUT 50
#define ID_CMD "identify %s"

unsigned twocint(char *buff);

int main(int argc, char *argv[]) {
  char beforefile[1024], aftercomb[1024], outputfile[1024], convcmd[1024], nocvcmd[1024], cleancmd[1024];
  char *end;
  char buff[1024], syscmd[1024];
  FILE *sysout;
  FILE *filechk;
  FILE *rectfp;
  char images;
  int combchoice;
  int width = 0, height = 0;

  if (argc != 3)  /* name file */
    return 0;

  sprintf(beforefile, "data/%s.jpg", argv[1]);
  sprintf(aftercomb, AFTER_COMB, argv[1]);
  sprintf(outputfile, OUTPUT_FILE, argv[1]);
  sprintf(convcmd, CONV_CMD, argv[1], argv[1]);
  sprintf(nocvcmd, NOCV_CMD, argv[1], argv[1]);
  sprintf(cleancmd, CLEAN_CMD, argv[1]);

  end = argv[2] + strlen(argv[2]) - 4;

  srand(time(NULL));

  /* Add images to the background */
  if (!strcasecmp(end, "jpeg") || !strcasecmp(end, ".jpg") ||
      (!strcasecmp(end, ".gif") || !strcasecmp(end, "tiff") ||
       !strcasecmp(end, ".ppm") || !strcasecmp(end, ".bmp") ||
       !strcasecmp(end, "pict") || !strcasecmp(end, ".pnm") ||
       !strcasecmp(end, ".png")) && !ONLY_PICTS) {

    /* If the output file is already there, wait */
    while (filechk = fopen(outputfile, "r"))
      fclose(filechk);
    if (!ensureValid(beforefile, &width, &height))
      exit(0);

    /* Combination */
    combchoice = (rand() % 3) * THREE_OPTS;
    if (combchoice == 0)
      sprintf(syscmd, COMB0_CMD, rand() % width - BACK_OUT,
	      rand() % height - BACK_OUT, argv[2], argv[1], argv[1]);
    else if (combchoice == 1)
      sprintf(syscmd, COMB1_CMD, rand() % width - BACK_OUT,
	      rand() % height - BACK_OUT, argv[2], argv[1], argv[1]);
    else
      sprintf(syscmd, COMB2_CMD, rand() % width - BACK_OUT,
	      rand() % height - BACK_OUT, argv[2], argv[1], argv[1]);
    printf("%s\n", syscmd);
    system(syscmd);
    if (!ensureValid(aftercomb, &width, &height))
      exit(0);
    /* Convolution */
    if (rand() % 2 == 0)
      system(convcmd);
    else
      system(nocvcmd);
    if (!ensureValid(outputfile, &width, &height))
      exit(0);
    /* Check if output has data */
    system(cleancmd);
    system(DISP_CMD);
  }
}

unsigned twocint(char *buff) {
  return (buff[0] - '0') * 10 + (buff[1] - '0');
}

int ensureValid(char *file, int *width, int *height) {
  FILE *filechk;
  FILE *sysout;
  char buff[1024], syscmd[1024];

  filechk = fopen(file, "r");
  if (!filechk || fgetc(filechk) == EOF) {
    if (filechk) {
      fclose(filechk);
      unlink(file);
    }
    return 0;
  }

  fclose(filechk);

  sprintf(syscmd, ID_CMD, file);
  sysout = popen(syscmd, "r");
  if (!sysout)
    return 0;

  if (!fgets(buff, 1024, sysout)) {
    pclose(sysout);
    return 0;
  }

  pclose(sysout);

  printf(buff);

  /* size should be after "file JPEG " */
  if (!isdigit(buff[strlen(file) + 6]) && buff[strlen(file) + 6] > '0')
    return 0;

  int mywidth = atoi(buff + strlen(file) + 6);
  int myheight = atoi(buff + strlen(file) + 6 + (mywidth < 100 ? (mywidth < 10 ? 1 : 2) : (mywidth >= 1000 ? 4 : 3)) + 1);
  printf("%d, %d\n", mywidth, myheight);
  if (*width == 0) {
    *width = mywidth;
    *height = myheight;
  }

  if (*width != mywidth || *height != myheight)
    return 0;

  return 1;
}
