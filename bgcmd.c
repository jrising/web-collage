#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TMP_DIR "/home/jrising/tmp/web-collage/"
#define BASE_PATH "bgfull.gif"
#define DISP_CMD "xli -onroot -quiet -fork bgfull.gif" // ""
#define ONLY_PICTS 0
#define THREE_OPTS 1
#define COMB0_CMD "composite -compose atop -geometry +%d+%d \"%s\" " BASE_PATH " " TMP_DIR "bgfull2.gif"
#define COMB1_CMD "composite -compose atop -geometry +%d+%d \"%s\" " BASE_PATH " " TMP_DIR "bgfull2.gif"
#define COMB2_CMD "composite -compose atop -geometry +%d+%d \"%s\" " BASE_PATH " " TMP_DIR "bgfull2.gif"
#define AFTER_COMB TMP_DIR "bgfull2.gif"
#define CONV_CMD "convert -modulate 99 " TMP_DIR "bgfull2.gif " TMP_DIR "bgfull3.gif"
#define NOCV_CMD "composite " TMP_DIR "bgfull2.gif -gravity center -resize 99% bgorig.gif " TMP_DIR "bgfull3.gif"  //"mv " TMP_DIR "bgfull2.gif " TMP_DIR "bgfull3.gif"
#define CLEAN_CMD "mv " TMP_DIR "bgfull3.gif " BASE_PATH
#define PUT_DIR "/home/jrising/tmp/web-collage/"
#define OUTPUT_FILE TMP_DIR "bgfull3.gif"
#define BACK_OUT 50
#define SCREEN_X (1152 + BACK_OUT)
#define SCREEN_Y (900 + BACK_OUT)
#define ID_CMD "identify %s"
#define ID_GOOD " 1152x900 "

unsigned twocint(char *buff);

int main(int argc, char *argv[]) {
  char *end;
  char buff[1024], syscmd[1024];
  FILE *sysout;
  FILE *filechk;
  FILE *rectfp;
  char images;
  int combchoice;

  if (argc != 2)
    return 0;

  end = argv[1] + strlen(argv[1]) - 4;

  srand(time(NULL));

  /* Add images to the background */
  if (!strcasecmp(end, "jpeg") || !strcasecmp(end, ".jpg") ||
      (!strcasecmp(end, ".gif") || !strcasecmp(end, "tiff") ||
       !strcasecmp(end, ".ppm") || !strcasecmp(end, ".bmp") ||
       !strcasecmp(end, "pict") || !strcasecmp(end, ".pnm")) && !ONLY_PICTS) {

    /* If the output file is already there, wait */
    while (filechk = fopen(OUTPUT_FILE, "r"))
      fclose(filechk);
    /* Combination */
    combchoice = (rand() % 3) * THREE_OPTS;
    if (combchoice == 0)
      sprintf(syscmd, COMB0_CMD, rand() % SCREEN_X - BACK_OUT,
	      rand() % SCREEN_Y - BACK_OUT, argv[1]);
    else if (combchoice == 1)
      sprintf(syscmd, COMB1_CMD, rand() % SCREEN_X - BACK_OUT,
	      rand() % SCREEN_Y - BACK_OUT, argv[1]);
    else
      sprintf(syscmd, COMB2_CMD, rand() % SCREEN_X - BACK_OUT,
	      rand() % SCREEN_Y - BACK_OUT, argv[1]);
    printf("%s\n", syscmd);
    system(syscmd);
    if (!ensureValid(AFTER_COMB))
      exit(0);
    /* Convolution */
    if (rand() % 2 == 0)
      system(CONV_CMD);
    else
      system(NOCV_CMD);
    if (!ensureValid(OUTPUT_FILE))
      exit(0);
    /* Check if output has data */
    system(CLEAN_CMD);
    system(DISP_CMD);      
  }
    
  /* Move (mv) the file to PUT_DIR */
  sprintf(buff, "mv \"%s\" \"%s/%ld%s\"", argv[1], PUT_DIR, rand(),
	  strrchr(argv[1], '/') + 1);
  system(buff);
}

unsigned twocint(char *buff) {
  return (buff[0] - '0') * 10 + (buff[1] - '0');
}

int ensureValid(char *file) {
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

  if (!strstr(buff, ID_GOOD))
    return 0;

  return 1;
}
