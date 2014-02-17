#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define WGET_PATH "/usr/bin/wget"
#define FILES_DIR "/home/jrising/tmp/web-collage/%s/"
#define LOG_FILE FILES_DIR "wslog.txt"
#define COLLECT_CMD WGET_PATH " -H -nd -T2 -Q50000 -o " LOG_FILE " -P " FILES_DIR
#define LS_CMD "ls -1 " FILES_DIR
#define DAT_FILE "data/%s.dat"
#define IMG_FILE "data/%s.jpg"
#define BUFFSIZE 512
#define NUMLINESIZE 11
#define DEBUG 0
#define MAX_SIZE 1048576
#define HOST_CROWD 50
#define WAIT_TIME 1

#define KEEP_PROB 2
#define DROP_PROB 2

#define DISLIKE_CNT 0
char *dislikes[] = {};
#define LIKES_CNT 0
char *likes[] = {};

int validpid(pid_t proc);
void freemaster(char *masterfile, FILE *master,
		unsigned long start, unsigned long len);
FILE *waitwritemaster(char *masterfile, unsigned long loc,
		      unsigned long start, unsigned long len);
FILE *waitreadmaster(char *masterfile, unsigned long loc,
		     unsigned long start, unsigned long len);
FILE *waitreadtowrite(char *masterfile, FILE *master,
		      unsigned start, unsigned len);
void voidline(FILE *master);
char *getpath(char *buff, int size, char *path, char *source);
char *getfilename(char *buff, int size, char *url);
char *addfilename(char *url, int size, char *pagefile);
int stripdown(char *path);
char *nexttag(char *buff, int size, FILE *fp);
char *geturl(char *tag);
char *tagoption(char *tag, char *option);
char *strcasestr(char *s1, char *s2);
int findlikes(FILE *fp);

