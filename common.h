#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <png.h>
#include <getopt.h>
#include <assert.h>
#include "auto_split.h"
#include "palletize.h"
#include "sup.h"
#include "abstract_lists.h"

#include <ass/ass.h>
#include <pthread.h>
#include <stdatomic.h>

#ifndef LINUX
#include <windows.h>
#include <vfw.h>
#endif

// #define MAX_PATH 1024
// #define max(a,b) ((a)>(b)?(a):(b))

#ifndef LINUX
static int64_t gcd( int64_t a, int64_t b )
{
    while (1)
    {
        int64_t c = a % b;
        if( !c )
            return b;
        a = b;
        b = c;
    }
}
#endif

typedef struct {
#if !defined(LINUX)
    PAVISTREAM p_avi;
#else
    int fps_den;
    int fps_num;
    int frames;
    FILE *fh;
#endif
    int width, height;
} avis_input_t;

char *read_file_bytes(FILE *fp, size_t *bufsize);

const char *detect_bom(const char *buf, const size_t bufsize);

#if defined(_WIN32)

#else
char *_fullpath(char *absPath, const char *relPath, size_t maxLength) {
	return realpath(relPath, absPath);
}
#include <libgen.h>
void _splitpath(char *path, char *drive, char *dir, char *fname, char *ext){
	if (drive) drive[0] = 0;
	if (dir) {
		char *tmp = dirname(path);
		strcpy(dir, tmp);
	}
}
#include <errno.h>
#endif

typedef struct {
	ASS_Renderer *ass_renderer;
	ASS_Library *ass_library;
	ASS_Track *ass;
} ass_input_t;

typedef struct {
	int i_width;
	int i_height;
	int i_fps_den;
	int i_fps_num;
} stream_info_t;


int open_file_avis( char *psz_filename, avis_input_t **p_handle, stream_info_t *p_param );
int get_frame_total_avis( avis_input_t *handle );

int read_frame_avis( char *p_pic, avis_input_t *handle, int i_frame );
int close_file_avis( avis_input_t *handle );


void msg_callback(int level, const char *fmt, va_list va, void *data);

int open_file_ass( char *psz_filename, ass_input_t **p_handle, stream_info_t *p_param);

int get_frame_total_ass( ass_input_t *handle, stream_info_t *p_param);

int close_file_ass( ass_input_t *handle );

void get_dir_path(char *filename, char *dir_path);

void write_png(char *dir, int file_id, uint8_t *image, int w, int h, int graphic, uint32_t *pal, crop_t c);

int is_identical (stream_info_t *s_info, char *img, char *img_old);

int is_empty (stream_info_t *s_info, char *img);

void zero_transparent (stream_info_t *s_info, char *img);

void swap_rb (stream_info_t *s_info, char *img, char *out);

/* SMPTE non-drop time code */
void mk_timecode (int frame, int fps, char *buf); /* buf must have length 12 (incl. trailing \0) */

void print_usage ();

int is_extension(char *filename, char *check_ext);

int parse_int(char *in, char *name, int *error);

int parse_tc(char *in, int fps);

int count_non_empty(char *arr[], int size);

typedef struct event_s
{
    int image_number;
    int start_frame;
    int end_frame;
    int graphics;
    int forced;
    crop_t c[2];
} event_t;

STATIC_LIST(event, event_t)

void add_event_xml_real (event_list_t *events, int image, int start, int end, int graphics, crop_t *crops, int forced);

void add_event_xml (event_list_t *events, int split_at, int min_split, int start, int end, int graphics, crop_t *crops, int forced);

void write_sup_wrapper (sup_writer_t *sw, uint8_t *im, int num_crop, crop_t *crops, uint32_t *pal, int start, int end, int split_at, int min_split, int stricter, int forced);

struct framerate_entry_s
{
	char *name;
	char *out_name;
	int rate;
	int drop;
	int fps_num;
	int fps_den;
};

// codes from assrender, modified

#define _r(c) (( (c) >> 24))
#define _g(c) ((((c) >> 16) & 0xFF))
#define _b(c) ((((c) >> 8)  & 0xFF))
#define _a(c) (( (c)        & 0xFF))

#define div256(x)   (((x + 128)   >> 8))
#define div255(x)   ((div256(x + div256(x))))

#define scale(srcA, srcC, dstC) \
	((srcA * srcC + (255 - srcA) * dstC))
#define dblend(srcA, srcC, dstA, dstC, outA) \
	(((srcA * srcC * 255 + dstA * dstC * (255 - srcA) + (outA >> 1)) / outA))

void col2rgb(uint32_t *c, uint8_t *r, uint8_t *g, uint8_t *b);

void make_sub_img(ASS_Image *img, uint8_t *sub_img, uint32_t width);

typedef struct {
    int thread_id;
    int init_frame;
    int last_frame;
    ass_input_t *ass_context;
    avis_input_t* avis_hnd;
    stream_info_t *s_info;
    int have_line;
    int sup_output;
    int xml_output;
    sup_writer_t *sw;
    int n_crop;
    int to;
    int split_at;
    int min_split;
    int stricter;
    int mark_forced;
    event_list_t *events;
    int buffer_opt;
    int ugly;
    int even_y;
    int autocrop;
    int pal_png;
    char* png_dir;
} ThreadData;

int ass_process_frames(ThreadData* arg);

int avs_process_frames(ThreadData* arg);

extern pthread_mutex_t mutex;
extern atomic_int num_of_events;
extern atomic_int stopFlag;

typedef void (*ProgressCallback)(int progress);
extern ProgressCallback progress_callback;

#endif // COMMON_H
