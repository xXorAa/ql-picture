#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef MAGICK_V6
#include <wand/MagickWand.h>
#else
#include <MagickWand/MagickWand.h>
#endif

#include "ql-palette.h"

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
	MagickWand *sqlux_wand = NULL, *sqlux_remap_wand = NULL;
	MagickWand *ql_wand = NULL, *ql_remap_wand = NULL;
	PixelIterator *pixel_iter = NULL;
	PixelWand **pixels = NULL;
	size_t num_pixels;
	int i, j, qdiv, pix_rgb;
	uint8_t green = 0, redblue = 0;
	FILE *sqlux_scr, *ql_scr;

	basename = remove_file_ext(argv[1]);

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

	MagickReadImage(ql_wand, argv[1]);
	MagickReadImageBlob(ql_remap_wand, ql_palette_mode8, sizeof(ql_palette_mode8));

	/* API incompatablity between v6 and v7 */
#ifdef MAGICK_V6
	MagickResizeImage(ql_wand, 256, 256, LanczosFilter, 1);
#else
	MagickResizeImage(ql_wand, 256, 256, LanczosFilter);
#endif
	MagickRemapImage(ql_wand, ql_remap_wand, FloydSteinbergDitherMethod);

	strncpy(temp_path, basename, PATH_MAX);
	strncat(temp_path, "_ql.png", PATH_MAX);

	MagickWriteImage(ql_wand, temp_path);

	pixel_iter = NewPixelIterator(ql_wand);
	pixels = PixelGetNextIteratorRow(pixel_iter, &num_pixels);

	strncpy(temp_path, basename, PATH_MAX);
	strncat(temp_path, "_scr", PATH_MAX);

	ql_scr = fopen(temp_path, "w");

	for (i=0; pixels != (PixelWand **) NULL; i++)
	{
		for (j = 0; j < num_pixels; j++){

			green <<= 2;
			redblue <<= 2;

			pix_rgb = (int)PixelGetRedQuantum(pixels[j])/qdiv;
			pix_rgb <<= 8;
			pix_rgb |= (int)PixelGetGreenQuantum(pixels[j])/qdiv;
			pix_rgb <<= 8;
			pix_rgb |= (int)PixelGetBlueQuantum(pixels[j])/qdiv;

			switch(pix_rgb) {
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
			}

			if (((j + 1) % 4) == 0) {
				fwrite(&green, 1, 1, ql_scr);
				fwrite(&redblue, 1, 1, ql_scr);
			}
		}

		pixels=PixelGetNextIteratorRow(pixel_iter, &num_pixels);
	}

	fclose(ql_scr);

	/* Clean up */
	if(ql_wand)
		ql_wand = DestroyMagickWand(ql_wand);
	if(ql_remap_wand)
		ql_remap_wand = DestroyMagickWand(ql_remap_wand);

	MagickWandTerminus();
}
