#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <signal.h>
/*
  TODO: Random function that isn't bad.
  TODO: Signal that works
  TODO: Cross platform time seed
*/
#define COLOR_COUNT 20 // N hues

static bool color_256 = true;
static bool each_char = true;
static bool rotate_color = true; // inside chars only works if each char is chosen
static bool random_start_color = false;

static void clear(int sig);
static uint hue_to_ansiNum(double hue);
static void Scolor(uint color);
static void color(uint color);
static void pchar(const char* str,uint start);
static void help(int code);
static long long time_millis();

static void clear(int sig){
  puts("\033[0m");
  exit(sig);
}

static uint hue_to_ansiNum(double hue) {
    const double sat = 1.0;
    const double gam = 0.75;
    double p, q, t, ff;
    double r, g, b;
    long i;
    if(hue >= 360.0)
      hue = 0.0;
    hue /= 60.0;
    i = (long)hue;
    ff = hue - i;
    p = gam * (1.0 - sat);
    q = gam * (1.0 - (sat * ff));
    t = gam * (1.0 - (sat * (1.0 - ff)));

    switch(i) {
      case 0:
          r = gam;
          g = t;
          b = p;
          break;
      case 1:
          r = q;
          g = gam;
          b = p;
          break;
      case 2:
          r = p;
          g = gam;
          b = t;
          break;

      case 3:
          r = p;
          g = q;
          b = gam;
          break;
      case 4:
          r = t;
          g = p;
          b = gam;
          break;
      case 5:
      default:
          r = gam;
          g = p;
          b = q;
          break;
    }
    r *= 6;
    g *= 6;
    b *= 6;
    return 16 + (36*(int)r) + (6*(int)g) + (int)b;
}

static void Scolor(uint color) {
  printf("\033[38;5;%um",color);
}

static void color(uint color) {
  if(color_256)
    Scolor(hue_to_ansiNum((color*360)/COLOR_COUNT));
  else {
    color%=13;
    color++;
    if(color > 7)
      printf("\033[9%um",color-7);
    else
      printf("\033[3%um",color);
  }
}

static void pchar(const char* str,uint start) {
  uint curent_color = start;
  while (*str) {
    if(!rotate_color)
      curent_color = rand()%COLOR_COUNT;
    color(curent_color);
    printf("%c",*str);
    str++;
    curent_color++;
    if(curent_color == COLOR_COUNT)
      curent_color = 1;
  }
}

static void help(int code) {
  puts("+------------------------------------+");
  puts("| -h - This message.                 |");
  puts("| -C - Only change whole line color. |");
  puts("| -r - Random line color.            |");
  puts("| -R - All random color.             |");
  puts("| -5 - Dissable 8-bit mode.          |");
  puts("+------------------------------------+");
  exit(code);
}

static long long time_millis() {
  struct timeval te;
  gettimeofday(&te, NULL);
  long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
  return milliseconds;
}

int main(int argc, char **argv){
  signal(SIGINT, clear);
  signal(SIGTERM, clear);
  signal(SIGABRT, clear);
  FILE *fp = stdin;
  int opt;
  while ((opt = getopt(argc, argv, "CrRh")) != -1) {
    switch (opt) {
      case 'C': each_char = false; break;
      case 'r': random_start_color = true; break;
      case 'R': rotate_color = false,random_start_color = true; break;
      case '5': color_256 = false; break;
      case 'h': help(0); break;
      default:
        help(EXIT_FAILURE);
    }
  }

  if(*argv[argc-1] != '-' && argc > 1) {
    fp = fopen(argv[argc-1], "r");
    if (!fp) {
      fprintf(stderr, "Error opening file '%s'\n", argv[argc-1]);
      return EXIT_FAILURE;
    }
  }

  srand(time_millis(NULL));
  char *line = NULL;
  size_t len = 0;
  ssize_t lineSize = 0;
  uint curent_color = 1;
  while ((lineSize = getline(&line, &len, fp)) != -1) {
    if(random_start_color)
      curent_color = rand()%COLOR_COUNT;
    color(curent_color);
    if(each_char)
      pchar(line,curent_color);
    else
      printf("%s",line);
    curent_color++;
    if(curent_color == COLOR_COUNT)
      curent_color = 1;
  }
  free(line);
  clear(0);
  return 0;
}