int main(int argc, char *argv[]) {
  char logfile[BUFFSIZE], filesdir[BUFFSIZE], collectcmd[BUFFSIZE], lscmd[BUFFSIZE], datfile[BUFFSIZE], imgfile[BUFFSIZE];
  FILE *master, *page;
  FILE *sysout;
  char pageline[BUFFSIZE], buff[BUFFSIZE], scmd[BUFFSIZE], pagefile[BUFFSIZE];
  char pageurl[BUFFSIZE], fullurl[BUFFSIZE], *url;
  char urldone;
  pid_t wgetchild;
  time_t waitstart;
  char *data;
  unsigned masterlength, masterloc;
  unsigned pageadds;
  long tester;
  int p;
  int firstentry;

  if (argc != 3 && argc != 4) {
    printf("Usage: web-collage <file> <program>\n");
    exit(-2);
  }

  /* Fill out the commands with name */
  sprintf(logfile, LOG_FILE, argv[1]);
  sprintf(filesdir, FILES_DIR, argv[1]);
  sprintf(collectcmd, COLLECT_CMD, argv[1], argv[1]);
  sprintf(lscmd, LS_CMD, argv[1]);
  sprintf(datfile, DAT_FILE, argv[1]);
  sprintf(imgfile, IMG_FILE, argv[1]);
  /* does the temp directory already exist? */
  mkdir(filesdir, S_IRWXU);

  /* Open File */
  if (!(master = fopen(datfile, "r"))) {
    printf("Creating File...\n");
    
    master = fopen(datfile, "w+");
    if (!master) {
      perror("creating master file");
      exit(-1);
    }

    fprintf(master, "%s\n", argv[3]); /* First URL */
    fclose(master);
  } else
    fclose(master);

  printf("Reading File...\n");

  srand48(time(NULL));

  master = waitreadmaster(datfile, 0, 0, 0); /* READ Lock! */
  fseek(master, 0, SEEK_END);
  masterlength = ftell(master);
  freemaster(datfile, master, 0, 0);      /* Unlock! */

  int iter = 0;
  while (iter++ < 3) {
      /* Get random URL */
      masterloc = lrand48() % masterlength;
      firstentry = 0;
      /* READ Lock! */
      master = waitreadmaster(datfile, masterloc, masterloc, BUFFSIZE);
      fgets(pageurl, BUFFSIZE, master); /* skip to line after random char */
      do
	if (!fgets(pageurl, BUFFSIZE - strlen(collectcmd) - 1, master)) {
	  rewind(master);
	  firstentry = 1;
	  fgets(pageurl, BUFFSIZE - strlen(collectcmd) - 1, master);
	}
      while (pageurl[0] == '\t' || pageurl[0] == '\n');

      if (!firstentry) {
	/* Invalidate URL */
	/* READ->WRITE Lock! */
	master = waitreadtowrite(datfile, master, masterloc, BUFFSIZE);
	fseek(master, -(strlen(pageurl) + 0), SEEK_CUR);
	voidline(master);
      }
      freemaster(datfile, master, masterloc, BUFFSIZE);
      /* Unlock! */

      pageurl[strlen(pageurl) - 1] = '\0';

    printf("\nGetting %s\n", pageurl);

    /* Get webpage, checking for any redirection */
    sprintf(scmd, "%s \"%s\"", collectcmd, pageurl);
    waitstart = time(NULL);
    if (!(wgetchild = fork())) {
      system(scmd);
      exit(0);
    }
    while (validpid(wgetchild) && waitstart + 5 > time(NULL));
    if (validpid(wgetchild))
      kill(wgetchild, 9);
    wait(NULL);
    
    sysout = fopen(logfile, "r");
    if (!sysout)
      continue; /* File does not exist */
    while (fgets(buff, BUFFSIZE, sysout)) {
      printf("%s", buff);
      if (!strncmp(buff, "Location:", 9)) {
	if (strchr(buff + 10, ' '))
	  *strchr(buff + 10, ' ') = '\0';
	strcpy(pageurl, buff + 10);
      }
    }
    fclose(sysout);

    unlink(logfile);

    /* Collect new links */
    sysout = popen(lscmd, "r");
    if (!sysout) {
      perror("searching files directory");
      exit(-3);
    }

    strcpy(pagefile, filesdir);
    if (fgets(pagefile + strlen(pagefile), BUFFSIZE, sysout) != NULL) {
      pagefile[strlen(pagefile) - 1] = '\0';  /* remove newline */
      page = fopen(pagefile, "r");

      addfilename(pageurl, BUFFSIZE, pagefile);

      pageadds = 0;

      /* Write Additional URLs */
      rewind(master);
      if (findlikes(page) || (rand() % KEEP_PROB)) { /* possibly just throw away */
	while (nexttag(pageline, BUFFSIZE, page))
	  if (url = geturl(pageline)) {
	    if (strncasecmp(url, "http://", 7)) {
	      if (!getpath(fullurl, BUFFSIZE, url, pageurl))
		continue;
	    } else
	      strcpy(fullurl, url);
	    /* search for place in file to place */
	    urldone = 0;
	    if (strlen(fullurl) > BUFFSIZE / 2)
	      continue;  /* too big, don't add */
	    if (rand() % (pageadds + 1) > KEEP_PROB)
	      continue;  /* add fewer urls as more on page */
	    if (strstr(fullurl, ".com") && rand() % DROP_PROB)
	      continue;  /* only chance to add a .com link */
	    if ((strstr(fullurl, "yahoo.com") ||
		 strstr(fullurl, "www.google.com")) && rand() % DROP_PROB)
	      continue;  /* almost no yahoo.com's, www.google.com's */
	    for (p = 0; p < DISLIKE_CNT; p++)
	      if (strstr(fullurl, dislikes[p])) /* just skip */
		continue;
	    /* READ Lock! */
	    master = waitreadmaster(datfile, 0, 0, 0);
	    while (fgets(buff, BUFFSIZE, master)) {
	      if (!strncmp(buff, fullurl, strlen(fullurl)) ||
		  (!strncmp(buff, fullurl, strchr(buff + strlen("http://"), '/')
			    - buff) && !(rand() % HOST_CROWD))) {
		urldone = 1;
		freemaster(datfile, master, 0, 0);
		/* Unlock! */
		break;
	      }
	      if (buff[0] == '\t' || buff[0] == '\n')
		if (strlen(buff) >= strlen(fullurl) + 1) {
		  masterloc = ftell(master);
		  freemaster(datfile, master, 0, 0);
		  /* Unlock! */
		  printf("Adding %s\n", fullurl);
		  pageadds++;
		  /* WRITE Lock! */
		  waitwritemaster(datfile, masterloc - strlen(buff), 
				  masterloc - strlen(buff), BUFFSIZE);
		  fprintf(master, "%s\n", fullurl);
		  if (strlen(buff) >= strlen(fullurl) + 2)
		    fputc('\t', master);
		  fseek(master, 0, SEEK_END);
		  masterlength = ftell(master);
		  freemaster(datfile, master,
			     masterloc - strlen(buff), BUFFSIZE);
		  /* Unlock! */
		  urldone = 1;
		  break;
		}
	    }
	    if (!urldone) {
	      if (ftell(master) > MAX_SIZE) { /* too big, just remove entries */
		fseek(master, lrand48() % ftell(master), SEEK_SET);
		fgets(buff, BUFFSIZE, master);
		urldone = 1;  /* flag that was greater than MAX_SIZE */
	      }
	      masterloc = ftell(master);
	      freemaster(datfile, master, 0, 0);
	      /* Unlock! */
	      printf("Adding %s\n", fullurl);
	      pageadds++;
	      /* WRITE Lock! */
	      waitwritemaster(datfile, masterloc, masterloc - 2, BUFFSIZE);
	      fprintf(master, "%s\n", fullurl);
	      if (urldone)
		voidline(master);
	      fseek(master, 0, SEEK_END);
	      masterlength = ftell(master);
	      freemaster(datfile, master, masterloc - 2, BUFFSIZE);
	      /* Unlock! */
	    }
	  }
      }

      fclose(page);

      /* Move file to final destination */
      sprintf(scmd, "%s \"%s\" \"%s\"", argv[2], argv[1], pagefile);
      system(scmd);
      sleep(WAIT_TIME);
      sprintf(scmd, "touch %sREMOVE.tmp", filesdir);
      system(scmd);
      sprintf(scmd, "rm %s*", filesdir);
      system(scmd);
    }

    pclose(sysout);
  }
}

