#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
ssize_t getline(char **ptr, size_t *n, FILE *stream)
{
	if (!*ptr)
		*ptr = malloc(100);
	if (fgets(*ptr, 100, stream) == NULL)
		return -1;
	if (ferror(stream))
		return -1;
	*n = strnlen(*ptr, 100);
	return *n;
}
#endif

#if __has_include(<unistd.h>)
#include <sys/time.h>
#define POSIX
#endif

#ifndef uint
#define uint uint32_t
#endif

extern char *optarg;

static bool color_256 = true;
static bool color_bold = false;
static bool each_char = true;
static bool rotate_color = true;	// inside chars only works if each char is chosen
static bool random_start_color = false;
static uint color_count = 50;		// N hues
static uint start_count = 0;		// Starting hue
static uint shift_c = 1;

static void clear(int sig);
static uint hue_to_ansiNum(double hue);
static void Scolor(uint color);
static void color(uint color);
static void pchar(const char *str, uint start);
static void help(int code);
static double dmod(double x, double y);

/* Random start*/
static void init_rand(void);
static int c_rand(void);
static long long time_millis(void);
static FILE *Fdat;

#ifdef POSIX
static long long time_millis(void)
{
	struct timeval te;
	gettimeofday(&te, NULL);
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
	return milliseconds;
}
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32)
static long long time_millis(void)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	LONG time_ms = (time.wSecond * 1000) + time.wMilliseconds;
	return time_ms;
}
#else
static long long time_millis(void)
{
	return time(NULL);
}
#endif

static int c_rand(void)
{
	if (!Fdat)
	{
		srand(time_millis() + rand());
		return rand();
	}
	else
	{
		int rand_data;
		fread(&rand_data, 1, sizeof(int), Fdat);
		return rand_data;
	}
}
static void init_rand(void)
{
	Fdat = fopen("/dev/urandom", "r");
	srand(time_millis());
}
/* Random end */

double dmod(double x, double y)
{
	return x - (int)(x / y) * y;
}

static void clear(int sig)
{
	puts("\033[0m");
	fclose(Fdat);
	exit(sig);
}

static void strip_ansi(char * str)
{
	const size_t len = strlen(str);
	char *ptr = str;
	char *ptr2;
	while(1)
	{
		ptr = strchr(ptr,'\033');
		if (!ptr)
			return;
		ptr2 = strchr(ptr,'m');
		char chk[] ="hldABCDEFGHJKSTfinsu0123456789";
		for (size_t i = 0; (!ptr2 || (ptr2-ptr) > 10) && i < ((sizeof chk) - 1); i++)
		{
			ptr2 = strchr(ptr,chk[i]);
		}
		if (!ptr2 || (ptr2-ptr) > 10)
		{
			ptr++;
			continue;
		}
		memcpy(ptr, ptr2 + 1, len - (ptr2 - str));
	}
}

static uint hue_to_ansiNum(double hue)
{
	const double sat = 1.0;
	const double gam = 0.75;
	double p, q, t, ff;
	double r, g, b;
	long i;
	hue = dmod(hue, 360.0);
	hue /= 60.0;
	i = (long)hue;
	ff = hue - i;
	p = gam * (1.0 - sat);
	q = gam * (1.0 - (sat * ff));
	t = gam * (1.0 - (sat * (1.0 - ff)));
	switch (i)
	{
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
	return 16 + (36 * (int)r) + (6 * (int)g) + (int)b;
}

static void Scolor(uint color)
{
	printf("\033[38;5;%um", color);
}

static void color(uint color)
{
	if (color_256)
		Scolor(hue_to_ansiNum(((color * 360) / color_count) + start_count));
	else
	{
		color %= 13;
		color++;
		if (color > 7)
			printf("\033[%s%um", (color_bold)? "1;3" : "9", color - 7);
		else
			printf("\033[3%um", color);
	}
}

static void pchar(const char *str, uint start)
{
	uint curent_color = start;
	while (*str)
	{
		if (!rotate_color)
			curent_color = c_rand() % color_count;
		color(curent_color);
		printf("%c", *str);
		str++;
		curent_color++;
		if (curent_color >= color_count)
			curent_color = 0;
	}
}

static void help(int code)
{
	puts("+----------------------------------------+");
	puts("| -h - This message.                     |");
	puts("| -C - Only change whole line color.     |");
	puts("| -r - Random line color.                |");
	puts("| -R - All random color.                 |");
	puts("| -5 - Dissable 8-bit mode.              |");
	puts("| -B - Use bold as bright color.         |");
	puts("| -s - Starting color in hue 360*.       |");
	puts("| -a - Total shades to use.              |");
	puts("| -A - Color shift amount for new lines. |");
	puts("+----------------------------------------+");
	exit(code);
}

int main(int argc, char **argv)
{
	signal(SIGINT, clear);
	signal(SIGTERM, clear);
	signal(SIGABRT, clear);
	FILE *fp = stdin;
	int opt;
	while ((opt = getopt(argc, argv, "CrRh5Ba:s:A:")) != -1)
	{
		switch (opt)
		{
		case 'C':
			each_char = false;
			break;
		case 'r':
			random_start_color = true;
			break;
		case 'R':
			rotate_color = false, random_start_color = true;
			break;
		case '5':
			color_256 = false;
			break;
		case 'B':
			color_bold = true;
			break;
		case 'h':
			help(0);
			break;
		case 'a':
		{
			errno = 0;
			color_count = strtol(optarg, NULL, 10);
			if (errno)
			{
				printf("invalid value for -%c", opt);
				help(EXIT_FAILURE);
			}
			break;
		}
		case 's':
		{
			errno = 0;
			start_count = strtol(optarg, NULL, 10);
			if (errno)
			{
				printf("invalid value for -%c", opt);
				help(EXIT_FAILURE);
			}
			break;
		}
		case 'A':
		{
			errno = 0;
			shift_c = strtol(optarg, NULL, 10);
			if (errno)
			{
				printf("invalid value for -%c", opt);
				help(EXIT_FAILURE);
			}
			break;
		}
		default:
			help(EXIT_FAILURE);
		}
	}

	if (*argv[argc - 1] != '-' && argc > 1)
	{
		char var_opts[] = "asA";
		bool file = true;
		for (size_t i = 0; i < sizeof(var_opts) - 1; i++)
		{
			char opts[] = "-a";
			opts[1] = var_opts[i];
			if (!strcmp(argv[argc - 2], opts))
				file = false;
		}
		if (file)
		{
			fp = fopen(argv[argc - 1], "r");
			if (!fp)
			{
				fprintf(stderr, "Error opening file '%s'\n", argv[argc - 1]);
				return EXIT_FAILURE;
			}
		}
	}
	init_rand();
	char *line = NULL;
	size_t len = 0;
	ssize_t lineSize = 0;
	uint curent_color = 0;
	while ((lineSize = getline(&line, &len, fp)) != -1)
	{
		strip_ansi(line);
		if (random_start_color)
			curent_color = c_rand() % color_count;
		color(curent_color);
		if (each_char)
			pchar(line, curent_color);
		else
			printf("%s", line);
		curent_color += shift_c;
		if (curent_color <= 0)
			curent_color = color_count - 1;
		if (curent_color >= color_count)
			curent_color = 0;
	}
	free(line);
	clear(0);
	fclose(Fdat);
	return 0;
}
