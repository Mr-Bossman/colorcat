#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define COLOR_COUNT 14
static bool color_256 = false;
static bool each_char = true;
static bool rotate_color=true; // inside chars only works if each char is chosen
static bool random_start_color=false;
static void color(unsigned int color){
  if(color > 6)
    printf("\e[9%um",color%7);
  else
    printf("\e[3%um",color);
}
static void Scolor(unsigned int color){
  printf("\e[38;5;%um",color);
}
static void pchar(const char* str,unsigned int start){
  unsigned int curent_color = start;
  while (*str)
  {
    if(!rotate_color)
      curent_color = (rand()%(COLOR_COUNT-1))+1;
    color(curent_color);
    printf("%c",*str);
    str++;
    curent_color++;
    if(curent_color == COLOR_COUNT) curent_color = 1;
  }
}

static void help(int code){
  printf("bad");
  exit(code);
}
int main(int argc, char **argv){
  int opt;
  while ((opt = getopt(argc, argv, "CrRh")) != -1) {
    switch (opt) {
      case 'C': each_char = false; break;
      case 'r': random_start_color = true; break;
      case 'R': rotate_color = false; break;
      case '2': color_256 = true; break;
      case 'h': help(0); break;
      default:
        help(-1);
    }
  }
  srand(time(NULL));
  char *line = NULL;
  size_t len = 0;
  ssize_t lineSize = 0;
  unsigned int curent_color = 1;
  while ((lineSize = getline(&line, &len, stdin)) != -1)
  {
    if(random_start_color)
      curent_color = (rand()%(COLOR_COUNT-1))+1;
    color(curent_color);
    if(each_char)
      pchar(line,curent_color);
    else
    printf("%s",line);
    curent_color++;
    if(curent_color == COLOR_COUNT) curent_color = 1;
  }
  free(line);
  return 0;
}