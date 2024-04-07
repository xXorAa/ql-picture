#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef MAGICK_V6
#include <wand/MagickWand.h>
#else
#include <MagickWand/MagickWand.h>
#endif

#include "args.h"
#include "ql-palette.h"

#define QL_FORMAT_SCR 's'
#define QL_FORMAT_PIC 'p'

char *remove_file_ext(char* file_str)
{
	char *ret_str, *lastext, *lastpath;

	if (file_str == NULL)
		return NULL;
	if ((ret_str = malloc(strlen(file_str) + 1)) == NULL)
		return NULL;

	// Make a copy and find the relevant characters.

	strcpy(ret_str, file_str);
	lastext = strrchr(ret_str, '.');
	lastpath = strrchr(ret_str, '/');

	if (lastext != NULL)
	{
		if (lastpath != NULL)
		{
			if (lastpath < lastext)
			{
				*lastext = '\0';
			}
		}
		else
		{
			*lastext = '\0';
		}
	}
	return ret_str;
}

int main(int argc, char **argv)
{
	char temp_path[PATH_MAX + 1];
	char *basename;
	char *filename;
	MagickWand *sqlux_wand = NULL, *sqlux_remap_wand = NULL;
	MagickWand *ql_wand = NULL, *ql_remap_wand = NULL;
	PixelIterator *pixel_iter = NULL;
	PixelWand **pixels = NULL;
	size_t num_pixels;
	int i, j, qdiv, pix_rgb, mode = 8, format = QL_FORMAT_SCR;
	uint8_t green = 0, redblue = 0, byte1 = 0, byte2 = 0;
	FILE *sqlux_scr, *ql_scr;

    ArgParser* parser = ap_new_parser();
    if (!parser) {
        exit(1);
    }

    // Register the program's helptext and version number.
    ap_set_helptext(parser, "Usage: ql-picture [OPTIONS] <file>\n\
\n\
Options:\n\
  -h,--help            Print this help message and exit\n\
  -m,--mode            QL mode 4/8 default(8)\n\
  -f,--format          format pic/scr default(scr)\n\
  -v,--version         version\n");

    ap_set_version(parser, "1.0");

    // Register a flag and a string-valued option.
    ap_add_int_opt(parser, "mode m", 8);
    ap_add_str_opt(parser, "format f", "scr");

    // Parse the command line arguments.
    if (!ap_parse(parser, argc, argv)) {
        exit(1);
    }

	if (ap_count_args(parser) != 1) {
		printf("Needs <file>\n");
		printf("%s\n", ap_get_helptext(parser));
		exit(1);
	}

	filename = ap_get_arg_at_index(parser, 0);

	mode = ap_get_int_value(parser, "mode");

	if (!((mode == 4) || (mode == 8))) {
		printf("Mode must be 4 or 8\n");
		exit(1);
	}

	if (strcasecmp("scr", ap_get_str_value(parser, "format")) == 0) {
		format = QL_FORMAT_SCR;
	} else if (strcasecmp("pic", ap_get_str_value(parser, "format")) == 0) {
		format = QL_FORMAT_PIC;
	} else {
		printf("Invalid format %s\n", ap_get_str_value(parser, "format"));
		exit(1);
	}

	basename = remove_file_ext(filename);

	switch (MAGICKCORE_QUANTUM_DEPTH) {
	case 8:
		qdiv = 1;
		break;
	case 16:
		qdiv = 256;
		break;
	default:
		fprintf(stderr, "Unsupported Quantum %d", MAGICKCORE_QUANTUM_DEPTH);
		exit(-1);
	}

	MagickWandGenesis();

	ql_wand = NewMagickWand();
	ql_remap_wand = NewMagickWand();

	MagickReadImage(ql_wand, filename);

	switch(mode) {
	case 8:
		MagickReadImageBlob(ql_remap_wand, ql_palette_mode8, sizeof(ql_palette_mode8));

		/* API incompatablity between v6 and v7 */
#ifdef MAGICK_V6
		MagickResizeImage(ql_wand, 256, 256, LanczosFilter, 1);
#else
		MagickResizeImage(ql_wand, 256, 256, LanczosFilter);
#endif
		MagickRemapImage(ql_wand, ql_remap_wand, FloydSteinbergDitherMethod);
		break;
	case 4:
		MagickReadImageBlob(ql_remap_wand, ql_palette_mode4, sizeof(ql_palette_mode4));

		/* API incompatablity between v6 and v7 */
#ifdef MAGICK_V6
		MagickResizeImage(ql_wand, 512, 256, LanczosFilter, 1);
#else
		MagickResizeImage(ql_wand, 512, 256, LanczosFilter);
#endif
		MagickRemapImage(ql_wand, ql_remap_wand, FloydSteinbergDitherMethod);
		break;
	}

	strncpy(temp_path, basename, PATH_MAX);
	strncat(temp_path, "_ql.png", PATH_MAX);

	MagickWriteImage(ql_wand, temp_path);

	pixel_iter = NewPixelIterator(ql_wand);
	pixels = PixelGetNextIteratorRow(pixel_iter, &num_pixels);

	strncpy(temp_path, basename, PATH_MAX);

	if (format == QL_FORMAT_SCR) {
		strncat(temp_path, "_scr", PATH_MAX);
	} else {
		strncat(temp_path, "_pic", PATH_MAX);
	}

	ql_scr = fopen(temp_path, "w");

	if (format == QL_FORMAT_PIC) {
		fputc(0x4A, ql_scr);
		fputc(0xFC, ql_scr);
		if (mode == 4) {
			// X 512
			fputc(0x02, ql_scr);
			fputc(0x00, ql_scr);
		} else {
			// X 256
			fputc(0x01, ql_scr);
			fputc(0x00, ql_scr);
		}
		// Y 256
		fputc(0x01, ql_scr);
		fputc(0x00, ql_scr);

		// bytes 128
		fputc(0x00, ql_scr);
		fputc(0x80, ql_scr);

		fputc(mode, ql_scr);
		fputc(0, ql_scr);
	}

	switch (mode) {
	case 4:
		for (i = 0; pixels != (PixelWand **)NULL; i++) {
			for (j = 0; j < num_pixels; j++) {
				byte1 <<= 1;
				byte2 <<= 1;

				pix_rgb = (int)PixelGetRedQuantum(pixels[j]) /
					  qdiv;
				pix_rgb <<= 8;
				pix_rgb |=
					(int)PixelGetGreenQuantum(pixels[j]) /
					qdiv;
				pix_rgb <<= 8;
				pix_rgb |= (int)PixelGetBlueQuantum(pixels[j]) /
					   qdiv;

				switch (pix_rgb) {
				case 0xFFFFFF:
					byte1 |= 0x01;
					byte2 |= 0x01;
					break;
				case 0x000000:
					break;
				case 0xFF0000:
					byte2 |= 0x01;
					break;
				case 0x00FF00:
					byte1 |= 0x01;
					break;
				default:
					printf("Unknown RGB %06x\n", pix_rgb);
				}

				if (((j + 1) % 8) == 0) {
					fwrite(&byte1, 1, 1, ql_scr);
					fwrite(&byte2, 1, 1, ql_scr);
				}
			}

			pixels = PixelGetNextIteratorRow(pixel_iter,
							 &num_pixels);
		}
		break;
	case 8:
		for (i = 0; pixels != (PixelWand **)NULL; i++) {
			for (j = 0; j < num_pixels; j++) {
				green <<= 2;
				redblue <<= 2;

				pix_rgb = (int)PixelGetRedQuantum(pixels[j]) /
					  qdiv;
				pix_rgb <<= 8;
				pix_rgb |=
					(int)PixelGetGreenQuantum(pixels[j]) /
					qdiv;
				pix_rgb <<= 8;
				pix_rgb |= (int)PixelGetBlueQuantum(pixels[j]) /
					   qdiv;

				switch (pix_rgb) {
				case 0xFFFFFF:
					green |= 0x02;
					redblue |= 0x03;
					break;
				case 0x000000:
					break;
				case 0xFF0000:
					redblue |= 0x02;
					break;
				case 0xFFFF00:
					green |= 0x02;
					redblue |= 0x02;
					break;
				case 0xFF00FF:
					redblue |= 0x03;
					break;
				case 0x00FF00:
					green |= 0x02;
					break;
				case 0x00FFFF:
					green |= 0x02;
					redblue |= 0x01;
					break;
				case 0x0000FF:
					redblue |= 0x01;
					break;
				default:
					printf("Unknown RGB %06x\n", pix_rgb);
				}

				if (((j + 1) % 4) == 0) {
					fwrite(&green, 1, 1, ql_scr);
					fwrite(&redblue, 1, 1, ql_scr);
				}
			}

			pixels = PixelGetNextIteratorRow(pixel_iter,
							 &num_pixels);
		}
		break;
	}

	fclose(ql_scr);

exit:
	/* Clean up */
	if(ql_wand)
		ql_wand = DestroyMagickWand(ql_wand);
	if(ql_remap_wand)
		ql_remap_wand = DestroyMagickWand(ql_remap_wand);

	MagickWandTerminus();

	ap_free(parser);
}