int validpid(pid_t proc) {
  FILE *sysout;
  char temp[BUFFSIZE];
  char valid = 0;

  sprintf(temp, "ps -A | grep %d", proc);

  sysout = popen(temp, "r");
  if (sysout) {
    if (fgets(temp, BUFFSIZE, sysout))
      if (!strstr(temp, "<defunct>"))
	valid = 1;
    pclose(sysout);
  }

  return valid;
}

void freemaster(char *masterfile, FILE *master, unsigned long start,
		unsigned long len) {
  struct flock fl = { F_UNLCK, SEEK_SET, start, len, 0 };
  int fd;

  fl.l_pid = getpid();

  if ((fd = open(masterfile, O_RDWR)) == -1) {
    perror("freemaster: open");
    exit(-8);
  }

  if (fcntl(fd, F_SETLK, &fl) == -1) {
    perror("freemaster: fcntl");
    exit(1);
  }

  fclose(master);
  close(fd);
}

FILE *waitwritemaster(char *masterfile, unsigned long loc,
		      unsigned long start, unsigned long len) {
  FILE *master;
  struct flock fl = { F_WRLCK, SEEK_SET, start, len, 0 };
  int fd;

  fl.l_pid = getpid();

  if ((fd = open(masterfile, O_RDWR)) == -1) {
    perror("waitwritemaster: open");
    exit(-8);
  }

  while (fcntl(fd, F_SETLKW, &fl) == -1)
    usleep(1000);

  master = fopen(masterfile, "r+");
  if (!master) {
    perror("waitwritemaster: fopen");
    exit(-7);
  }
  fseek(master, loc, SEEK_SET);

  close(fd);
  return master;
}

