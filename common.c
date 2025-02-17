#include "common.h"
#include <string.h>

char *read_file_bytes(FILE *fp, size_t *bufsize)
{
	int res;
	long sz;
	long bytes_read;
	char *buf;
	res = fseek(fp, 0, SEEK_END);
	if (res == -1) {
		fclose(fp);
		return 0;
	}

	sz = ftell(fp);
	rewind(fp);

	buf = sz < SIZE_MAX ? malloc(sz + 1) : NULL;
	if (!buf) {
		fclose(fp);
		return NULL;
	}
	bytes_read = 0;
	do {
		res = fread(buf + bytes_read, 1, sz - bytes_read, fp);
		if (res <= 0) {
			fclose(fp);
			free(buf);
			return 0;
		}
		bytes_read += res;
	} while (sz - bytes_read > 0);
	buf[sz] = '\0';
	fclose(fp);

	if (bufsize)
		*bufsize = sz;
	return buf;
}

const char *detect_bom(const char *buf, const size_t bufsize)
{
	if (bufsize >= 4) {
		if (!strncmp(buf, "\xef\xbb\xbf", 3))
			return "UTF-8";
		if (!strncmp(buf, "\x00\x00\xfe\xff", 4))
			return "UTF-32BE";
		if (!strncmp(buf, "\xff\xfe\x00\x00", 4))
			return "UTF-32LE";
		if (!strncmp(buf, "\xfe\xff", 2))
			return "UTF-16BE";
		if (!strncmp(buf, "\xff\xfe", 2))
			return "UTF-16LE";
	}
	return "UTF-8";
}

