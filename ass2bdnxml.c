/*----------------------------------------------------------------------------
 * ass2bdnxml - Generates BluRay subtitle stuff from ass/ssa subtitles
 * based on avs2bdnxml 2.08
 * Copyright (C) 2008-2013 Arne Bochem <avs2bdnxml at ps-auxw de>
 * Copyright (C) 2022-2022 Masaiki <mydarer@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/
#include "common.h"
#include <pthread.h>
// codes from assrender end here

int main (int argc, char *argv[])
{
	struct framerate_entry_s framerates[] = { {"23.976", "23.976", 24, 0, 24000, 1001}
											/*, {"23.976d", "23.976", 24000/1001.0, 1}*/
											, {"24", "24", 24, 0, 24, 1}
											, {"25", "25", 25, 0, 25, 1}
											, {"29.97", "29.97", 30, 0, 30000, 1001}
											, {"30", "30", 30, 0, 30, 1}
											/*, {"29.97d", "29.97", 30000/1001.0, 1}*/
											, {"50", "50", 50, 0, 50, 1}
											, {"59.94", "59.94", 60, 0, 60000, 1001}
											, {"60", "60", 60, 0, 60, 1}
											/*, {"59.94d", "59.94", 60000/1001.0, 1}*/
											, {NULL, NULL, 0, 0, 0, 0}
											};
	char *ass_filename = NULL;
	char *track_name = "Undefined";
	char *language = "und";
	char *video_format = "1080p";
	char *frame_rate = "23.976";
	char *out_filename[2] = {NULL, NULL};
	char *sup_output_fn = NULL;
	char *xml_output_fn = NULL;
	char *x_offset = "0";
	char *y_offset = "0";
	char *t_offset = "0";
	char *buffer_optimize = "0";
	char *split_after = "0";
	char *minimum_split = "3";
	char *palletize_png = "1";
	char *even_y_string = "0";
	char *auto_crop_image = "1";
	char *ugly_option = "0";
	char *seek_string = "0";
	char *allow_empty_string = "0";
	char *stricter_string = "0";
	char *count_string = "2147483647";
	char * num_threads_string = "4";
	char *in_img = NULL, *old_img = NULL, *tmp = NULL, *out_buf = NULL;
	char *intc_buf = NULL, *outtc_buf = NULL;
	char *drop_frame = NULL;
	char png_dir[MAX_PATH + 1] = {0};
	const char *additional_font_dir = NULL;
	crop_t crops[2];
	pic_t pic;
	uint32_t *pal = NULL;
	int out_filename_idx = 0;
	int have_fps = 0;
	int n_crop = 1;
	int split_at = 0;
	int min_split = 3;
	int autocrop = 0;
	int xo, yo, to;
	int fps = 25;
	int count_frames = INT_MAX, last_frame;
	int init_frame = 0;
	int frames;
	int first_frame = -1, start_frame = -1, end_frame = -1;
	int num_of_events = 0;
	int i, c, j;
	int have_line = 0;
	int must_zero = 0;
	int checked_empty;
	int even_y = 0;
	int auto_cut = 0;
	int pal_png = 1;
	int ugly = 0;
	int progress_step = 1000;
	int buffer_opt;
	long long bench_start = time(NULL);
	int fps_num = 25, fps_den = 1;
	int sup_output = 0;
	int xml_output = 0;
	int allow_empty = 0;
	int stricter = 0;
	int num_threads = 4;
	stream_info_t *s_info = malloc(sizeof(stream_info_t));
	event_list_t *events = event_list_new();
	event_t *event;
	FILE *fh;

	/* Get args */
	if (argc < 2)
	{
		print_usage();
		return 0;
	}
	while (1)
	{
		static struct option long_options[] =
			{ {"output",       required_argument, 0, 'o'}
			, {"seek",         required_argument, 0, 'j'}
			, {"count",        required_argument, 0, 'c'}
			, {"trackname",    required_argument, 0, 't'}
			, {"language",     required_argument, 0, 'l'}
			, {"video-format", required_argument, 0, 'v'}
			, {"fps",          required_argument, 0, 'f'}
			, {"x-offset",     required_argument, 0, 'x'}
			, {"y-offset",     required_argument, 0, 'y'}
			, {"t-offset",     required_argument, 0, 'd'}
			, {"split-at",     required_argument, 0, 's'}
			, {"min-split",    required_argument, 0, 'm'}
			, {"autocrop",     required_argument, 0, 'a'}
			, {"even-y",       required_argument, 0, 'e'}
			, {"palette",      required_argument, 0, 'p'}
			, {"buffer-opt",   required_argument, 0, 'b'}
			, {"ugly",         required_argument, 0, 'u'}
			, {"null-xml",     required_argument, 0, 'n'}
			, {"stricter",     required_argument, 0, 'z'}
			, {"font-dir",     required_argument, 0, 'g'}
			, {"thread-num",   required_argument, 0, 'i'}
			, {0, 0, 0, 0}
			};
			int option_index = 0;

			c = getopt_long(argc, argv, "o:j:c:t:l:v:f:x:y:d:b:s:m:e:p:a:u:n:z:g:i:", long_options, &option_index);
			if (c == -1)
				break;
			switch (c)
			{
				case 'o':
					if (out_filename_idx < 2)
						out_filename[out_filename_idx++] = optarg;
					else
					{
						fprintf(stderr, "No more than two output filenames allowed.\nIf more than one is used, the other must have a\ndifferent output format.\n");
						exit(0);
					}
					break;
				case 'j':
					seek_string = optarg;
					break;
				case 'c':
					count_string = optarg;
					break;
				case 't':
					track_name = optarg;
					break;
				case 'l':
					language = optarg;
					break;
				case 'v':
					video_format = optarg;
					break;
				case 'f':
					frame_rate = optarg;
					break;
				case 'x':
					x_offset = optarg;
					break;
				case 'y':
					y_offset = optarg;
					break;
				case 'd':
					t_offset = optarg;
					break;
				case 'e':
					even_y_string = optarg;
					break;
				case 'p':
					palletize_png = optarg;
					break;
				case 'a':
					auto_crop_image = optarg;
					break;
				case 'b':
					buffer_optimize = optarg;
					break;
				case 's':
					split_after = optarg;
					break;
				case 'm':
					minimum_split = optarg;
					break;
				case 'u':
					ugly_option = optarg;
					break;
				case 'n':
					allow_empty_string = optarg;
					break;
				case 'z':
					stricter_string = optarg;
					break;
				case 'g':
					additional_font_dir = optarg;
					break;
				case 'i':
					num_threads_string = optarg;
					break;
				default:
					print_usage();
					return 0;
					break;
			}
	}
	if (argc - optind == 1)
		ass_filename = argv[optind];
	else
	{
		fprintf(stderr, "Only a single input file allowed.\n");
		return 1;
	}

	/* Both input and output filenames are required */
	if (ass_filename == NULL)
	{
		print_usage();
		return 0;
	}
	if (out_filename[0] == NULL)
	{
		print_usage();
		return 0;
	}

	memset(s_info, 0, sizeof(stream_info_t));

	/* Get target output format */
	for (i = 0; i < out_filename_idx; i++)
	{
		if (is_extension(out_filename[i], "xml"))
		{
			xml_output_fn = out_filename[i];
			xml_output++;
			get_dir_path(xml_output_fn, png_dir);
		}
		else if (is_extension(out_filename[i], "sup") || is_extension(out_filename[i], "pgs"))
		{
			sup_output_fn = out_filename[i];
			sup_output++;
			pal_png = 1;
		}
		else
		{
			fprintf(stderr, "Output file extension must be \".xml\", \".sup\" or \".pgs\".\n");
			return 1;
		}
	}
	if (sup_output > 1 || xml_output > 1)
	{
		fprintf(stderr, "If more than one output filename is used, they must have\ndifferent output formats.\n");
		exit(0);
	}

	/* Set X and Y offsets, and split value */
	xo = parse_int(x_offset, "x-offset", NULL);
	yo = parse_int(y_offset, "y-offset", NULL);
	pal_png = parse_int(palletize_png, "palette", NULL);
	even_y = parse_int(even_y_string, "even-y", NULL);
	autocrop = parse_int(auto_crop_image, "autocrop", NULL);
	split_at = parse_int(split_after, "split-at", NULL);
	ugly = parse_int(ugly_option, "ugly", NULL);
	allow_empty = parse_int(allow_empty_string, "null-xml", NULL);
	stricter = parse_int(stricter_string, "stricter", NULL);
	init_frame = parse_int(seek_string, "seek", NULL);
	count_frames = parse_int(count_string, "count", NULL);
	min_split = parse_int(minimum_split, "min-split", NULL);
	num_threads = parse_int(num_threads_string, "thread-num", NULL);
	if(num_threads <=0 || num_threads >16)
	{
		return -1;
	}
	ass_input_t *ass_context[num_threads];
	sup_writer_t *sw[num_threads];
	if (!min_split)
		min_split = 1;

	/* TODO: Sanity check video_format and frame_rate. */

	/* Get frame rate */
	i = 0;
	while (framerates[i].name != NULL)
	{
		if (!strcasecmp(framerates[i].name, frame_rate))
		{
			frame_rate = framerates[i].out_name;
			fps = framerates[i].rate;
			drop_frame = framerates[i].drop ? "true" : "false";
			s_info->i_fps_num = fps_num = framerates[i].fps_num;
			s_info->i_fps_den = fps_den = framerates[i].fps_den;
			have_fps = 1;
		}
		i++;
	}
	if (!have_fps)
	{
		if (sscanf(frame_rate, "%d/%d", &fps_num, &fps_den) == 2){
			drop_frame = "false";
			s_info->i_fps_num = fps_num;
			s_info->i_fps_den = fps_den;
			have_fps = 1;
		}
		fprintf(stderr, "Error: Invalid framerate (%s).\n", frame_rate);
		return 1;
	}

	/* Get timecode offset. */
	to = parse_tc(t_offset, fps);

	/* Detect CPU features
	detect_sse2();*/

	/* Get video info and allocate buffer */
	for(int i = 0; i < num_threads; i++)
	{
		if (open_file_ass(ass_filename, &ass_context[i], s_info))
		{
			print_usage();
			return 1;
		}
	}


	char *video_formats[] = { "2k","1440p","1080p","720p" };
	int video_format_widths[] = { 2560,2560,1920,1280 };
	int video_format_heights[] = { 1440,1440,1080,720 };
	int video_format_matched = 0;
	for (int ii = 0; ii < 4; ++ii) {
		if (!strcasecmp(video_format, video_formats[ii])) {
			s_info->i_width = video_format_widths[ii];
			s_info->i_height = video_format_heights[ii];
			video_format_matched = 1;
		}
	}
	if (video_format && !video_format_matched)
	{
		if (sscanf(video_format,"%d*%d", &s_info->i_width, &s_info->i_height) != 2){
			fprintf(stderr, "Error: Invalid video_format (%s).\n", video_format);
			return 1;
		}
	}

	for(int i = 0; i < num_threads; i++)
	{
		ass_set_storage_size(ass_context[i]->ass_renderer, s_info->i_width, s_info->i_height);
		ass_set_frame_size(ass_context[i]->ass_renderer, s_info->i_width, s_info->i_height);
	
		if (additional_font_dir)
			ass_set_fonts_dir(ass_context[i]->ass_library, additional_font_dir);
	
		ass_set_fonts(ass_context[i]->ass_renderer, NULL, NULL, ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
	}


	in_img  = calloc(s_info->i_width * s_info->i_height * 4 + 16 * 2, sizeof(char)); /* allocate + 16 for alignment, and + n * 16 for over read/write */
	old_img = calloc(s_info->i_width * s_info->i_height * 4 + 16 * 2, sizeof(char)); /* see above */
	out_buf = calloc(s_info->i_width * s_info->i_height * 4 + 16 * 2, sizeof(char));

	/* Check minimum size */
	if (s_info->i_width < 8 || s_info->i_height < 8)
	{
		fprintf(stderr, "Error: Video dimensions below 8x8 (%dx%d).\n", s_info->i_width, s_info->i_height);
		return 1;
	}

	/* Align buffers */
	in_img  = in_img + (short)(16 - ((long)in_img % 16));
	old_img = old_img + (short)(16 - ((long)old_img % 16));
	out_buf = out_buf + (short)(16 - ((long)out_buf % 16));

	/* Set up buffer (non-)optimization */
	buffer_opt = parse_int(buffer_optimize, "buffer-opt", NULL);
	pic.b = out_buf;
	pic.w = s_info->i_width;
	pic.h = s_info->i_height;
	pic.s = s_info->i_width;
	n_crop = 1;
	crops[0].x = 0;
	crops[0].y = 0;
	crops[0].w = pic.w;
	crops[0].h = pic.h;

	/* Get frame number */
	frames = get_frame_total_ass(ass_context[i], s_info);
	
	if (count_frames + init_frame > frames)
	{
		count_frames = frames - init_frame;
	}
	last_frame = count_frames + init_frame;

	/* No frames mean nothing to do */
	if (count_frames < 1)
	{
		fprintf(stderr, "No frames found.\n");
		return 0;
	}

	/* Set progress step */
	if (count_frames < 1000)
	{
		if (count_frames > 200)
			progress_step = 50;
		else if (count_frames > 50)
			progress_step = 10;
		else
			progress_step = 1;
	}

	int changed = 1;

	/* Process frames */
	for(int i = 0; i < num_threads; i++)
	{
			/* Open SUP writer, if applicable */
		if (sup_output)
		{
			sw[i] = new_sup_writer(sup_output_fn, pic.w, pic.h, fps_num, fps_den);
		}
		// ass_set_line_spacing(ass_context[i]->ass_renderer, -10);
	}
	
	    /* Process frames */
		int frames_per_thread = count_frames / num_threads;

		pthread_t threads[num_threads];
		ThreadData thread_data[num_threads];
	
		for (int i = 0; i < num_threads; i++) {
			thread_data[i].thread_id = i;
			thread_data[i].init_frame = init_frame + i * frames_per_thread;
			thread_data[i].last_frame = (i == num_threads - 1) ? last_frame : (thread_data[i].init_frame + frames_per_thread - 1);
			thread_data[i].ass_context = ass_context[i];
			thread_data[i].s_info = s_info;
			thread_data[i].have_line = have_line;
			thread_data[i].sup_output = sup_output;
			thread_data[i].xml_output = xml_output;
			thread_data[i].sw = sw[i];
			thread_data[i].n_crop = n_crop;
			thread_data[i].to = to;
			thread_data[i].split_at = split_at;
			thread_data[i].min_split = min_split;
			thread_data[i].stricter = stricter;
			thread_data[i].events = events;
			thread_data[i].buffer_opt = buffer_opt;
			thread_data[i].ugly = ugly;
			thread_data[i].even_y = even_y;
			thread_data[i].autocrop = autocrop;
			thread_data[i].pal_png = pal_png;
			thread_data[i].png_dir = png_dir;
			pthread_create(&threads[i], NULL, process_frames, &thread_data[i]);
		}
	
		for (int i = 0; i < num_threads; i++) {
			pthread_join(threads[i], NULL);
		}

	fprintf(stderr, "\rProgress: %d/%d - Lines: %d - Done\n", i - init_frame, count_frames, num_of_events);

	/* Add last event, if available */
	if (have_line)
	{
		if (sup_output)
		{
			assert(pal != NULL);
			write_sup_wrapper(sw, (uint8_t *)out_buf, n_crop, crops, pal, start_frame + to, i - 1 + to, split_at, min_split, stricter);
			if (!xml_output)
				free(pal);
			pal = NULL;
		}
		if (xml_output)
		{
			add_event_xml(events, split_at, min_split, start_frame + to, i - 1 + to, n_crop, crops);
			free(pal);
			pal = NULL;
		}
		auto_cut = 1;
		end_frame = i - 1;
	}

	for(int i = 0; i < num_threads; i++)
	{
		if (sup_output)
		{
			close_sup_writer(sw[i]);
		}
	}

	if (xml_output)
	{
		/* Check if we actually have any events */
		if (first_frame == -1)
		{
			if (!allow_empty)
			{
				fprintf(stderr, "No events detected. Cowardly refusing to write XML file.\n");
				return 0;
			}
			else
			{
				first_frame = 0;
				end_frame = 0;
			}
		}

		/* Initialize timecode buffers */
		intc_buf = calloc(12, 1);
		outtc_buf = calloc(12, 1);

		/* Creating output file */
		if ((fh = fopen(xml_output_fn, "w")) == 0)
		{
			perror("Error opening output XML file");
			return 1;
		}

		/* Write XML header */
		mk_timecode(first_frame + to, fps, intc_buf);
		mk_timecode(end_frame + to + auto_cut, fps, outtc_buf);
		fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<BDN Version=\"0.93\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"xsi:noNamespaceSchemaLocation=\"BD-03-006-0093b BDN File Format.xsd\">\n"
			"<Description>\n"
			"<Name Title=\"%s\" Content=\"\"/>\n"
			"<Language Code=\"%s\"/>\n"
			"<Format VideoFormat=\"%s\" FrameRate=\"%s\" DropFrame=\"%s\"/>\n"
			"<Events LastEventOutTC=\"%s\" FirstEventInTC=\"%s\"\n", track_name, language, video_format, frame_rate, drop_frame, outtc_buf, intc_buf);

		mk_timecode(0, fps, intc_buf);
		mk_timecode(frames + to, fps, outtc_buf);
		fprintf(fh, "ContentInTC=\"%s\" ContentOutTC=\"%s\" NumberofEvents=\"%d\" Type=\"Graphic\"/>\n"
			"</Description>\n"
			"<Events>\n", intc_buf, outtc_buf, num_of_events);

		/* Write XML events */
		if (!event_list_empty(events))
		{
			event = event_list_first(events);
			do
			{
				mk_timecode(event->start_frame, fps, intc_buf);
				mk_timecode(event->end_frame, fps, outtc_buf);
				
				if (auto_cut && event->end_frame == frames - 1)
				{
					mk_timecode(event->end_frame + 1, fps, outtc_buf);
				}
				
				fprintf(fh, "<Event Forced=\"False\" InTC=\"%s\" OutTC=\"%s\">\n", intc_buf, outtc_buf);
				for (i = 0; i < event->graphics; i++)
				{
					fprintf(fh, "<Graphic Width=\"%d\" Height=\"%d\" X=\"%d\" Y=\"%d\">%08d_%d.png</Graphic>\n", event->c[i].w, event->c[i].h, xo + event->c[i].x, yo + event->c[i].y, event->image_number - to, i);
				}
				fprintf(fh, "</Event>\n");
				event = event_list_next(events);
			}
			while (event != NULL);
		}

		/* Write XML footer */
		fprintf(fh, "</Events>\n</BDN>\n");

		/* Close XML file */
		fclose(fh);
	}

	/* Cleanup */
	for(int i = 0; i < num_threads; i++)
	{
		close_file_ass(ass_context[i]);
	}

	/* Give runtime */
	fprintf(stderr, "Time elapsed: %lld\n", time(NULL) - bench_start);

	return 0;
}