FILE *waitreadmaster(char *masterfile, unsigned long loc,
		     unsigned long start, unsigned long len) {
  FILE *master;
  struct flock fl = { F_RDLCK, SEEK_SET, start, len, 0 };
  int fd;

  fl.l_pid = getpid();

  if ((fd = open(masterfile, O_RDWR)) == -1) {
    perror("waitreadmaster: open");
    exit(-8);
  }

  while (fcntl(fd, F_SETLKW, &fl) == -1)
    usleep(1000);

  master = fopen(masterfile, "r");
  if (!master) {
    perror("waitreadmaster: fopen");
    exit(-7);
  }
  fseek(master, loc, SEEK_SET);

  close(fd);
  return master;
} 

FILE *waitreadtowrite(char *masterfile, FILE *master,
		      unsigned start, unsigned len) {
  struct flock fl = { F_WRLCK, SEEK_SET, start, len, 0 };
  int fd;
  unsigned long loc = ftell(master);

  fl.l_pid = getpid();

  if ((fd = open(masterfile, O_RDWR)) == -1) {
    perror("waitreadtowrite: open");
    exit(-8);
  }

  while (fcntl(fd, F_SETLKW, &fl) == -1)
    usleep(1000);

  master = freopen(masterfile, "r+", master);
  if (!master) {
    perror("waitwritemaster: fopen");
    exit(-7);
  }
  fseek(master, loc, SEEK_SET);

  close(fd);
  return master;
}

void voidline(FILE *master) {
  char buff[BUFFSIZE], ch;
  unsigned long start;

  start = ftell(master);
  fputc('\t', master);
  fflush(master);
  fgets(buff, BUFFSIZE, master);

  /* check if next is invalidated */
  if (fgets(buff, BUFFSIZE, master))
    if (buff[0] == '\t' || buff[0] == '\n') {
      fseek(master, -(strlen(buff) + 1), SEEK_CUR);
      fputc('\t', master);  /* combine on one line */
    }
  fflush(master);

  /* check if previous is invalidated */
  fseek(master, start - 2, SEEK_SET);
  while (fgetc(master) != '\n') {
    if (ftell(master) < 2)
      break;
    fseek(master, -2, SEEK_CUR);
  }
  ch = fgetc(master);
  if (ch == '\t' || ch == '\n') {
    fseek(master, -1, SEEK_CUR);
    fgets(buff, BUFFSIZE, master);
    fseek(master, -1, SEEK_CUR);
    fputc('\t', master); /* combine on one line */
  }
  fflush(master);

  fseek(master, start, SEEK_SET);
}

char *getpath(char *buff, int size, char *path, char *source) {
  if (DEBUG)
    printf("getpath: %s, %s\n", path, source);

  if (strlen(source) > size)
    return NULL;

  strcpy(buff, source);

  if (buff[strlen(buff) - 1] != '/')
    stripdown(buff);

  if (path[0] == '/') { /* go to root dir */
    if (strchr(buff + strlen("http://"), '/'))
      strcpy(strchr(buff + strlen("http://"), '/'), path);
    else {
      buff[strlen(buff)] = '/';
      strcpy(buff + strlen(buff), path);
    }
    return buff;
  }

  while (strspn(path, ".") != 0) {
    if (!strncmp(path, "../", 3)) {
      stripdown(buff);
      path += 3;
    } else if (!strncmp(path, "./", 2))
      path += 2;
    else
      break;
  }

  if (strlen(buff) + strlen(path) > size)
    return NULL;

  strcat(buff, path);

  return buff;
}

char *getfilename(char *buff, int size, char *url) {
  if (strrchr(url, '/')) {
    if (strlen(buff) + strlen(strrchr(url, '/') + 1) < size)
      strcpy(buff, strrchr(url, '/') + 1);
  } else
    if (strlen(buff) + strlen(url) < size)
      strcpy(buff, url);

  return buff;
}

char *addfilename(char *url, int size, char *pagefile) {
  char buff[BUFFSIZE];

  getfilename(buff, BUFFSIZE, pagefile);

  /* If pagefile filename is unexpected, add to the end of url */
  if (!strncmp(url + strlen(url) - strlen(buff), buff,
	       size - strlen(url) + strlen(buff)))
    return url; /* already ready */

  if (!(url[strlen(url) - 1] == '/'))
    strncat(url, "/", size);
  strncat(url, buff, size);

  return url;
}