void msg_callback(int level, const char *fmt, va_list va, void *data)
{
	if (level > (intptr_t)data)
		return;

	fprintf(stderr, "libass: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
}

int open_file_ass(char *psz_filename, ass_input_t **p_handle, stream_info_t *p_param)
{
	*p_handle = malloc(sizeof(ass_input_t));
	memset(*p_handle, 0, sizeof(ass_input_t));

	ASS_Renderer *ass_renderer;
	ASS_Library *ass_library = ass_library_init();

	if (!ass_library)
		return 1;

	ass_set_message_cb(ass_library, msg_callback, (void *)(intptr_t)0);

	ass_renderer = ass_renderer_init(ass_library);

	if (!ass_renderer)
		return 1;

	FILE *fp = fopen(psz_filename, "rb");
	if (!fp)
		return 1;
	size_t bufsize;
	char *buf = read_file_bytes(fp, &bufsize);
	const char* cs = detect_bom(buf, bufsize);
	ASS_Track *ass = ass_read_memory(ass_library, buf, bufsize, (char *)cs);

	if (!ass)
		return 1;

	(*p_handle)->ass_library = ass_library;
	(*p_handle)->ass_renderer = ass_renderer;
	(*p_handle)->ass = ass;

	p_param->i_height = ass->PlayResY;
	p_param->i_width = ass->PlayResX;

	return 0;
}

int get_frame_total_ass(ass_input_t *handle, stream_info_t *p_param)
{
	long long max_time_point = -1;
	for (int i = 0; i < handle->ass->n_events; ++i) {
		ASS_Event *evt = handle->ass->events + i;
		max_time_point = max(max_time_point, evt->Start + evt->Duration);
	}
	return (long double)max_time_point / p_param->i_fps_den * p_param->i_fps_num / 1000;
}

int close_file_ass(ass_input_t *handle)
{
	ass_renderer_done(handle->ass_renderer);
	ass_library_done(handle->ass_library);
	ass_free_track(handle->ass);
}

void get_dir_path(char *filename, char *dir_path)
{
	char abs_path[MAX_PATH + 1] = {0};
	char drive[3] = {0};
	char dir[MAX_PATH + 1] = {0};

	/* Get absolute path of output XML file */
	if (_fullpath(abs_path, filename, MAX_PATH) == NULL)
	{
		fprintf(stderr, "Cannot determine absolute path for: %s\n", filename);
		exit(1);
	}

	/* Split absolute path into components */
	_splitpath(abs_path, drive, dir, NULL, NULL);
	strncpy(dir_path, drive, 2);
	strncat(dir_path, dir, MAX_PATH - 2);

	if (strlen(dir_path) > MAX_PATH - 16)
	{
		fprintf(stderr, "Path for PNG files too long.\n");
		exit(1);
	}
}

void write_png(char *dir, int file_id, uint8_t *image, int w, int h, int graphic, uint32_t *pal, crop_t c)
{
	FILE *fh;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	png_colorp palette = NULL;
	png_bytep trans = NULL;
	char tmp[16] = {0};
	char filename[MAX_PATH + 1] = {0};
	char *col;
	int step = pal == NULL ? 4 : 1;
	int colors = 0;
	int i;

	snprintf(tmp, 15, "%08d_%d.png", file_id, graphic);
	strncpy(filename, dir, MAX_PATH);
	strncat(filename, tmp, 15);

	if ((fh = fopen(filename, "wb")) == NULL)
	{
		perror("Cannot open PNG file for writing");
		exit(1);
	}

	/* Initialize png struct */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		fprintf(stderr, "Cannot create png_ptr.\n");
		exit(1);
	}

	/* Initialize info struct */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fprintf(stderr, "Cannot create info_ptr.\n");
		exit(1);
	}

	/* Set long jump stuff (weird..?) */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fh);
		fprintf(stderr, "Error while writing PNG file: %s\n", filename);
		exit(1);
	}

	/* Initialize IO */
	png_init_io(png_ptr, fh);

	/* Set file info */
	if (pal == NULL)
		png_set_IHDR(png_ptr, info_ptr, c.w, c.h, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else
	{
		png_set_IHDR(png_ptr, info_ptr, c.w, c.h, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		palette = calloc(256, sizeof(png_color));
		trans = calloc(256, sizeof(png_byte));
		colors = 1;
		for (i = 1; i < 256 && pal[i]; i++)
		{
			col = (char *)&(pal[i]);
			palette[i].red = col[0];
			palette[i].green = col[1];
			palette[i].blue = col[2];
			trans[i] = col[3];
			colors++;
		}
		png_set_PLTE(png_ptr, info_ptr, palette, colors);
		png_set_tRNS(png_ptr, info_ptr, trans, colors, NULL);
	}

	/* Allocate row pointer memory */
	row_pointers = calloc(c.h, sizeof(png_bytep));

	/* Set row pointers */
	image = image + step * (c.x + w * c.y);
	for (i = 0; i < c.h; i++)
	{
		row_pointers[i] = image + i * w * step;
	}
	png_set_rows(png_ptr, info_ptr, row_pointers);

	/* Set compression */
	png_set_filter(png_ptr, 0, PNG_FILTER_VALUE_SUB);
	png_set_compression_level(png_ptr, 5);

	/* Write image */
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/* Free memory */
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(row_pointers);
	if (palette != NULL)
		free(palette);
	if (trans != NULL)
		free(trans);

	/* Close file handle */
	fclose(fh);
}

int is_identical(stream_info_t *s_info, char *img, char *img_old)
{
	uint32_t *max = (uint32_t *)(img + s_info->i_width * s_info->i_height * 4);
	uint32_t *im = (uint32_t *)img;
	uint32_t *im_old = (uint32_t *)img_old;

	while (im < max)
	{
		if (!((char *)im)[3])
			*im = 0;
		if (*(im++) ^ *(im_old++))
			return 0;
	}

	return 1;
}

int is_empty(stream_info_t *s_info, char *img)
{
	char *max = img + s_info->i_width * s_info->i_height * 4;
	char *im = img;

	while (im < max)
	{
		if (im[3])
			return 0;
		im += 4;
	}

	return 1;
}

void zero_transparent(stream_info_t *s_info, char *img)
{
	char *max = img + s_info->i_width * s_info->i_height * 4;
	char *im = img;

	while (im < max)
	{
		if (!im[3])
			*(uint32_t *)img = 0;
		im += 4;
	}
}

void swap_rb(stream_info_t *s_info, char *img, char *out)
{
	char *max = img + s_info->i_width * s_info->i_height * 4;

	while (img < max)
	{
		out[0] = img[2];
		out[1] = img[1];
		out[2] = img[0];
		out[3] = img[3];
		img += 4;
		out += 4;
	}
}

void mk_timecode(int frame, int fps, char *buf)
{
	int frames, s, m, h;
	int tc = frame;

	tc = frame;
	frames = tc % fps;
	tc /= fps;
	s = tc % 60;
	tc /= 60;
	m = tc % 60;
	tc /= 60;
	h = tc;

	if (h > 99)
	{
		fprintf(stderr, "Timecodes above 99:59:59:99 not supported: %u:%02u:%02u:%02u\n", h, m, s, frames);
		exit(1);
	}

	if (snprintf(buf, 12, "%02d:%02d:%02d:%02d", h, m, s, frames) != 11)
	{
		fprintf(stderr, "Timecode lead to invalid format: %s\n", buf);
		exit(1);
	}
}

void print_usage()
{
	fprintf(stderr,
		"ass2bdnxml 1.02\n\n"
		"Usage: ass2bdnxml [options] -o output input\n\n"
		"Input has to be an ass or a ssa subtitle file\n\n"
		"  -o, --output <string>        Output file in BDN XML format\n"
		"                               For SUP/PGS output, use a .sup extension\n"
		"  -j, --seek <integer>         Start processing at this frame, first is 0\n"
		"  -c, --count <integer>        Number of input frames to process\n"
		"  -t, --trackname <string>     Name of track, like: Undefined\n"
		"  -l, --language <string>      Language code, like: und\n"
		"  -v, --video-format <string>  Either of: 720p, 1080p, 2k, or with custom resolution like 720*480\n"
		"  -f, --fps <float>            Either of: 23.976, 24, 25, 30, 29.97, 50, 59.94, 60, or custom fps like 15/1\n"
		"  -x, --x-offset <integer>     X offset, for use with partial frames.\n"
		"  -y, --y-offset <integer>     Y offset, for use with partial frames.\n"
		"  -d, --t-offset <string>      Offset timecodes by this many frames or\n"
		"                               given non-drop timecode (HH:MM:SS:FF).\n"
		"  -s, --split-at <integer>     Split events longer than this, in frames.\n"
		"                               Disabled when 0, which is the default.\n"
		"  -m, --min-split <integer>    Minimum length of line segment after split.\n"
		"  -e, --even-y <integer>       Enforce even Y coordinates. [on=1, off=0]\n"
		"  -a, --autocrop <integer>     Automatically crop output. [on=1, off=0]\n"
		"  -p, --palette <integer>      Output 8bit palette PNG. [on=1, off=0]\n"
		"  -n, --null-xml <integer>     Allow output of empty XML files. [on=1, off=0]\n"
		"  -z, --stricter <integer>     Stricter checks in the SUP writer. May lead to\n"
		"                               less optimized buffer use, but might raise\n"
		"                               compatibility. [on=1, off=0]\n"
		"  -u, --ugly <integer>         Allow splitting images in ugly ways.\n"
		"                               Might improve buffer problems, but is ugly.\n"
		"                               [on=1, off=0]\n"
		"  -b, --buffer-opt <integer>   Optimize PG buffer size by image\n"
		"                               splitting. [on=1, off=0]\n\n"
		"  -g, --font-dir <string>      additional font dir for libass\n"
		"Example:\n"
		"  ass2bdnxml -t Undefined -l und -v 1080p -f 23.976 -a1 -p1 -b0 -m3 \\\n"
		"    -u0 -e0 -n0 -z0 -o output.xml input.ass\n"
		"  (Input and output are required settings. The rest are set to default.)\n"
		);
}

int is_extension(char *filename, char *check_ext)
{
	char *ext = strrchr(filename, '.');

	if (ext == NULL)
		return 0;

	ext++;
	if (!strcasecmp(ext, check_ext))
		return 1;

	return 0;
}

int parse_int(char *in, char *name, int *error)
{
	char *end;
	int r;
	errno = 0;
	if (error != NULL)
		*error = 0;
	r = strtol(in, &end, 10);
	if (errno || end == in || end != in + strlen(in))
	{
		if (error != NULL)
			*error = 1;
		if (name != NULL)
		{
			fprintf(stderr, "Error: Failed to parse integer (%s): %s\n", name, in);
			exit(1);
		}
	}
	return r;
}

int parse_tc(char *in, int fps)
{
	int r = 0;
	int e;
	int h, m, s, f;

	/* Test for raw frame number. */
	r = parse_int(in, NULL, &e);
	if (!e)
		return r;

	if (strlen(in) != 2 * 4 + 3 || in[2] != ':' || in[5] != ':' || in[8] != ':')
	{
		fprintf(stderr, "Error: Invalid timecode offset. Expected FRAMENUMBER or HH:MM:SS:FF, but got: %s\n", in);
		exit(1);
	}
	in[2] = 0;
	in[5] = 0;
	in[8] = 0;
	h = parse_int(in,     "t-offset hours",   NULL);
	m = parse_int(in + 3, "t-offset minutes", NULL);
	s = parse_int(in + 6, "t-offset seconds", NULL);
	f = parse_int(in + 9, "t-offset frames",  NULL);
	r = f;
	r += s * fps;
	fps *= 60;
	r += m * fps;
	fps *= 60;
	r += h * fps;
	return r;
}

void add_event_xml_real(event_list_t *events, int image, int start, int end, int graphics, crop_t *crops)
{
	event_t *new = calloc(1, sizeof(event_t));
	new->image_number = image;
	new->start_frame = start;
	new->end_frame = end;
	new->graphics = graphics;
	new->c[0] = crops[0];
	new->c[1] = crops[1];
	event_list_insert_after(events, new);
}

void add_event_xml(event_list_t *events, int split_at, int min_split, int start, int end, int graphics, crop_t *crops)
{
	int image = start;
	int d = end - start;

	if (!split_at)
		add_event_xml_real(events, image, start, end, graphics, crops);
	else
	{
		while (d >= split_at + min_split)
		{
			d -= split_at;
			add_event_xml_real(events, image, start, start + split_at, graphics, crops);
			start += split_at;
		}
		if (d)
			add_event_xml_real(events, image, start, start + d, graphics, crops);
	}
}

void write_sup_wrapper(sup_writer_t *sw, uint8_t *im, int num_crop, crop_t *crops, uint32_t *pal, int start, int end, int split_at, int min_split, int stricter)
{
	int d = end - start;

	if (!split_at)
		write_sup(sw, im, num_crop, crops, pal, start, end, stricter);
	else
	{
		while (d >= split_at + min_split)
		{
			d -= split_at;
			write_sup(sw, im, num_crop, crops, pal, start, start + split_at, stricter);
			start += split_at;
		}
		if (d)
			write_sup(sw, im, num_crop, crops, pal, start, start + d, stricter);
	}
}

void col2rgb(uint32_t *c, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = _r(*c);
	*g = _g(*c);
	*b = _b(*c);
}

void make_sub_img(ASS_Image *img, uint8_t *sub_img, uint32_t width)
{
	uint8_t c1, c2, c3, a, a1;
	uint8_t *src;
	uint8_t *dstC1, *dstC2, *dstC3, *dstA, *dst;
	uint32_t dsta;

	while (img) {
		if (img->w == 0 || img->h == 0) {
			// nothing to render
			img = img->next;
			continue;
		}
		col2rgb(&img->color, &c1, &c2, &c3);
		a1 = 255 - _a(img->color); // transparency

		src = img->bitmap;
		dst = sub_img + (img->dst_y * width + img->dst_x)*4;
		dstC1 = dst+2;
		dstC2 = dst+1;
		dstC3 = dst+0;
		dstA = dst+3;

		for (int i = 0; i < img->h; i++) {
			for (int j = 0; j < img->w*4; j+=4) {
				a = div255(src[j/4] * a1);
				if (a) {
					if (dstA[j]) {
						dsta = scale(a, 255, dstA[j]);
						dstC1[j] = dblend(a, c1, dstA[j], dstC1[j], dsta);
						dstC2[j] = dblend(a, c2, dstA[j], dstC2[j], dsta);
						dstC3[j] = dblend(a, c3, dstA[j], dstC3[j], dsta);
						dstA[j] = div255(dsta);
					}
					else {
						dstC1[j] = c1;
						dstC2[j] = c2;
						dstC3[j] = c3;
						dstA[j] = a;
					}
				}
			}

			src += img->stride;
			dstC1 += width * 4;
			dstC2 += width * 4;
			dstC3 += width * 4;
			dstA += width * 4;
		}

		img = img->next;
	}
}