int stripdown(char *path) {
  if (path[strlen(path) - 1] == '/')
    path[strlen(path) - 1] = '\0';

  if (!strrchr(path, '/'))
    return -1;

  if (strrchr(path, '/') != path + strlen("http:/"))
    *(strrchr(path, '/') + 1) = '\0';
}

char *nexttag(char *buff, int size, FILE *fp) {
  int c, i = 1;
  char whitespace = 1;
  int q = 0, p;
  char buffer[BUFFSIZE];

  if (DEBUG)
    printf("nexttag\n");

  while ((c = fgetc(fp)) != '<' && c != EOF)
    if (q < BUFFSIZE - 1)
      buffer[q++] = c;
  buffer[q] = '\0';
  if (c == EOF)
    return NULL;
  for (p = 0; p < DISLIKE_CNT; p++)
    if (strstr(buffer, dislikes[p]))
      if (rand() % 10) /* have a chance to just die */
	return NULL;

  buff[0] = '<';

  while (i < size - 2 && (c = fgetc(fp)) != '>' && c != EOF) {
    if (strchr(" \t\n", c))
      if (whitespace)
	continue;
      else {
	whitespace = 1;
	buff[i++] = ' ';
      }
    else {
      buff[i++] = c;
      whitespace = 0;
    }
  }

  buff[i++] = '>';
  buff[i++] = '\0';

  return buff;
}

char *geturl(char *tag) {
  char *tmp;

  if (DEBUG)
    printf("geturl: %s\n", tag);

  if (!strncasecmp("a ", tag + 1, 2) || !strncasecmp("area ", tag + 1, 5) ||
      !strncasecmp("base ", tag + 1, 5) || !strncasecmp("link ", tag + 1, 5)) {
    tmp = tagoption(tag, "href");
    if (!tmp)
      return NULL;
    if (tmp[0] == '#')
      return NULL;
    if (!strncasecmp("mailto:", tmp, 7))
      return NULL;
    if (strchr(tmp, '#'))
      *strchr(tmp, '#') = '\0';
    return tmp;
  }

  if (!strncasecmp("img ", tag + 1, 4) ||
      !strncasecmp("bgsound ", tag + 1, 8) ||
      !strncasecmp("embed ", tag + 1, 6) ||
      !strncasecmp("frame ", tag + 1, 6) ||
      !strncasecmp("iframe ", tag + 1, 7))
    return tagoption(tag, "src");

  if (!strncasecmp("body ", tag + 1, 5))
    return tagoption(tag, "background");

  return NULL;
}

char *tagoption(char *tag, char *option) {
  char *tmp = strcasestr(tag, option);

  if (DEBUG)
    printf("tagoption\n");

  if (!tmp)
    return NULL;

  /* go to where option setting begins */
  tmp += strlen(option) + strspn(tmp + strlen(option), " =");

  if (tmp[0] == '\"') {
    tmp++;
    if (strchr(tmp, '\"'))
      *strchr(tmp, '\"') = '\0';
    else
      tmp[strlen(tmp) - 1] == '\0';
  } else if (strchr(tmp, ' '))
    *strchr(tmp, ' ') = '\0';
  else
    *strchr(tmp, '>') = '\0';

  return tmp;
}

char *strcasestr(char *s1, char *s2) {
  int i, j = 0;

  for (i = 0; i < strlen(s1); i++)
    if (toupper(s1[i]) == toupper(s2[j])) {
      if (s2[j + 1] == '\0')
	return s1 + i - j;
      j++;
    } else if (j != 0) {
      i -= j;
      j = 0;
    }

  return NULL;
}

int findlikes(FILE *fp) {
  char buffer[BUFFSIZE];
  int c = 0, q, p;
  int likecnt = 0;

  while (c != EOF) {
    q = 0;
    while ((c = fgetc(fp)) != EOF && q < BUFFSIZE - 1)
      buffer[q++] = c;
    buffer[q] = '\0';

    for (p = 0; p < LIKES_CNT; p++)
      if (strstr(buffer, likes[p])) {
	rewind(fp);
	return 1;
      }
  }

  rewind(fp);
  return 0;
}
    
      
    
