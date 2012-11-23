/* swf_dump.c -- written by Alexis WILKE for Made to Order Software Corp. (c) 2002-2009 */

/*

Copyright (c) 2002-2009 Made to Order Software Corp.

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/


/*
 * This simple tool can be used to dump out a list
 * of tags from a .swf file.
 */

#include	<stdlib.h>
#include	<stdio.h>
#ifndef _MSVC
#include	<unistd.h>
#endif
#include	<string.h>
#include	<errno.h>
#include	<limits.h>
#include	<math.h>
#include	<zlib.h>

#include	"sswf/libsswf-config.h"


#ifdef _MSVC
#define	snprintf		_snprintf
#endif


const char *version = TO_STR(SSWF_VERSION);



/* the following are offsets to write in the header of a targa image file format */
enum targa_header_t {
	TGA_OFFSET_IDENTIFIER_LENGTH = 0,
	TGA_OFFSET_COLORMAP_TYPE,
	TGA_OFFSET_IMAGE_TYPE,			/* includes compression flag */
	TGA_OFFSET_COLORMAP_INDEX_LO,
	TGA_OFFSET_COLORMAP_INDEX_HI,
	TGA_OFFSET_COLORMAP_LENGTH_LO,
	TGA_OFFSET_COLORMAP_LENGTH_HI,
	TGA_OFFSET_COLORMAP_SIZE,
	TGA_OFFSET_ORIGIN_X_LO,
	TGA_OFFSET_ORIGIN_X_HI,
	TGA_OFFSET_ORIGIN_Y_LO,
	TGA_OFFSET_ORIGIN_Y_HI,
	TGA_OFFSET_WIDTH_LO,
	TGA_OFFSET_WIDTH_HI,
	TGA_OFFSET_HEIGHT_LO,
	TGA_OFFSET_HEIGHT_HI,
	TGA_OFFSET_BITS_PER_PIXEL,
	TGA_OFFSET_FLAGS,

	TGA_HEADER_SIZE			/* the size of the targa files header */
};




typedef struct SWF_DUMP_FLAGS {
	long		verbose;		/* level of verbosity (default to 1, 0 = quiet) */
	long		decompress;		/* decompress images and save them in a .ppm file */
	long		movie_header;		/* whether we should simply write the movie sizes and exit */
	long		tga24;			/* always remove the alpha channel */
	long		object;			/* the object ID to dump - nothing else is dumped */
} swf_dump_flags;



#define	SWF_DUMP_BUFSIZ		4096			/* buffer size for uncompressed movies */

typedef struct SWF_DUMP_STATE {
	const char *	filename;
	FILE *		file;
	swf_dump_flags *flags;
	unsigned char	header[16];			/* Magic, Version + Size */
	unsigned long	jpeg_size;			/* size of the jpeg_tables */
	char *		jpeg_tables;			/* this is useful to save the following JPEG V1.0 */

	long		bufpos;				/* current read position (in bits) in the buffer */
	long		last_byte;			/* last byte read, we keep it because there may be bits we didn't use yet */
	long		max_bytes;			/* last fread() result */
	long		next_byte;			/* what byte to read on the following getbyte() call */
	long		file_offset;			/* the offset within the entire file */
	long		buf_size;			/* current maximum size of the buffer */
	unsigned char *	buffer;				/* a buffer of preloaded data */

	long		fill_bits;			/* # of bits to indicate a fill # */
	long		line_bits;			/* # of bits to indicate a line # */
} swf_dump_state;

typedef struct SWF_DUMP_RECT {
	long		x_min;
	long		x_max;
	long		y_min;
	long		y_max;
} swf_dump_rect;

typedef struct SWF_DUMP_TAG {
	long		tag;
	long		size;
} swf_dump_tag;


int scan(swf_dump_state *state);
int scan_shape(swf_dump_state *state, long max_styles, long use_rgba, long two_objects, long new_styles, long shape4);




int getrawbyte(swf_dump_state *state)
{
	if(state->next_byte >= state->max_bytes) {
		/* TODO? we could read the entire file at once also... */
		if(state->buffer == 0) {
			state->buffer = malloc(SWF_DUMP_BUFSIZ);
			if(state->buffer == NULL) {
				fflush(stdout);
				fprintf(stderr, "ERROR: out of memory.\n");
				exit(1);
			}
			state->buf_size = SWF_DUMP_BUFSIZ;
		}
		state->max_bytes = fread(state->buffer, 1, state->buf_size, state->file);
		if(state->max_bytes <= 0) {
			fflush(stdout);
			fprintf(stderr, "ERROR: error while reading file \"%s\" (%d).\n", state->filename, errno);
			return -1;
		}
		/* restart our counter */
		state->next_byte = 0;
	}

	/* get the next byte now */
	state->last_byte = state->buffer[state->next_byte];
	state->next_byte++;
	state->file_offset++;

	return 0;
}


int getdata(swf_dump_state *state, void *buffer, long size)
{
#define	BUFFER		((unsigned char *) buffer)

	/* align the bit position */
	state->bufpos = (state->bufpos + 7) & -8;

	/* TODO: to accelerate this, we should understand the lower level and copy a block at once instead */
	while(size > 0) {
		if(getrawbyte(state) != 0) {
			return -1;
		}
		BUFFER[0] = (unsigned char) state->last_byte;
		buffer = BUFFER + 1;
		size--;
		state->bufpos += 8;
	}

	return 0;
}


int skipdata(swf_dump_state *state, long size)
{
	/* align the bit position */
	state->bufpos = (state->bufpos + 7) & -8;

	/* TODO: to accelerate this, we should understand the lower level and copy a block at once instead */
	while(size > 0) {
		if(getrawbyte(state) != 0) {
			return -1;
		}
		size--;
		state->bufpos += 8;
	}

	return 0;
}


int getbits(swf_dump_state *state, long bit_size, long *result, long is_signed)
{
	long		r;
	unsigned long	bit, sign, mask;

	if(bit_size <= 0) {
		*result = 0;
		return 0;
	}

	/* TODO: the following should use shifts to be faster! */
	sign = bit = 1 << (bit_size - 1);
	mask = 1 << (7 - (state->bufpos & 7));
	state->bufpos += bit_size;
	r = 0;
	while(bit != 0) {
		if(mask == 128) {
			if(getrawbyte(state) != 0) {
				return -1;
			}
		}
		if((state->last_byte & mask) != 0) {
			r += bit;
		}
		mask >>= 1;
		if(mask == 0) {
			mask = 128;
		}
		bit >>= 1;
	}

	if(is_signed && (r & sign) != 0) {
		r |= - (long) sign;
	}

	*result = r;

	return 0;
}



int getbyte(swf_dump_state *state, long *value)
{
	unsigned char	c[1];

	if(getdata(state, c, 1) != 0) {
		return -1;
	}

	*value = c[0];

	return 0;
}


int getword(swf_dump_state *state, long *value)
{
	unsigned char	c[2];

	if(getdata(state, c, 2) != 0) {
		return -1;
	}

	*value = c[0] + (c[1] << 8);

	return 0;
}


int getlong(swf_dump_state *state, long *value)
{
	unsigned char	c[4];

	if(getdata(state, c, 4) != 0) {
		return -1;
	}

	*value = c[0] + (c[1] << 8) + (c[2] << 16) + (c[3] << 24);

	return 0;
}




int getfixed(swf_dump_state *state, long byte_size, double *value, long decimals)
{
	unsigned char	c[4];

	c[1] = c[2] = c[3] = 0;		/* quick clear in case byte_size != 4 */
	if(getdata(state, c, byte_size) != 0) {
		return -1;
	}

	*value = (double) (c[0] + (c[1] << 8) + (c[2] << 16) + (c[3] << 24)) / (double) (1 << decimals);

	return 0;
}
#define	getfixed8_8(state, value)	getfixed(state, 4, value, 16)


int getbitsfixed(swf_dump_state *state, long bits_size, double *value, long decimals, long is_signed)
{
	long		r;

	if(getbits(state, bits_size, &r, is_signed) != 0) {
		return -1;
	}

	*value = (double) r / (double) (1 << decimals);

	return 0;
}




int getrect(swf_dump_state *state, swf_dump_rect *rect)
{
	long		size;

	/* get the size of each parameter */
	if(getbits(state, 5, &size, 0) != 0) {
		return -1;
	}

	/* get the actual positions of the rectangle */
	if(getbits(state, size, &rect->x_min, 1) != 0) {
		return -1;
	}
	if(getbits(state, size, &rect->x_max, 1) != 0) {
		return -1;
	}
	if(getbits(state, size, &rect->y_min, 1) != 0) {
		return -1;
	}
	if(getbits(state, size, &rect->y_max, 1) != 0) {
		return -1;
	}

	return 0;
}


/* read one RGB or RGBA entry - if alpha is a NULL pointer, read RGB only */
int getcolor(swf_dump_state *state, long *red, long *green, long *blue, long *alpha)
{
	if(getbyte(state, red) != 0) {
		return -1;
	}
	if(getbyte(state, green) != 0) {
		return -1;
	}
	if(getbyte(state, blue) != 0) {
		return -1;
	}
	if(alpha != NULL) {
		if(getbyte(state, alpha) != 0) {
			return -1;
		}
	}

	return 0;
}


int getstring(swf_dump_state *state, char **str)
{
	char		*r;
	long		max, p, c;

	*str = NULL;

	r = malloc(256);
	if(r == NULL) {
		fflush(stdout);
		fprintf(stderr, "ERROR: out of memory.\n");
		return -1;
	}
	max = 256;
	p = 0;
	do {
		if(getbyte(state, &c) != 0) {
			return -1;
		}
		if(p >= max) {
			max += 256;
			r = realloc(r, max);
			if(r == NULL) {
				fflush(stdout);
				fprintf(stderr, "ERROR: out of memory.\n");
				return -1;
			}
		}
		r[p] = (char) c;
		p++;
	} while(c != 0);

	*str = r;

	return 0;
}



int gettag(swf_dump_state *state, swf_dump_tag *tag)
{
	long		value;

	if(getword(state, &value) != 0) {
		return -1;
	}

	tag->tag = value >> 6;

	tag->size = value & 0x3F;
	if(tag->size == 0x3F) {		/* long size! */
		if(getlong(state, &tag->size) != 0) {
			return -1;
		}
	}

	return 0;
}




int scan_unknown(swf_dump_state *state, swf_dump_tag *tag)
{
	long		i, j, byte;
	char		line[16];

	for(i = 0; i < tag->size; i++) {
		if((i & 15) == 0) {
			/* print a new "address"! (offset within that tag) */
			printf("  %08lX- ", i);
		}
		if(getbyte(state, &byte) != 0) {
			return -1;
		}
		printf(" %02lX", byte);
		line[i & 15] = (char) byte;
		if((i & 15) == 15) {
			/* print the ASCII now */
			printf("        ");
			for(j = 0; j < 16; j++) {
				byte = line[j];
				if(byte < ' ' || (byte >= 0x7F && byte <= 0xBF)) {
					byte = '.';
				}
				printf("%c", (char) byte);
			}
			printf("\n");
		}
	}
	if((i & 15) != 0) {
		/* we didn't print the ASCII yet */
		j = i;
		do {
			printf("   ");
			j++;
		} while((j & 15) != 0);
		printf("        ");
		i &= 15;
		for(j = 0; j < i; j++) {
			byte = line[j];
			if(byte < ' ' || (byte >= 0x7F && byte <= 0xBF)) {
				byte = '.';
			}
			printf("%c", (char) byte);
		}
		printf("\n");
	}

	return 0;
}
#define	scan_undefined(state, tag) printf("  TODO: this tag is known but not yet understood.\n"), scan_unknown(state, tag)


int scan_matrix(swf_dump_state *state, const char *indent)
{
	long		flag, bits, value;
	double		fixed;

	/* align the read pointer to next byte (probably not necessary but just in case...) */
	getdata(state, NULL, 0);

	if(getbits(state, 1, &flag, 0) != 0) {
		return -1;
	}

	if(flag) {		/* has scale */
		if(getbits(state, 5, &bits, 0) != 0) {
			return -1;
		}

		if(getbitsfixed(state, bits, &fixed, 16, 1) != 0) {
			return -1;
		}
		printf("%sScale X: %g\n", indent, fixed);

		if(getbitsfixed(state, bits, &fixed, 16, 1) != 0) {
			return -1;
		}
		printf("%sScale Y: %g\n", indent, fixed);
	}

	if(getbits(state, 1, &flag, 0) != 0) {
		return -1;
	}

	if(flag) {		/* has rotate */
		if(getbits(state, 5, &bits, 0) != 0) {
			return -1;
		}

		if(getbitsfixed(state, bits, &fixed, 16, 1) != 0) {
			return -1;
		}
		printf("%sRotate Skew 0: %g\n", indent, fixed);

		if(getbitsfixed(state, bits, &fixed, 16, 1) != 0) {
			return -1;
		}
		printf("%sRotate Skew 1: %g\n", indent, fixed);
	}

	if(getbits(state, 5, &bits, 0) != 0) {
		return -1;
	}

	if(getbits(state, bits, &value, 1) != 0) {
		return -1;
	}
	printf("%sTranslate X: %g\n", indent, (double) value);

	if(getbits(state, bits, &value, 1) != 0) {
		return -1;
	}
	printf("%sTranslate Y: %ld\n", indent, value);

	return 0;
}


int scan_color_transform(swf_dump_state *state, const char *indent, long use_rgba)
{
	long		has_add, has_mult, size, red, green, blue, alpha;

	/* align the read pointer to next byte (this IS necessary) */
	getdata(state, NULL, 0);

	if(getbits(state, 1, &has_add, 0) != 0) {
		return -1;
	}
	if(getbits(state, 1, &has_mult, 0) != 0) {
		return -1;
	}
	if(getbits(state, 4, &size, 0) != 0) {
		return -1;
	}

	if(has_mult) {
		if(getbits(state, size, &red, 0) != 0) {
			return -1;
		}
		if(getbits(state, size, &green, 0) != 0) {
			return -1;
		}
		if(getbits(state, size, &blue, 0) != 0) {
			return -1;
		}
		if(use_rgba) {
			if(getbits(state, size, &alpha, 0) != 0) {
				return -1;
			}
			printf("%sRGBA Factors: (%ld, %ld, %ld, %ld)\n", indent, red, green, blue, alpha);
		}
		else {
			printf("%sRGB Factors: (%ld, %ld, %ld)\n", indent, red, green, blue);
		}
	}

	if(has_add) {
		if(getbits(state, size, &red, 0) != 0) {
			return -1;
		}
		if(getbits(state, size, &green, 0) != 0) {
			return -1;
		}
		if(getbits(state, size, &blue, 0) != 0) {
			return -1;
		}
		if(use_rgba) {
			if(getbits(state, size, &alpha, 0) != 0) {
				return -1;
			}
			printf("%sRGBA Translate: (%ld, %ld, %ld, %ld)\n", indent, red, green, blue, alpha);
		}
		else {
			printf("%sRGB Translate: (%ld, %ld, %ld)\n", indent, red, green, blue);
		}
	}

	if(!has_add && !has_mult) {
		printf("%sNo color matrix.\n", indent);
	}

	return 0;
}



int scan_gradient(swf_dump_state *state, const char *indent, long use_rgba, long two_objects, long has_focal)
{
	long		count, spread, interpolate, red, green, blue, alpha, position, focal;
	static const char *spread_modes[4] = {
		"PAD", "REFLECT", "REPEAT", "undefined3"
	};
	static const char *interpolate_modes[4] = {
		"NORMAL", "LINEAR", "undefined2", "undefined3"
	};

	if(getbyte(state, &count) != 0) {
		return -1;
	}

	spread = (count >> 5) & 3;
	interpolate = (count >> 4) & 3;
	count &= 15;

	/* the maximum is automatically 15 anyway */
	if(count < 1 /*|| count > 15*/) {
		fflush(stdout);
		fprintf(stderr, "ERROR: invalid count of %ld (limit is 1 to 15)\n", count);
		return -1;
	}

	printf("%sSpread mode: %s\n", indent, spread_modes[spread]);
	printf("%sInterpolation: %s\n", indent, interpolate_modes[interpolate]);
	printf("%s%ld gradient colors:\n", indent, count);

	/* in case we remove the previous test otherwise a 'do { ... } while()' would also work. */
	while(count > 0) {
		count--;
		if(getbyte(state, &position) != 0) {
			return -1;
		}
		printf("%s  Position: %ld (%g%%)\n", indent, position, (double) (position * 100) / 255.0);
		if(use_rgba) {
			if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
				return -1;
			}
			printf("%s  RGBA: (%ld, %ld, %ld, %ld)\n", indent, red, green, blue, alpha);
		}
		else {
			if(getcolor(state, &red, &green, &blue, NULL) != 0) {
				return -1;
			}
			printf("%s  RGB: (%ld, %ld, %ld)\n", indent, red, green, blue);
		}
		if(two_objects) {
			if(getbyte(state, &position) != 0) {
				return -1;
			}
			printf("%s  Position to morph to: %ld (%g%%)\n", indent, position, (double) (position * 100) / 255.0);

			if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
				return -1;
			}
			printf("%s  RGBA to morph to: (%ld, %ld, %ld, %ld)\n", indent, red, green, blue, alpha);
		}
	}

	if(has_focal) {
		if(getword(state, &focal) != 0) {
			return -1;
		}
		printf("%sFocal Point: %.3g (0x%04lX)\n", indent, (float) focal / 256.0f, focal);
	}

	return 0;
}



int scan_color(swf_dump_state *state, const char *info, long size)
{
	long		i, color;

	if(size != 3 && size != 4) {
		fflush(stdout);
		fprintf(stderr, "ERROR: invalid color size of %ld bytes\n", size);
		return -1;
	}

	printf("  %s (RGB%s):", info, size == 3 ? "" : "A");
	for(i = 0; i < size; i++) {
		if(getbyte(state, &color) != 0) {
			return -1;
		}
		printf("%s%ld/0x%02lX", i == 0 ? " (" : ", ", color, color);
	}
	printf(")\n");

	return 0;
}


int scan_end(swf_dump_state *state, swf_dump_tag *tag)
{
	/* we found the end of the file */
	if(tag->size != 0) {
		fflush(stdout);
		fprintf(stderr, "ERROR: invalid End tag (size != 0).\n");
		return scan_undefined(state, tag);
	}
	return 1;
}


int scan_showframe(swf_dump_state *state, swf_dump_tag *tag)
{
	if(tag->size != 0) {
		fflush(stdout);
		fprintf(stderr, "ERROR: invalid ShowFrame tag (size != 0).\n");
		return scan_undefined(state, tag);
	}
	return 0;
}


int scan_placeobject(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, depth, p;

	p = state->bufpos;
	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Place object: #%ld\n", id);

	if(getword(state, &depth) != 0) {
		return -1;
	}
	printf("  Depth: #%ld\n", depth);

	printf("  Transformation matrix:\n");
	if(scan_matrix(state, "    ") != 0) {
		return -1;
	}

	/* the color matrix is present only if the tag size permits it */
	p = state->bufpos - p;
	if(p / 8 + 1 >= tag->size) {
		return 0;
	}
	printf("  Color transformation:\n");
	return scan_color_transform(state, "    ", 0);
}



int scan_definebits(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, offset, idx, max, size, c, total, save, p;
	char		filename[256];
	FILE		*f;

	total = tag->size;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);
	total -= 2;

	if(tag->tag == 35) {
		if(getlong(state, &offset) != 0) {
			return -1;
		}
		total -= 4;
		printf("  Offset to the alpha channel: %ld\n", offset);
	}

	/* shall we save the result? */
	f = NULL;	/* avoid warnings - if save is false it should never be used! */
	save = state->flags->decompress;
	if(save) {
		snprintf(filename, sizeof(filename), "swf_dump-%ld.jpg", id);
		f = fopen(filename, "wb");
		if(f == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: can't open file \"%s\" to save a JPEG image.\n", filename);
			save = 0;
		}
		else if(tag->tag == 6) {
			if(state->jpeg_tables == NULL) {
				fflush(stdout);
				fprintf(stderr, "WARNING: no JPEG tables found yet! Concatenate \"swf_dump-tables.jpg\" and \"%s\" yourself later.\n", filename);
			}
			else {
				/* this already includes the 0xFF 0xD8 and does NOT include the 0xFF 0xD9 at the end */
				fwrite(state->jpeg_tables, 1, state->jpeg_size, f);
			}
		}
		else {
			/* we skip ALL 0xFF 0xD8/9 below! */
			fputc(0xFF, f);
			fputc(0xD8, f);
		}
	}

	max = tag->tag == 6 ? 1 : 2;
	idx = 0;
	while(idx < max) {
		size = 0;
		while(size < total) {
			if(getbyte(state, &c) != 0) {
				return -1;
			}
			size++;
			p = 0;
			while(c == 0xFF) {
				if(getbyte(state, &c) != 0) {
					return -1;
				}
				size++;
				/* some people have played a trick on me and put an FF D9
				 * at the very beginning of their JPEGs!
				 */
				if(c == 0xD9 && size != 2) {
					/* we found the end of a stream */
					goto out;
				}
				p = c;
				if(save) {
					fputc(0xFF, f);
				}
				/* here could also check for 0xE0 and determine
				 * the image width, height and format!
				 */
			}
			if(p != 0xD8 && save) {
				fputc(c, f);
			}
		}
out:
		idx++;
		printf("  Byte size #%ld: %ld (compressed).\n", idx, size);
		total -= size;
	}

	if(save) {
		fputc(0xFF, f);
		fputc(0xD9, f);
		fclose(f);
	}

	if(tag->tag == 35) {
		/* we need to skip the alpha channel also! */
		printf("  Byte size #3 (alpha channel): %ld (compressed).\n", total);
		while(total > 0) {
			if(getbyte(state, &c) != 0) {
				return -1;
			}
			total--;
		}
	}

	return 0;
}




extern int scan_doaction_offset(swf_dump_state *state, swf_dump_tag *tag, long offset, int test_end);
int scan_definebutton(swf_dump_state *state, swf_dump_tag *tag)
{
	long		cnt, id, flags, ref, depth, menu, offset, start_at, end_at;

	start_at = (state->bufpos + 7) / 8;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);

	if(tag->tag == 34) {
		if(getbyte(state, &menu) != 0) {
			return -1;
		}
		printf("  %s\n", menu ? "Track Menu" : "Push Button");

		if(getword(state, &offset) != 0) {
			return -1;
		}
		printf("  Offset to first condition + action: #%ld\n", offset);
	}

	for(cnt = 1;; cnt++) {
		if(getbyte(state, &flags) != 0) {
			return -1;
		}
		if(flags == 0) {
			/* end of button states */
			break;
		}
		printf("  State #%ld\n", cnt);
		printf("    Flags: 0x%02lX", flags);
		if((flags & 0x08) != 0) {
			printf(", HIT TEST");
		}
		if((flags & 0x04) != 0) {
			printf(", DOWN");
		}
		if((flags & 0x02) != 0) {
			printf(", OVER");
		}
		if((flags & 0x01) != 0) {
			printf(", UP");
		}
		printf("\n");
		if(getword(state, &ref) != 0) {
			return -1;
		}
		printf("    Display object ID: #%ld\n", ref);
		if(getword(state, &depth) != 0) {
			return -1;
		}
		printf("    Depth: #%ld\n", depth);
		printf("    Transformation matrix:\n");
		if(scan_matrix(state, "      ") != 0) {
			return -1;
		}
		if(tag->tag == 34) {
			/* we also have a color matrix! */
			printf("    Color transformation:\n");
			if(scan_color_transform(state, "      ", 1) != 0) {
				return -1;
			}
		}
	}

	if(tag->tag == 34) {
		while(offset != 0) {
			if(getword(state, &offset) != 0) {
				return -1;
			}
			printf("  Offset to the next condition + action: #%ld\n", offset);

			if(getword(state, &flags) != 0) {
				return -1;
			}
			printf("  Condition(s): #0x%04lX", flags);
			if((flags & 0xFE00) != 0) {
				printf(", KEY=");
				switch(flags >> 9) {
				case  1: printf("LEFT-ARROW");	break;
				case  2: printf("RIGHT-ARROW");	break;
				case  3: printf("HOME");	break;
				case  4: printf("END");		break;
				case  5: printf("INSERT");	break;
				case  6: printf("DELETE");	break;
				case  8: printf("BACKSPACE");	break;
				case 13: printf("ENTER");	break;
				case 14: printf("ARROW-UP");	break;
				case 15: printf("ARROW-DOWN");	break;
				case 16: printf("PAGE-UP");	break;
				case 17: printf("PAGE-DOWN");	break;
				case 18: printf("TAB");		break;
				case 19: printf("ESCAPE");	break;
				default:
					if((flags >> 9) < 32 || (flags >> 9) == 127) {
						printf("<invalid=0x%02lX>", flags >> 9);
					}
					else if((flags >> 9) == '\'') {
						printf("'\\''");
					}
					else {
						printf("'%c'", (unsigned char) (flags >> 9));
					}
					break;
				}
			}
			if((flags & 0x100) != 0) {
				printf(", OVER DOWN TO IDLE");
			}
			if((flags & 0x080) != 0) {
				printf(", IDLE TO OVER DOWN");
			}
			if((flags & 0x040) != 0) {
				printf(", OUT DOWN TO IDLE");
			}
			if((flags & 0x020) != 0) {
				printf(", OUT DOWN TO OVER DOWN");
			}
			if((flags & 0x010) != 0) {
				printf(", OVER DOWN TO OUT DOWN");
			}
			if((flags & 0x008) != 0) {
				printf(", OVER DOWN TO OVER UP");
			}
			if((flags & 0x004) != 0) {
				printf(", OVER UP TO OVER DOWN");
			}
			if((flags & 0x002) != 0) {
				printf(", OVER UP TO IDLE");
			}
			if((flags & 0x001) != 0) {
				printf(", IDLE TO OVER UP");
			}
			printf("\n");
			end_at = (state->bufpos + 7) / 8;
			if(scan_doaction_offset(state, tag, end_at - start_at, offset == 0) != 0) {
				return -1;
			}
		}
	}
	else {
		printf("  Button actions on activation:\n");
		end_at = (state->bufpos + 7) / 8;
		return scan_doaction_offset(state, tag, end_at - start_at, 1);
	}

	return 0;
}


int scan_definescalinggrid(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id;
	swf_dump_rect	rect;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object Ref. #: %ld\n", id);

	if(getrect(state, &rect) != 0) {
		return -1;
	}
	printf("  Inner Grid Rectangle: (%ld, %ld) - (%ld, %ld)\n",
			rect.x_min, rect.y_min, rect.x_max, rect.y_max);

	return 0;
}



int scan_jpegtables(swf_dump_state *state, swf_dump_tag *tag)
{
	FILE		*f;

	if(tag->size < 4) {
		fflush(stdout);
		fprintf(stderr, "ERROR: invalid JPEG tables (buffer smaller than 4?!?)\n");
		return -1;
	}

	if(state->jpeg_tables != NULL) {
		free(state->jpeg_tables);
	}
	state->jpeg_tables = malloc(tag->size);
	if(state->jpeg_tables == NULL) {
		fflush(stdout);
		fprintf(stderr, "ERROR: out of memory.\n");
		exit(1);
	}
	if(getdata(state, state->jpeg_tables, tag->size) != 0) {
		free(state->jpeg_tables);
		state->jpeg_tables = NULL;
		return -1;
	}

/* just in case, we save them seperate (not including the final 0xFF 0xD9) */
	f = fopen("swf_dump-tables.jpg", "wb");
	if(f == NULL) {
		fflush(stdout);
		fprintf(stderr, "ERROR: can't open file \"swf_dump-tables.jpg\".\n");
		return -1;
	}
	state->jpeg_size = tag->size - 2;
	fwrite(state->jpeg_tables, 1, state->jpeg_size, f);
	fclose(f);

	return 0;
}


int scan_setbackgroundcolor(swf_dump_state *state, swf_dump_tag *tag)
{
	return scan_color(state, "Background Color", tag->size);
}


int scan_definefont(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, idx, l, skip, count, p, script;
	unsigned short	*offsets;

	if(getword(state, &id) != 0) {
		return -1;
	}
	if(state->flags->object == id) {
		printf("font \"name\" {\n");
		script = 1;
	}
	else if(state->flags->object != -1) {
		/* we have to skip the data ourself in this case... */
		skipdata(state, tag->size - 2);
		return 0;
	}
	else {
		printf("  Object ID: #%ld\n", id);
		script = 0;
	}

	p = state->bufpos;
	if(getword(state, &count) != 0) {
		return -1;
	}
	if((count & 1) != 0) {
		fflush(stdout);
		fprintf(stderr, "WARNING: the 1st offset is odd (%ld).\n", count);
		count &= -2;
	}

	/* the first offset gives us the size of the offset table and the number of shapes in the array */
	if(count > 0) {
		offsets = malloc(count);
		if(offsets == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: out of memory.\n");
			return -1;
		}
		offsets[0] = (unsigned short) count;
	}
	else {
		offsets = NULL;
		/* note: when this happens the getdata() doesn't read anything
		 * however it has a side effect of aligning the input buffer
		 * pointer and thus we still need to call it.
		 */
	}
	if(getdata(state, offsets + 1, count - sizeof(unsigned short)) != 0) {
		if(offsets != NULL) {
			free(offsets);
		}
		return -1;
	}

	count /= 2;
	if(!script) {
		printf("  Font defines %ld glyph%s.\n", count, count > 1 ? "s" : "");
	}

	/* we reached the shape array, read it now */
	for(idx = 0; idx < count; idx++) {
		if(script) {
			printf("\tglyph \"g%ld\" {\n", idx);
		}
		else {
			printf("  Font glyph #%ld (offset from start of file: %ld)\n", idx, state->bufpos);
		}
		/* verify the offset */
		l = offsets[idx] - (state->bufpos - p) / 8;
		if(l < 0) {
			fflush(stdout);
			fprintf(stderr, "ERROR: an offset seems to define two overlapping shapes.\n");
		}
		else if(l > 0) {
			fflush(stdout);
			fprintf(stderr, "WARNING: an offset is defining a new shape %ld bytes after the end of the previous shape (%d vs %ld).\n   ",
						l, offsets[idx], (state->bufpos - p) / 8);
			/* this should NEVER occur... we'll see */
			while(l > 0) {
				l--;
				if(getbyte(state, &skip) != 0) {
					free(offsets);
					return -1;
				}
				fflush(stdout);
				fprintf(stderr, " %02lX", skip);
			}
			fflush(stdout);
			fprintf(stderr, "\n");
		}

		if(scan_shape(state, 0x7FFF, 0, 0, 1, 0) != 0) {
			if(offsets != NULL) {
				free(offsets);
			}
			return -1;
		}
		/* align the read pointer to next byte (probably not necessary but just in case...) */
		getdata(state, NULL, 0);
		if(script) {
			printf("\t};\n");
		}
	}

	if(offsets != NULL) {
		free(offsets);
	}

	if(script) {
		printf("};\n\n");
		exit(0);
	}

	return 0;
}




int scan_definetext(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, bits_glyphs, bits_advances, flags, cnt, c, space, x, y, height;
	long		red, green, blue, alpha;
	int		use_rgba;
	swf_dump_rect	rect;

	use_rgba = tag->tag == 33;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);

	if(getrect(state, &rect) != 0) {
		return -1;
	}
	printf("  Placement & Size: (%ld, %ld) - (%ld, %ld)\n",
			rect.x_min, rect.y_min, rect.x_max, rect.y_max);

	printf("  Transformation matrix:\n");
	if(scan_matrix(state, "    ") != 0) {
		return -1;
	}

	if(getbyte(state, &bits_glyphs) != 0) {
		return -1;
	}
	printf("  Uses %ld bits per glyph reference\n", bits_glyphs);

	if(getbyte(state, &bits_advances) != 0) {
		return -1;
	}
	printf("  Uses %ld bits per advance value\n", bits_advances);

	for(;;) {
		if(getbyte(state, &flags) != 0) {
			return -1;
		}
		if(flags == 0) {
			return 0;
		}
		if((flags & 0x80) == 0) { 	/* TEXT */
			printf("  * New text entry\n");
			cnt = flags & 0x7F;
			do {
				if(getbits(state, bits_glyphs, &c, 0) != 0) {
					return -1;
				}
				if(getbits(state, bits_advances, &space, 1) != 0) {
					return -1;
				}
				printf("    Print glyph: #%ld, then advance by %ld TWIPs\n", c, space);
				cnt--;
			} while(cnt > 0);
		}
		else {				/* SETUP */
			printf("  * New setup entry\n");
			if((flags & 0x08) != 0) {		/* has font */
				if(getword(state, &id) != 0) {
					return -1;
				}
				printf("  Use font ID: #%ld\n", id);
			}
			if((flags & 0x04) != 0) {		/* has color */
				if(use_rgba) {
					if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
						return -1;
					}
					printf("    Color: (%ld, %ld, %ld, %ld)\n", red, green, blue, alpha);
				}
				else {
					if(getcolor(state, &red, &green, &blue, NULL) != 0) {
						return -1;
					}
					printf("    Color: (%ld, %ld, %ld)\n", red, green, blue);
				}
			}
			if((flags & 0x03) != 0) {		/* has some offset */
				if((flags & 0x01) != 0) {
					if(getword(state, &x) != 0) {
						return -1;
					}
				}
				else {
					x = 0;
				}
				if((flags & 0x02) != 0) {
					if(getword(state, &y) != 0) {
						return -1;
					}
				}
				else {
					y = 0;
				}
				printf("  Offset: (%ld, %ld)\n", x, y);
			}
			if((flags & 0x08) != 0) {		/* has font height also */
				if(getword(state, &height) != 0) {
					return -1;
				}
				printf("  Font height: %ld\n", height);
			}
		}
	}
	/*NOTREACHED*/
}



int scan_doaction_offset(swf_dump_state *state, swf_dump_tag *tag, long offset, int test_end)
{
	long		action, length, max, l, len, idx, func2_end;
	char		buf[256], *large_buf, *s, *property, **dictionary, *dict_str;
	unsigned char	*data;
	float		f;
	double		d;
	int		dict_size, result;

	result = -1;
	func2_end = 0;
	max = 0;
	dictionary = NULL;
	dict_size = 0;
	large_buf = NULL;
	data = NULL;
	for(;;) {
		if(getbyte(state, &action) != 0) {
error:
			result = -1;
done:
			if(large_buf != NULL) {
				free(large_buf);
			}
			if(dictionary != NULL) {
				free(dictionary);
			}
			return result;
		}
		if(action == 0) {		/* end marker */
			result = 0;
			printf("  Action 0x%08lX- END\n", offset);
			if(!test_end) {
				/* this happens in DefineButton2 where
				 * actions are followed by other
				 * conditions
				 */
				goto done;
			}
			offset++;
			idx = tag->size - offset;
/*printf("Size = %ld, offset = %ld\n", tag->size, offset);*/
			if(idx > 0) {
				printf("  Actions followed by garbage (%ld byte%s)\n", idx, idx == 1 ? "" : "s");
				idx = 0;
				while(offset < tag->size) {
					if(getbyte(state, &action) != 0) {
						goto error;
					}
					printf("  %02lX", action);
					offset++;
					idx++;
					if(idx == 16) {
						printf("\n");
						idx = 0;
					}
				}
				printf("\n");
			}
			goto done;
		}
		if(action >= 128) {
			if(getword(state, &length) != 0) {
				goto error;
			}
			if(length < sizeof(buf)) {
				data = (unsigned char *) buf;
			}
			else if(length > max) {
				if(large_buf != NULL) {
					free(large_buf);
				}
				large_buf = malloc(length);
				data = (unsigned char *) large_buf;
			}
			if(getdata(state, data, length) != 0) {
				goto error;
			}
		}
		else {
			data = NULL;	/* this should never be used in this case */
			length = 0;
		}
		printf("  Action 0x%08lX- ", offset);
		offset++;
		if(length != 0) {
			offset += length + 2;
		}
		switch(action) {
		/*case 0x00:	printf("END"); break; -- caught before */
		case 0x04:	printf("NEXT FRAME"); break;
		case 0x05:	printf("PREVIOUS FRAME"); break;
		case 0x06:	printf("PLAY"); break;
		case 0x07:	printf("STOP"); break;
		case 0x08:	printf("TOGGLE QUALITY"); break;
		case 0x09:	printf("STOP SOUND"); break;
		case 0x0A:	printf("ADD CAST"); break;
		case 0x0B:	printf("SUBTRACT"); break;
		case 0x0C:	printf("MULTIPLY"); break;
		case 0x0D:	printf("DIVIDE"); break;
		case 0x0E:	printf("EQUAL CAST"); break;
		case 0x0F:	printf("LESS THAN CAST"); break;
		case 0x10:	printf("LOGICAL AND"); break;
		case 0x11:	printf("LOGICAL OR"); break;
		case 0x12:	printf("LOGICAL NOT"); break;
		case 0x13:	printf("STRING EQUAL"); break;
		case 0x14:	printf("STRING LENGTH"); break;
		case 0x15:	printf("SUB-STRING"); break;
		case 0x17:	printf("POP"); break;
		case 0x18:	printf("INTEGRAL PART"); break;
		case 0x1C:	printf("GET VARIABLE"); break;
		case 0x1D:	printf("SET VARIABLE"); break;
		case 0x20:	printf("SET TARGET"); break;
		case 0x21:	printf("CONCATENATE STRING"); break;
		case 0x22:	printf("GET PROPERTY"); break;
		case 0x23:	printf("SET PROPERTY"); break;
		case 0x24:	printf("DUPLICATE SPRITE"); break;
		case 0x25:	printf("REMOVE SPRITE"); break;
		case 0x26:	printf("TRACE"); break;
		case 0x27:	printf("START DRAG"); break;
		case 0x28:	printf("STOP DRAG"); break;
		case 0x29:	printf("STRING LESS THAN"); break;
		case 0x2A:	printf("THROW"); break;
		case 0x2B:	printf("CAST OBJECT"); break;
		case 0x2C:	printf("IMPLEMENTS"); break;
		case 0x2D:	printf("FSCOMMAND2"); break;
		case 0x30:	printf("RANDOM"); break;
		case 0x31:	printf("MULTIBYTES STRING LENGTH"); break;
		case 0x32:	printf("ORD"); break;
		case 0x33:	printf("CHR"); break;
		case 0x34:	printf("GET TIMER"); break;
		case 0x35:	printf("MULTIBYTES SUB-STRING"); break;
		case 0x36:	printf("MULTIBYTES ORD"); break;
		case 0x37:	printf("MULTIBYTES CHR"); break;
		case 0x3A:	printf("DELETE"); break;
		case 0x3B:	printf("DELETE ALL"); break;
		case 0x3C:	printf("SET LOCAL VARIABLE"); break;
		case 0x3D:	printf("CALL FUNCTION"); break;
		case 0x3E:	printf("RETURN"); break;
		case 0x3F:	printf("MODULO"); break;
		case 0x40:	printf("NEW"); break;
		case 0x41:	printf("DECLARE LOCAL VARIABLE"); break;
		case 0x42:	printf("DECLARE ARRAY"); break;
		case 0x43:	printf("DECLARE OBJECT"); break;
		case 0x44:	printf("TYPE OF"); break;
		case 0x45:	printf("GET TARGET"); break;
		case 0x46:	printf("ENUMERATE"); break;
		case 0x47:	printf("ADD TYPED"); break;
		case 0x48:	printf("LESS THAN TYPED"); break;
		case 0x49:	printf("EQUAL TYPED"); break;
		case 0x4A:	printf("NUMBER"); break;
		case 0x4B:	printf("STRING"); break;
		case 0x4C:	printf("DUPLICATE"); break;
		case 0x4D:	printf("SWAP"); break;
		case 0x4E:	printf("GET MEMBER"); break;
		case 0x4F:	printf("SET MEMBER"); break;
		case 0x50:	printf("INCREMENT"); break;
		case 0x51:	printf("DECREMENT"); break;
		case 0x52:	printf("CALL METHOD"); break;
		case 0x53:	printf("NEW METHOD"); break;
		case 0x54:	printf("INSTANCE OF"); break;
		case 0x55:	printf("ENUMERATE OBJECT"); break;
		case 0x60:	printf("AND"); break;
		case 0x61:	printf("OR"); break;
		case 0x62:	printf("XOR"); break;
		case 0x63:	printf("SHIFT LEFT"); break;
		case 0x64:	printf("SHIFT RIGHT"); break;
		case 0x65:	printf("SHIFT RIGHT UNSIGNED"); break;
		case 0x66:	printf("STRICT EQUAL"); break;
		case 0x67:	printf("GREATER THAN TYPED"); break;
		case 0x68:	printf("STRING GREATER THAN"); break;
		case 0x69:	printf("EXTENDS"); break;
		case 0x81:	l = data[0] + data[1] * 256;
				printf("GOTO FRAME #%ld", l);
				break;
		case 0x83:	l = strlen((char *) data) + 1;
				printf("GET URL \"%s\", TARGET \"%s\"", data, data + l);
				break;
		case 0x87:	printf("STORE REGISTER #%d", data[0]); break;
		case 0x88:	l = data[0] + data[1] * 256;
				printf("DICTIONARY (%ld entries)", l);
				if(dictionary != 0) {
					free(dictionary);
				}
				dict_size = l;
				dictionary = malloc(l * sizeof(char *) + length - 2);
				memcpy(dictionary + l, data + 2, length - 2);
				s = (char *) (dictionary + l);
				for(idx = 0; idx < l; idx++) {
					dictionary[idx] = s;
					printf("\n	String #%ld: \"%s\"", idx, s);
					s += strlen(s) + 1;
				}
				break;
		case 0x8A:	l = data[0] + data[1] * 256;
				printf("WAIT FOR FRAME #%ld, or skip %d instructions", l, data[2]);
				break;
		case 0x8B:	printf("SET TARGET \"%s\"", data); break;
		case 0x8C:	printf("GOTO LABEL \"%s\"", data); break;
		case 0x8D:	printf("WAIT FOR FRAME (dynamic), or skip %d instructions", data[0]); break;
		case 0x8E:	{
				const char *n;

				printf("FUNCTION2 %s(", data);
				s = (char *) data + strlen((char *) data) + 1;
				if(s[2] != 0) {
					printf("_registers:%d", (unsigned char) s[2]);
					n = ", ";
				}
				else {
					n = "";
				}
				l = (unsigned char) s[3] + (unsigned char) s[4] * 256;
				if((l & 0x0003) != 0) {
					printf("%s", n);
					if((l & 0x0002) != 0) {
						printf("/");
					}
					printf("this");
					n = ", ";
				}
				if((l & 0x000C) != 0) {
					printf("%s", n);
					if((l & 0x0008) != 0) {
						printf("/");
					}
					printf("arguments");
					n = ", ";
				}
				if((l & 0x0030) != 0) {
					printf("%s", n);
					if((l & 0x0020) != 0) {
						printf("/");
					}
					printf("super");
					n = ", ";
				}
				if((l & 0x0040) != 0) {
					printf("%s_root", n);
					n = ", ";
				}
				if((l & 0x0080) != 0) {
					printf("%s_parent", n);
					n = ", ";
				}
				if((l & 0x0100) != 0) {
					printf("%s_global", n);
					n = ", ";
				}
				l = (unsigned char) s[0] + (unsigned char) s[1] * 256;
				s += 5;
				while(l > 0) {
					l--;
					printf("%s%s:%d", n, s + 1, (unsigned char) s[0]);
					s += strlen(s + 1) + 2;
					n = ", ";
				}
				l = (unsigned char) s[0] + (unsigned char) s[1] * 256;
				if(func2_end < offset + l) {
					func2_end = offset + l;
				}
				printf("); [%ld bytes -> 0x%08lX]", l, offset + l);
				}
				break;
		case 0x8F:	{
				int finally_len = (unsigned char) data[1] + (unsigned char) data[2] * 256;
				int catch_len = (unsigned char) data[3] + (unsigned char) data[4] * 256;
				int try_len = (unsigned char) data[5] + (unsigned char) data[6] * 256;

				printf("TRY is %d bytes -> 0x%08lX", try_len, offset + try_len);
				if((data[0] & 1) != 0) {
					printf("; CATCH(");
					if((data[0] & 4) != 0) {
						printf("%d", data[7]);
					}
					else {
						printf("%s", data + 7);
					}
					printf(") is %d bytes -> 0x%08lX", catch_len, offset + try_len + catch_len);
				}
				if((data[0] & 2) != 0) {
					printf("; FINALLY is %d bytes -> 0x%08lX", finally_len, offset + try_len + catch_len + finally_len);
				}
				}
				break;
		case 0x94:	l = data[0] + data[1] * 256;
				printf("WITH [%ld bytes -> 0x%08lX]", l, offset + l);
				break;
		case 0x96:	printf("PUSH DATA");
				s = (char *) data;
				l = length;
				while(l > 0) {
					l--;
					switch(*s++) {
					case 0:
						printf("\n    String: \"%s\"", s);
						len = strlen(s) + 1;
						s += len;
						l -= len;
						break;

					case 1:
						/*
						 * ensure the value is properly aligned - some processor don't like odd addresses
						 * and also it may look swapped on some processors
						 */
						((char *) &f)[0] = s[0];
						((char *) &f)[1] = s[1];
						((char *) &f)[2] = s[2];
						((char *) &f)[3] = s[3];
						property = "";
						if(f == floor(f)) switch((long) floor(f)) {
						case 0:
							property = " (x)";
							break;

						case 1:
							property = " (y)";
							break;

						case 2:
							property = " (x_scale)";
							break;

						case 3:
							property = " (y_scale)";
							break;

						case 4:
							property = " (current_frame)";
							break;

						case 5:
							property = " (total_frames)";
							break;

						case 6:
							property = " (alpha)";
							break;

						case 7:
							property = " (visibility)";
							break;

						case 8:
							property = " (width)";
							break;

						case 9:
							property = " (height)";
							break;

						case 10:
							property = " (rotation)";
							break;

						case 11:
							property = " (target)";
							break;

						case 12:
							property = " (loaded_frames)";
							break;

						case 13:
							property = " (name)";
							break;

						case 14:
							property = " (drop target)";
							break;

						case 15:
							property = " (url)";
							break;

						case 16:
							property = " (high quality)";
							break;

						case 17:
							property = " (show focus rectangle)";
							break;

						case 18:
							property = " (sound buffer time)";
							break;

						case 19:
							property = " (quality)";
							break;

						case 20:
							property = " (x_mouse)";
							break;

						case 21:
							property = " (y_mouse)";
							break;

						/*
						 * apparently this would be a flag
						 * but it also seems to be a depth
						 * issue not a property
						 */
						case 16384:
							property = " (WTHIT)";
							break;

						}
						printf("\n    Float: %g%s", f, property);
						s += 4;
						l -= 4;
						break;

					case 2:
						printf("\n    Null");
						break;

					case 3:
						printf("\n    Undefined");
						break;

					case 4:
						/*
						 * NOTE: it looks like the
						 * register 255 can't be used
						 * even in func2...
						 */
						if(offset >= func2_end
						&& (unsigned char) *s >= 4) {
							printf("\n    Invalid Register: %d (only 0 to 3 allowed)", (unsigned char) *s++);
						}
						else {
							printf("\n    Register: %d", (unsigned char) *s++);
						}
						l--;
						break;

					case 5:
						printf("\n    Boolean: %s", *s++ ? "true" : "false");
						l--;
						break;

					case 6:
						/* the long words are swapped and it may not be aligned */
						((char *) &d)[4] = s[0];
						((char *) &d)[5] = s[1];
						((char *) &d)[6] = s[2];
						((char *) &d)[7] = s[3];
						((char *) &d)[0] = s[4];
						((char *) &d)[1] = s[5];
						((char *) &d)[2] = s[6];
						((char *) &d)[3] = s[7];
						printf("\n    Double: %g", d);
						s += 8;
						l -= 8;
						break;

					case 7:
						/* ensure the value is properly aligned - some processor don't like odd addresses */
						len = (s[3] << 24) + ((unsigned char) s[2] << 16) + ((unsigned char) s[1] << 8) + (unsigned char) s[0];
						printf("\n    Integer: %ld (0x%02X%02X%02X%02X)", len, (unsigned char) s[3], (unsigned char) s[2], (unsigned char) s[1], (unsigned char) s[0]);
						s += 4;
						l -= 4;
						break;

					case 8:
						if(*s < dict_size) {
							dict_str = dictionary[(unsigned char) *s];
						}
						else {
							dict_str = "<not found in dictionary>";
						}
						printf("\n    Dictionary Lookup: %d (%s)", (unsigned char) *s, dict_str);
						s++;
						l--;
						break;

					case 9:
						len = s[0] + s[1] * 256;
						if(len < dict_size) {
							dict_str = dictionary[len];
						}
						else {
							dict_str = "<not found in dictionary>";
						}
						printf("\n    Large Dictionary Lookup: %ld", len);
						s += 2;
						l -= 2;
						break;

					default:
						printf("\n    WARNING: unknown push data type #%d (0x%02X) -- skip %ld bytes\n", s[-1], s[-1], l);
						l = 0;		/* skip the rest! */
						break;

					}
				}
				break;
		case 0x99:	l = (short) (data[0] + data[1] * 256);
				printf("BRANCH ALWAYS [%s%ld bytes -> 0x%08lX]", l >= 0 ? "+" : "", l, offset + l);
				break;
		case 0x9A:	printf("GET URL2%s", data[0] == 1 ? ": GET" : (data[0] == 2 ? ": POST" : " (no variable)"));
				break;
		case 0x9B:	printf("FUNCTION: %s(", data);
				s = (char *) data + strlen((char *) data) + 1;
				l = (unsigned char) s[0] + (unsigned char) s[1] * 256;
				s += 2;
				if(l > 0) {
					printf("%s", s);
					s += strlen(s) + 1;
					l--;
					while(l > 0) {
						l--;
						printf(", %s", s);
						s += strlen(s) + 1;
					}
				}
				l = (unsigned char) s[0] + (unsigned char) s[1] * 256;
				printf("); [%ld bytes -> 0x%08lX]", l, offset + l);
				break;
		case 0x9D:	l = (short) (data[0] + data[1] * 256);
				printf("BRANCH IF TRUE [%s%ld bytes -> 0x%08lX]", l >= 0 ? "+" : "", l, offset + l);
				break;
		case 0x9E:	printf("CALL FRAME"); break;	/* there isn't any data -- ie. size always == 0 */
		case 0x9F:	printf("GOTO EXPRESSION and %s", data[0] ? "PLAY" : "STOP"); break;
		default:	printf("? unknown or undefined action ? (%ld/%02lX)", action, action); break;
		}
		printf("\n");
	}
	/*NOTREACHED*/
}


int scan_doaction(swf_dump_state *state, swf_dump_tag *tag)
{
	return scan_doaction_offset(state, tag, 0, 1);
}



void print_language(long language)
{
	printf("  Language: #%ld - ", language);
	switch(language) {
	case 0:
		printf("LOCALE");
		break;

	case 1:
		printf("LATIN");
		break;

	case 2:
		printf("JAPANESE");
		break;

	case 3:
		printf("KOREAN");
		break;

	case 4:
		printf("SIMPLIFIED CHINESE");
		break;

	case 5:
		printf("TRADITIONAL CHINESE");
		break;

	default:
		printf("(unknown?)");
		break;

	}
	printf("\n");
}


int scan_definefontinfo(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, size, flags, count, idx, v;
	char		*name, c;

	/* get the font reference/identification */
	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);

	/* read the name (that's in a P-string) */
	if(getbyte(state, &size) != 0) {
		return -1;
	}
	name = malloc(size + 1);
	if(name == 0) {
		fflush(stdout);
		fprintf(stderr, "ERROR: out of memory!\n");
		exit(1);
	}
	if(getdata(state, name, size) != 0) {
		return -1;
	}
	name[size] = '\0';
	printf("  Font Name: \"%s\".\n", name);
	free(name);

	/* get the flags */
	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	if((flags & 0xC0) != 0) {
		printf("  Some reserved font flags are set: 0x%02lX\n", flags);
	}
	if((flags &0x38) != 0) {
		printf("  Prior v6.x language:");
		if((flags & 0x20) != 0) {
			printf(" Unicode");
		}
		if((flags & 0x10) != 0) {
			printf(" Shift-JIS");
		}
		if((flags & 0x08) != 0) {
			printf(" ANSII");
		}
		printf("\n");
	}
	if((flags & 0x7) != 0) {
		printf("  Flags:");
		if((flags & 0x04) != 0) {
			printf(" ITALIC");
		}
		if((flags & 0x02) != 0) {
			printf(" BOLD");
		}
		if((flags & 0x01) != 0) {
			printf(" WIDE");
		}
		printf("\n");
	}

	if(tag->tag == 62) {		/* DefineFontInfo2 includes the language */
		if(getbyte(state, &v) != 0) {
			return -1;
		}
		print_language(v);
	}

	/* compute the number of glyphs */
	count = tag->size - 4 - size;

	if((flags & 0x01) == 0) {
		for(idx = 0; idx < count; idx++) {
			if(getbyte(state, &v) != 0) {
				return -1;
			}
			if(v < ' ' || v > 0x7E) {
				c = '.';
			}
			else {
				c = (char) v;
			}
			printf("  Glyph #%03ld: \"%c\" (%ld).\n", idx, c, v);
		}
	}
	else {
		count /= 2;
		for(idx = 0; idx < count; idx++) {
			if(getword(state, &v) != 0) {
				return -1;
			}
			if(v < ' ' || v > 0x7E) {
				c = '.';
			}
			else {
				c = (char) v;
			}
			printf("  Glyph #%03ld: \"%c\" (%ld).\n", idx, c, v);
		}
	}

	return 0;
}


static int sound_rate[4] = { 5512, 11025, 22050, 44100 };
static const char *sound_size[2] = { "8 bits", "16 bits" };
static const char *sound_stereo[2] = { "mono", "stereo" };
static const char *sound_compression[16] = {
	"raw (0)",
	"ADPCM (1)",
	"MP3 (2)",
	"uncompressed (3)",
	"unknown (4)",
	"unknown (5)",
	"Nellymoser (6)",
	"unknown (7)",
	"unknown (8)",
	"unknown (9)",
	"unknown (10)",
	"unknown (11)",
	"unknown (12)",
	"unknown (13)",
	"unknown (14)",
	"unknown (15)"
};


int scan_definesound(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, flags, count, compressed_size;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Sound ID: #%ld\n", id);

	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	printf("  Sound Compression Format: %s\n", sound_compression[(flags >> 4) & 15]);
	printf("  Sound Rate: %d\n", sound_rate[(flags >> 2) & 3]);
	printf("  Sound Size: %s\n", sound_size[(flags >> 1) & 1]);
	printf("  Sound Mode: %s\n", sound_stereo[flags & 1]);

	if(getlong(state, &count) != 0) {
		return -1;
	}
	printf("  Total number of samples: %ld\n", count);

	compressed_size = tag->size - 7;
	printf("  Size of data buffer: %ld bytes\n", compressed_size);

	/* we need to skip the data -- later we probably want to save it in a file */
	skipdata(state, compressed_size);

	return 0;
}


static int scan_soundinfo(swf_dump_state *state, const char *indent)
{
	long		value, flags, count;

	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	if((flags & 0x0C0) != 0) {
		fflush(stdout);
		fprintf(stderr, "WARNING: reserved flags not zero (0x%02lX)\n", flags & 0x0C0);
	}

	if((flags & 0x20) != 0) {
		printf("%sStop sound playback\n", indent);
	}
	else {
		printf("%sStart playing this sound\n", indent);
	}
	if((flags & 0x10) != 0) {
		printf("%sDon't play multiple times\n", indent);
	}
	else {
		printf("%sSound can be started multiple times\n", indent);
	}

	if((flags & 0x01) != 0) {
		if(getlong(state, &value) != 0) {
			return -1;
		}
		printf("%sStart playing at sample: %lu\n", indent, value);
	}

	if((flags & 0x02) != 0) {
		if(getlong(state, &value) != 0) {
			return -1;
		}
		printf("%sStop playing at sample: %lu\n", indent, value);
	}

	if((flags & 0x04) != 0) {
		if(getword(state, &value) != 0) {
			return -1;
		}
		printf("%sRepeat the sound %lu times\n", indent, value);
	}

	if((flags & 0x08) != 0) {
		if(getbyte(state, &count) != 0) {
			return -1;
		}
		printf("%s%ld envelopes:\n", indent, count);
		while(count > 0) {
			count--;
			if(getlong(state, &value) != 0) {
				return -1;
			}
			printf("%s  Volume at offset %lu: ", indent, value);
			if(getword(state, &value) != 0) {
				return -1;
			}
			printf("%lu (left) and ", value);
			if(getword(state, &value) != 0) {
				return -1;
			}
			printf("%lu (right).\n", value);
		}
	}

	return 0;
}

int scan_startsound(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Start Sound with ID: #%ld\n", id);

	scan_soundinfo(state, "  ");

	return 0;
}


int scan_definebuttonsound(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Over Up to Idle Sound: #%ld\n", id);
	scan_soundinfo(state, "    ");

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Idle to Over Up Sound: #%ld\n", id);
	scan_soundinfo(state, "    ");

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Over Up to Over Down Sound: #%ld\n", id);
	scan_soundinfo(state, "    ");

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Over Down to Over Up Sound: #%ld\n", id);
	scan_soundinfo(state, "    ");

	return 0;
}


int scan_soundstreamhead(swf_dump_state *state, swf_dump_tag *tag)
{
	long		flags, seek, size;

	if(getword(state, &flags) != 0) {
		return -1;
	}

	if((flags & 0xF000) != 0) {
		fflush(stdout);
		fprintf(stderr, "WARNING: reserved bits not zero (0x%04lX)\n", flags & 0xF000);
	}
	printf("  Playback Rate: %d\n", sound_rate[(flags >> 10) & 3]);
	printf("  Playback Size: %s\n", sound_size[(flags >> 9) & 1]);
	printf("  Playback Mode: %s\n", sound_stereo[(flags >> 8) & 1]);
	printf("  Stream Sound Rate: %d\n", sound_rate[(flags >> 2) & 3]);
	printf("  Stream Sound Size: %s\n", sound_size[(flags >> 1) & 1]);
	printf("  Stream Sound Mode: %s\n", sound_stereo[(flags >> 0) & 1]);
	printf("  Stream Sound Compression: %s\n", sound_compression[(flags >> 4) & 15]);

	if(getword(state, &size) != 0) {
		return -1;
	}
	printf("  Average Data Size: %ld\n", size);

	/* in case we have an MP3 compression we also have a latency seek */
	if((flags & (15 << 12)) == (2 << 12)) {
		if(getword(state, &seek) != 0) {
			return -1;
		}
		printf("  MP3 Latency Seek: %ld\n", seek);
	}

	return 0;
}


int scan_soundstreamblock(swf_dump_state *state, swf_dump_tag *tag)
{
	/* at this time we simply skip the data! */
	skipdata(state, tag->size);

	return 0;
}


int scan_definebuttoncxform(swf_dump_state *state, swf_dump_tag *tag) { return scan_undefined(state, tag); }


int scan_definevideostream(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, value;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld (0x%04lX)\n", id, id);

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Number of Frames: %ld\n", value);

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Width: %ld\n", value);

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Height: %ld\n", value);

	if(getbyte(state, &value) != 0) {
		return -1;
	}
	if((value & 0xF0) != 0) {
		printf("  Reserved not zero: 0x%02lX\n", value);
	}

	switch(value & 0x0E) {
	case 0x00:
		printf("  Video Deblocking: use VIDEOPACKET value\n");
		break;

	case 0x02:
		printf("  Video Deblocking: off\n");
		break;

	case 0x04:
		printf("  Video Deblocking: Level 1 (Fast deblocking filter)\n");
		break;

	case 0x06:
		printf("  Video Deblocking: Level 2 (VP6 only, better deblocking filter)\n");
		break;

	case 0x08:
		printf("  Video Deblocking: Level 3 (VP6 only, better deblocking plus fast deringing filter)\n");
		break;

	case 0x0A:
		printf("  Video Deblocking: Level 4 (VP6 only, better deblocking plus better deringing filter)\n");
		break;

	default:
		printf("  Video Deblocking: reserved deblocking #0x%02lX\n", value & 0x0E);
		break;

	}

	printf("  Video Smoothing: %s\n", (value & 1) ? "on" : "off");

	if(getbyte(state, &value) != 0) {
		return -1;
	}
	printf("  Video Codec: ");
	switch(value) {
	case 2:
		printf("Sorenson H.263\n");
		break;

	case 3:
		printf("Screen Video (V7+)\n");
		break;

	case 4:
		printf("VP6 (V8+)\n");
		break;

	case 5:
		printf("VP6 with Alpha (V8+)\n");
		break;

	default:
		printf("Unknown #%ld (0x%02lX)\n", value, value);
		break;

	}

	return 0;
}


int scan_videoframe(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, value;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object Ref. #: %ld (0x%04lX)\n", id, id);

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Frame Number: %ld (0x%04lX)\n", value, value);

	/* we just skip the VideoData field */
	skipdata(state, tag->size - 4);

	return 0;
}


int scan_protect(swf_dump_state *state, swf_dump_tag *tag)
{
	long		flags;

	/* most of the time, the Protect tag is empty */
	if(tag->size == 0) {
		return 0;
	}

	/* the DebugProtect2 includes a 16 bits value */
	if(tag->tag == 64) {
		if(getword(state, &flags) != 0) {
			return -1;
		}
		printf("  Flags: 0x%04lX\n", flags);
		tag->size -= 2;		/* we've eaten two bytes already */
	}

	/* when there is a size != 0 then there is a password (V5.0+) */
	printf("  MD5 password:\n");
	return scan_unknown(state, tag);
}


int scan_scriptlimits(swf_dump_state *state, swf_dump_tag *tag)
{
	long		value;

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Maximum Recursion Depth: %ld\n", value);

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Timeout: %ld seconds\n", value);

	return 0;
}


int scan_fileattributes(swf_dump_state *state, swf_dump_tag *tag)
{
	long		flags;

	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	printf("  Use Network: %s\n", (flags & 0x01) == 0 ? "No" : "Yes");
	printf("  Has Metadata Tag: %s\n", (flags & 0x10) == 0 ? "No" : "Yes");
	if((flags & 0xEE) != 0) {
		printf("  Other flags[0]: 0x%02lX\n", flags);
	}

	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	if(flags != 0) {
		printf("  Other flags[1]: 0x%02lX\n", flags);
	}

	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	if(flags != 0) {
		printf("  Other flags[2]: 0x%02lX\n", flags);
	}

	if(getbyte(state, &flags) != 0) {
		return -1;
	}
	if(flags != 0) {
		printf("  Other flags[3]: 0x%02lX\n", flags);
	}

	return 0;
}


int scan_metadata(swf_dump_state *state, swf_dump_tag *tag)
{
	char *		xml;

	if(getstring(state, &xml) != 0) {
		return -1;
	}
	printf("  XML:\n==========\n%s\n==========\n", xml);
	free(xml);

	return 0;
}


int scan_settabindex(swf_dump_state *state, swf_dump_tag *tag)
{
	long		value;

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Depth: %ld\n", value);

	if(getword(state, &value) != 0) {
		return -1;
	}
	printf("  Tab Index: %ld\n", value);

	return 0;
}


static void print_events(const char *msg, unsigned long flags)
{
	printf("%s", msg);

	if((flags & 0x00020000) != 0) {
		printf(" KEY-PRESS");
	}
	if((flags & 0x00010000) != 0) {
		printf(" POINTER-DRAG-LEAVE");
	}
	if((flags & 0x00008000) != 0) {
		printf(" POINTER-DRAG-ENTER");
	}
	if((flags & 0x00004000) != 0) {
		printf(" POINTER-LEAVE");
	}
	if((flags & 0x00002000) != 0) {
		printf(" POINTER-ENTER");
	}
	if((flags & 0x00001000) != 0) {
		printf(" POINTER-RELEASE-OUTSIDE");
	}
	if((flags & 0x00000800) != 0) {
		printf(" POINTER-RELEASE-INSIDE");
	}
	if((flags & 0x00000400) != 0) {
		printf(" POINTER-PUSH");
	}
	if((flags & 0x00000200) != 0) {
		printf(" INITIALIZE");
	}
	if((flags & 0x00000100) != 0) {
		printf(" DATA");
	}
	if((flags & 0x00000080) != 0) {
		printf(" KEY-UP");
	}
	if((flags & 0x00000040) != 0) {
		printf(" KEY-DOWN");
	}
	if((flags & 0x00000020) != 0) {
		printf(" POINTER-UP");
	}
	if((flags & 0x00000010) != 0) {
		printf(" POINTER-DOWN");
	}
	if((flags & 0x00000008) != 0) {
		printf(" POINTER-MOVE");
	}
	if((flags & 0x00000004) != 0) {
		printf(" UNLOAD");
	}
	if((flags & 0x00000002) != 0) {
		printf(" ENTER-FRAME");
	}
	if((flags & 0x00000001) != 0) {
		printf(" ONLOAD");
	}
	/* mark unknown flags in some way */
	if((flags & 0xFFFC0000) != 0) {
		printf(" ???");
	}

	printf(" (0x%08lX)\n", flags);
}


int scan_placeobject2(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, depth, flags, r, start_at, end_at;
	char		*str;

	start_at = (state->bufpos + 7) / 8;

	/* get all the flags at once */
	if(getbyte(state, &flags) != 0) {
		return -1;
	}

	if(getword(state, &depth) != 0) {
		return -1;
	}
	printf("  Depth: #%ld\n", depth);

	if((flags & 0x02) != 0) {		/* Has ID */
		if(getword(state, &id) != 0) {
			return -1;
		}
		printf("  %slace object: #%ld\n", (flags & 0x01) == 0 ? "Rep" : "P", id);
	}
	else if((flags & 0x01) == 0) {
		printf("  Remove object at this depth.\n");
	}

	if((flags & 0x04) != 0) {		/* Has matrix */
		printf("  Transformation matrix:\n");
		if(scan_matrix(state, "    ") != 0) {
			return -1;
		}
	}

	if((flags & 0x08) != 0) {		/* Has color transformation */
		printf("  Color transformation:\n");
		if(scan_color_transform(state, "    ", 1) != 0) {
			return -1;
		}
	}

	if((flags & 0x10) != 0) {		/* Has morph position */
		if(getword(state, &id) != 0) {
			return -1;
		}
		printf("  Morph at: #%ld (%g%%)\n", id, id * 100.0 / 65535.0);
	}

	if((flags & 0x20) != 0) {		/* Has a name */
		if(getstring(state, &str) != 0) {
			return -1;
		}
		printf("  Name: \"%s\".\n", str);
		free(str);
	}

	if((flags & 0x40) != 0) {		/* Has a clipping depth */
		if(getword(state, &id) != 0) {
			return -1;
		}
		printf("  Clipping depth: %ld.\n", id);
	}

	if((flags & 0x80) != 0) {
		printf("  On Event(s) Actions:\n");
		if(getword(state, &id) != 0) {
			return -1;
		}
		if(id != 0) {
			/* bizarre! the reserved word should always be zero */
			printf("    Reserved: 0x%04lX\n", id);
		}
		if(state->header[3] <= 5) {
			r = getword(state, &id) != 0;
		}
		else {
			r = getlong(state, &id) != 0;
		}
		if(r != 0) {
			return -1;
		}
		print_events("    All flags:", id);

		for(;;) {
			if(state->header[3] <= 5) {
				r = getword(state, &id);
			}
			else {
				r = getlong(state, &id);
			}
			if(r != 0) {
				return -1;
			}
			if(id == 0) {
				break;
			}
			print_events("    On events:", id);
			if(getlong(state, &id) != 0) {
				return -1;
			}
			printf("      Length of actions code: %ld bytes\n", id);
			printf("****************************************\n");
			end_at = (state->bufpos + 7) / 8;
			scan_doaction_offset(state, tag, end_at - start_at, 0);
			printf("****************************************\n");
		}
	}

	return 0;
}






int scan_removeobject(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, depth;

	if(tag->tag == 5) {
		if(getword(state, &id) != 0) {
			return -1;
		}
		printf("  Object Ref. #: %ld\n", id);
	}

	if(getword(state, &depth) != 0) {
		return -1;
	}

	printf("  Depth: %ld\n", depth);

	return 0;
}




/*
 * This can be done within a list of shapes so we want to have it seperate
 * to be able to call it from different places
 */
int scan_styles(swf_dump_state *state, long max_styles, long use_rgba, long two_objects, long shape4)
{
	static const char *	bitmap_types[4] = {
				"Smoothed and tiled", "Smoothed and clipped",
				"Tiled with hard edge", "Clipped with hard edge"
			};
	static const char *	gradient_types[4] = {
				"Linear", "unknown1", "Radial", "Focal"
			};
	static const char *	cap_types[4] = {
				"Round", "None", "Square", "unknown3"
			};
	static const char *	join_types[4] = {
				"Round", "Bevel", "Miter", "unknown3"
			};
	long		i, id, count, type, red, green, blue, alpha, width;
	long		script, has_fill;

	script = state->flags->object != -1;
	has_fill = 0;

/* read the fill style array */
	if(getbyte(state, &count) != 0) {
		return -1;
	}
	if(count == 255 && max_styles != 255) {
		if(getword(state, &count) != 0) {
			return -1;
		}
	}

	/* NOTE: count CAN be zero */
	if(!script) {
		printf("  Fill styles: %ld entries\n", count);
	}
	i = 1;
	while(count > 0) {
		count--;
		if(script) {
			printf("\tfill style \"f%ld\" { ", i);
		}
		else {
			printf("    Fill #%ld\n", i);
		}
		if(getbyte(state, &type) != 0) {
			return -1;
		}
		switch(type) {
		case 0x00:		/* solid fill */
			if(use_rgba) {
				if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
					return -1;
				}
				if(script) {
					printf("color { %g, %g, %g, %g }", red / 255.0, green / 255.0, blue / 255.0, alpha / 255.0);
				}
				else {
					printf("    Solid: (%ld, %ld, %ld, %ld)\n", red, green, blue, alpha);
				}
				if(two_objects) {	/* NOTE: use_rgba is always true when two_objects is true */
					if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
						return -1;
					}
					if(script) {
						printf("; color { %g, %g, %g, %g }", red / 255.0, green / 255.0, blue / 255.0, alpha / 255.0);
					}
					else {
						printf("    Solid to morph to: (%ld, %ld, %ld, %ld)\n", red, green, blue, alpha);
					}
				}
			}
			else {
				if(getcolor(state, &red, &green, &blue, NULL) != 0) {
					return -1;
				}
				if(script) {
					printf("color { %g, %g, %g }", red / 255.0, green / 255.0, blue / 255.0);
				}
				else {
					printf("    Solid: (%ld, %ld, %ld)\n", red, green, blue);
				}
			}
			break;

		case 0x10:		/* linear or radial gradient fill */
		case 0x12:
		case 0x13:		/* V8.0 */
			printf("    %s gradient fill\n", gradient_types[type & 0x03]);
			printf("    Matrix:\n");
			scan_matrix(state, "      ");
			if(two_objects) {
				printf("    Matrix to morph to:\n");
				scan_matrix(state, "      ");
			}
			printf("    Gradient:\n");
			scan_gradient(state, "      ", use_rgba, two_objects, type == 0x13);
			break;

		case 0x40:		/* tilled or clipped bitmap fill */
		case 0x41:
		case 0x42:		/* V7.0 */
		case 0x43:		/* V7.0 */
			printf("    %s bitmap fill:\n", bitmap_types[type & 3]);
			if(getword(state, &id) != 0) {
				return -1;
			}
			if(id == 65535) {
				printf("      Clipping matrix definition.\n");
			}
			else {
				printf("      Bitmap ID: %ld\n", id);
			}
			printf("      Matrix:\n");
			scan_matrix(state, "        ");
			if(two_objects) {
				printf("      Matrix to morph to:\n");
				scan_matrix(state, "        ");
			}
			break;

		default:
			printf("    Unknown shape fill type: 0x%02lX\n", type);
			return -1;

		}
		if(script) {
			printf(" };\n");
		}
		i++;
	}

/* read the line style array */
	if(getbyte(state, &count) != 0) {
		return -1;
	}
	if(count == 255 && max_styles != 255) {
		if(getword(state, &count) != 0) {
			return -1;
		}
	}

	/* NOTE: count CAN be zero */
	if(!script) {
		printf("  Line styles: %ld entries\n", count);
	}
	i = 1;
	while(count > 0) {
		count--;

		if(script) {
			printf("\tline style \"l%ld\" { ", i);
		}
		else {
			printf("    Line #%ld\n", i);
		}

		if(getword(state, &width) != 0) {
			return -1;
		}
		if(script) {
			printf("%g", width / 20.0);
		}
		else {
			printf("    Line width: %ld\n", width);
		}

		if(two_objects) {
			if(getword(state, &width) != 0) {
				return -1;
			}
			if(script) {
				printf(", %g", width / 20.0);
			}
			else {
				printf("    Line width to morpth to: %ld\n", width);
			}
		}

		if(shape4) {
			long start_cap, end_cap, join, no_hscale, no_vscale;
			long pixel_hinting, no_close, pad, miter_limit;

			if(getbits(state, 2, &start_cap, 0)) {
				return -1;
			}
			if(getbits(state, 2, &join, 0)) {
				return -1;
			}
			if(getbits(state, 1, &has_fill, 0)) {
				return -1;
			}
			if(getbits(state, 1, &no_hscale, 0)) {
				return -1;
			}
			if(getbits(state, 1, &no_vscale, 0)) {
				return -1;
			}
			if(getbits(state, 1, &pixel_hinting, 0)) {
				return -1;
			}
			if(getbits(state, 5, &pad, 0)) {
				return -1;
			}
			if(getbits(state, 1, &no_close, 0)) {
				return -1;
			}
			if(getbits(state, 2, &end_cap, 0)) {
				return -1;
			}

			if(join == 2) {
				getword(state, &miter_limit);
			}

			if(script) {
				if(start_cap == end_cap) {
					if(start_cap != 0) {
						printf(" Caps: %ld;", start_cap);
					}
				}
				else {
					printf(" Caps: %ld, %ld;", start_cap, end_cap);
				}
				if(join != 0) {
					printf(" Join: %ld", join);
					if(join == 2) {
						printf(", %.5g", (float) miter_limit / 256.0f);
					}
					printf(";");
				}
				if(no_hscale || no_vscale) {
					printf(" Scale: %ld", no_hscale);
					if(no_hscale != no_vscale) {
						printf(", %ld", no_vscale);
					}
					printf(";");
				}
				if(pixel_hinting) {
					printf(" Pixel Hinting: 1;");
				}
				if(no_close) {
					printf(" No Close: 1;");
				}
			}
			else {
				printf("    Start/end caps: %s, %s\n", cap_types[start_cap], cap_types[end_cap]);
				printf("    Join: %s", join_types[join]);
				if(join == 2) {
					printf(", %.3g (0x%04lX)", (float) miter_limit / 256.0f, miter_limit);
				}
				printf("\n");
				printf("    Scale: %ld, %ld\n", no_hscale, no_vscale);
				printf("    Pixel Hinting: %ld\n", pixel_hinting);
				printf("    No Close: %ld\n", no_close);
			}
		}

		if(has_fill) {
			/* this is a sub-style in a line definition! */
fprintf(stderr, "Fill inside line not supported yet...\n");
exit(1);
		}
		else {
			if(use_rgba) {
				if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
					return -1;
				}
				if(script) {
					printf("; color { %ld, %ld, %ld, %ld }", red, green, blue, alpha);
				}
				else {
					printf("    Solid: (%ld, %ld, %ld, %ld)\n", red, green, blue, alpha);
				}
				if(two_objects) {	/* NOTE: use_rgba is always true when two_objects is true */
					if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
						return -1;
					}
					if(script) {
						printf("; color { %ld, %ld, %ld, %ld }", red, green, blue, alpha);
					}
					else {
						printf("    Solid to morph to: (%ld, %ld, %ld, %ld)\n", red, green, blue, alpha);
					}
				}
			}
			else {
				if(getcolor(state, &red, &green, &blue, NULL) != 0) {
					return -1;
				}
				if(script) {
					printf("; color { %ld, %ld, %ld }", red, green, blue);
				}
				else {
					printf("    Solid: (%ld, %ld, %ld)\n", red, green, blue);
				}
			}
		}

		if(script) {
			printf(" };\n");
		}

		i++;
	}

	return 0;
}


int scan_shape(swf_dump_state *state, long max_styles, long use_rgba, long two_objects, long new_styles, long shape4)
{
	long		i, flags, size, value, x, y, script, mode;

/* read the # of bits used to read fill & line references */
	if(getbits(state, 4, &state->fill_bits, 0) != 0) {
		return -1;
	}
	if(getbits(state, 4, &state->line_bits, 0) != 0) {
		return -1;
	}
	printf("  Fill style bits: %ld, line style bits: %ld\n", state->fill_bits, state->line_bits);

	script = state->flags->object != -1;

	mode = 0;		/* no mode */
	i = 1;
	for(;;) {
		if(!new_styles) {
			/* in this case we don't have the NewStyles flag
			 * so we need some tricky readings...
			 */
			if(getbits(state, 5, &flags, 0) != 0) {
				return -1;
			}
			if((flags & 0x10) != 0) {
				if(getbits(state, 1, &value, 0) != 0) {
					return -1;
				}
				flags = flags * 2 + value;
			}
			else {
				flags = ((flags & 0x10) << 1) + (flags & 0x0F);
			}
		}
		else {
			if(getbits(state, 6, &flags, 0) != 0) {
				return -1;
			}
		}

/* last shape found? */
		if(flags == 0) {
			if(script && mode == 1) {
				/* close the last edges ... */
				printf("\t};\n");
			}
			return 0;
		}

		if(!script) {
			printf("  Edge #%ld (%s) flags: 0x%02lX\n", i, (flags & 0x20) == 0 ? "setup" : "point", flags << 2);
		}

/* read a new setup (first bit == 0) or read an edge (first bit == 1) */
		if((flags & 0x20) == 0) {
			/* read a new setup - we already have the 5 flags to test whether such and such entry is available */
			if(script && mode == 1) {
				/* close the last edges ... */
				printf("\t};\n");
				mode = 0;
			}
			if((flags & 0x01) != 0) {	/* Has move to */
				if(getbits(state, 5, &size, 0) != 0) {
					return -1;
				}
				if(getbits(state, size, &x, 1) != 0) {
					return -1;
				}
				if(getbits(state, size, &y, 1) != 0) {
					return -1;
				}
				if(script) {
					printf("\tmove: %g, %g;\n", x / 20.0, y / 20.0);
				}
				else {
					printf("    Move: origin + (%ld, %ld)\n", x, y);
				}
			}
			if((flags & 0x02) != 0) {	/* Has fill style 0 */
				if(getbits(state, state->fill_bits, &value, 0) != 0) {
					return -1;
				}
				if(script) {
					if(value == 0) {
						printf("\tfill0: no_fill;\n");
					}
					else {
						printf("\tfill0: f%ld;\n", value);
					}
				}
				else {
					printf("    Fill style #0: %ld\n", value);
				}
			}
			if((flags & 0x04) != 0) {	/* Has fill style 1 */
				if(getbits(state, state->fill_bits, &value, 0) != 0) {
					return -1;
				}
				if(script) {
					if(value == 0) {
						printf("\tfill1: no_fill;\n");
					}
					else {
						printf("\tfill1: f%ld;\n", value);
					}
				}
				else {
					printf("    Fill style #1: %ld\n", value);
				}
			}
			if((flags & 0x08) != 0) {	/* Has line style */
				if(getbits(state, state->line_bits, &value, 0) != 0) {
					return -1;
				}
				if(script) {
					if(value == 0) {
						printf("\tno_line;\n");
					}
					else {
						printf("\tl%ld;\n", value);
					}
				}
				else {
					printf("    Line style: %ld\n", value);
				}
			}
			if((flags & 0x10) != 0) {	/* Has new styles */
				/* new styles! */
				if(!script) {
					printf("    New style setup:\n");
				}
				/* TODO: we would need to have a better indentation for this one! (like many others since many things can be encapsulated) */
				if(scan_styles(state, max_styles, use_rgba, two_objects, shape4) != 0) {
					return -1;
				}
				/* read the # of bits used to read fill & line references */
				if(getbits(state, 4, &state->fill_bits, 0) != 0) {
					return -1;
				}
				if(getbits(state, 4, &state->line_bits, 0) != 0) {
					return -1;
				}
				printf("  Fill style bits: %ld, line style bits: %ld\n", state->fill_bits, state->line_bits);
			}
		}
		else {
			if(script && mode != 1) {
				printf("\tedges {\n");
				mode = 1;
			}
			size = (flags & 0x0F) + 2;
			if((flags & 0x10) == 0) {	/* 0 - Curve, 1 - Straight */
				if(getbits(state, size, &x, 1) != 0) {
					return -1;
				}
				if(getbits(state, size, &y, 1) != 0) {
					return -1;
				}
				if(script) {
					printf("\t\t%g, %g,", x / 20.0, y / 20.0);
				}
				else {
					printf("    Control: origin + (%ld, %ld)\n", x, y);
				}
				if(getbits(state, size, &x, 1) != 0) {
					return -1;
				}
				if(getbits(state, size, &y, 1) != 0) {
					return -1;
				}
				if(script) {
					printf(" %g, %g;\n", x / 20.0, y / 20.0);
				}
				else {
					printf("    Anchor: origin + (%ld, %ld)\n", x, y);
				}
			}
			else {
				if(getbits(state, 1, &flags, 0) != 0) {
					return -1;
				}
				if(flags == 1) {
					if(getbits(state, size, &x, 1) != 0) {
						return -1;
					}
					if(getbits(state, size, &y, 1) != 0) {
						return -1;
					}
				}
				else {
					if(getbits(state, 1, &flags, 0) != 0) {
						return -1;
					}
					if(flags == 0) {
						if(getbits(state, size, &x, 1) != 0) {
							return -1;
						}
						y = 0;
					}
					else {
						if(getbits(state, size, &y, 1) != 0) {
							return -1;
						}
						x = 0;
					}
				}
				if(script) {
					printf("\t\t%g, %g;\n", x / 20.0, y / 20.0);
				}
				else {
					printf("    Delta: origin + (%ld, %ld)\n", x, y);
				}
			}
		}

		i++;
	}
	/*NOTREACHED*/
}


int scan_defineshape(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id, offset, max_styles, use_rgba, two_objects, new_styles, shape4, pad;
	long		scaling_strokes;
	swf_dump_rect	rect;

	max_styles = 0x7FFF;		/* there is a list of 15 bits to indicate this size at this time */
	use_rgba = 0;
	new_styles = 1;
	two_objects = 0;
	shape4 = 0;
	switch(tag->tag) {
	case 2:		/* DefineShape */
		max_styles = 255;
		new_styles = 1;
		break;

	/* case 22:	-- DefineShape2 *** this object uses the defaults */

	case 32:	/* DefineShape3 */
		use_rgba = 1;
		break;

	case 46:	/* DefineMorphShape */
		use_rgba = 1;
		two_objects = 1;
		break;

	case 83:
		use_rgba = 1;
		shape4 = 1;
		break;

	}

	if(getword(state, &id) != 0) {
		return -1;
	}
	if(state->flags->object == id) {
		/* ha! that's to be printed out as an SSWF script */
		printf("shape \"name\" {\n");
		if(getrect(state, &rect) != 0) {
			return -1;
		}
		printf("\trectangle { %g, %g, %g, %g };\n",
			rect.x_min / 20.0, rect.y_min / 20.0, rect.x_max / 20.0, rect.y_max / 20.0);
		if(two_objects) {
			if(getrect(state, &rect) != 0) {
				return -1;
			}
			printf("\trectangle { %g, %g, %g, %g };\n",
				rect.x_min / 20.0, rect.y_min / 20.0, rect.x_max / 20.0, rect.y_max / 20.0);
			if(getlong(state, &offset) != 0) {
				return -1;
			}
			/* don't print out this offset, it's generated */
		}

		if(scan_styles(state, max_styles, use_rgba, two_objects, shape4) != 0) {
			return -1;
		}

		if(scan_shape(state, max_styles, use_rgba, two_objects, new_styles, shape4) != 0) {
			return -1;
		}
		/* not sure how the script would support the morph yet - it isn't done in there yet! */
		if(two_objects) {
			/* morph version needs a 2nd shape */
			if(scan_shape(state, max_styles, use_rgba, two_objects, new_styles, shape4) != 0) {
				return -1;
			}
		}
		printf("};\n\n");
		exit(0);
	}
	if(state->flags->object != -1) {
		/* we have to skip the data ourself in this case... */
		skipdata(state, tag->size - 2);
		return 0;
	}

	printf("  Object ID: #%ld (0x%04lX)\n", id, id);

	if(getrect(state, &rect) != 0) {
		return -1;
	}
	printf("  Placement & Size: (%ld, %ld) - (%ld, %ld)\n",
			rect.x_min, rect.y_min, rect.x_max, rect.y_max);

	if(two_objects) {
		/* reset the scanner position to the next byte */
		getdata(state, NULL, 0);

		if(getrect(state, &rect) != 0) {
			return -1;
		}
		printf("  2nd Placement & Size: (%ld, %ld) - (%ld, %ld)\n",
				rect.x_min, rect.y_min, rect.x_max, rect.y_max);
	}

	if(shape4) {
		/* reset the scanner position to the next byte */
		getdata(state, NULL, 0);

		if(getrect(state, &rect) != 0) {
			return -1;
		}
		printf("  Placement & Size for Stokes: (%ld, %ld) - (%ld, %ld)\n",
				rect.x_min, rect.y_min, rect.x_max, rect.y_max);

		if(two_objects) {
			/* reset the scanner position to the next byte */
			getdata(state, NULL, 0);

			if(getrect(state, &rect) != 0) {
				return -1;
			}
			printf("  2nd Placement & Size for Stokes: (%ld, %ld) - (%ld, %ld)\n",
					rect.x_min, rect.y_min, rect.x_max, rect.y_max);
		}

		/* get the size of each parameter */
		if(getbits(state, 6, &pad, 0) != 0) {
			return -1;
		}
		if(pad != 0) {
			printf("  Unknown style flags: 0x%02lX\n", pad << 2);
		}
		if(getbits(state, 2, &scaling_strokes, 0) != 0) {
			return -1;
		}
		switch(scaling_strokes) {
		case 0:
			printf("  No strokes are defined for scaling or non-scaling.\n");
			break;

		case 1:
			printf("  The lines have scaling strokes.\n");
			break;

		case 2:
			printf("  The lines have non-scaling strokes.\n");
			break;

		case 3:
			printf("  The lines have scaling and non-scaling strokes.\n");
			break;

		}
	}

	if(two_objects) {
		if(getlong(state, &offset) != 0) {
			return -1;
		}
		printf("  Offset to the 2nd shape data: %ld\n", offset);
	}

	if(scan_styles(state, max_styles, use_rgba, two_objects, shape4) != 0) {
		return -1;
	}

	if(scan_shape(state, max_styles, use_rgba, two_objects, new_styles, shape4) != 0) {
		return -1;
	}

	if(two_objects) {
		/* morph version needs a 2nd shape */

		/* reset the scanner position to the next byte */
		if(getdata(state, NULL, 0) != 0) {
			return -1;
		}

		if(scan_shape(state, max_styles, use_rgba, two_objects, new_styles, shape4) != 0) {
			return -1;
		}
	}

	return 0;
}


/* accepts both, lossless and lossless2 tags */
void swapRGB(char *rgb, long size)
{
	register char	c;

	while(size > 0) {
		size--;
		c = rgb[0];
		rgb[0] = rgb[2];
		rgb[2] = c;
		rgb += 3;
	}
}

void swapRGBA(char *rgba, long size)
{
	register char	c;

	while(size > 0) {
		size--;
		c = rgba[0];
		rgba[0] = rgba[2];
		rgba[2] = c;
		rgba += 4;
	}
}


int scan_definebitslossless(swf_dump_state *state, swf_dump_tag *tag)
{
	long		x, y, id, format, width, height, colormap_count, index;
	long		compressed_size, colormap_size, cnt, is_24;
	unsigned long	size;
	unsigned char	*s, *d, *image, *compressed, *uncompressed, *color, header[TGA_HEADER_SIZE];
	char		filename[256];
	FILE		*f;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld (0x%04lX)\n", id, id);

	if(getbyte(state, &format) != 0) {
		return -1;
	}
	printf("  Format: %s (%ld)%s\n", format == 3 ? "Paletted Image" :
			(format == 4 ? "RGB:5 image"
			: (format == 5 ? "RGB:8 image" : "unknown format")), format,
			tag->tag == 20 ? "" : " with alpha channel");

	if(getword(state, &width) != 0) {
		return -1;
	}
	if(getword(state, &height) != 0) {
		return -1;
	}
	printf("  Bitmap sizes: %ldx%ld\n", width, height);

	compressed_size = tag->size - 7;

	if(format == 3) {
		/* the size of the colormap */
		if(getbyte(state, &colormap_count) != 0) {
			return -1;
		}
		/* in this case the lines are adjusted to the nearest long word (32 bits) */
		colormap_count++;	/* the SWF saves 'count - 1' instead of 'count' */
		printf("  Colormap size: %ld\n", colormap_count);
		colormap_size = colormap_count * (tag->tag == 20 ? 3 : 4);
		size = colormap_size + ((width + 3) & -4) * height;
		compressed_size--;		/* the colormap_count byte! */
	}
	else {
		/* compute the output size */
		size = width * height * (format == 4 ? 2 : 4);
		colormap_size = 0;	/* mainly to avoid warnings - also a safeguard */
	}

	printf("  Byte size: %ld uncompressed and %ld compressed.\n", size, compressed_size);

	if(state->flags->decompress && format >= 3 && format <= 5) {
		/* allocate the buffer to hold the compressed and uncompressed data */
		uncompressed = malloc(size + compressed_size);
		if(uncompressed == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: out of memory, can't save image.\n");
			return -1;
		}
		compressed = uncompressed + size;
		/* read the compressed data */
		if(getdata(state, compressed, compressed_size) != 0) {
			free(uncompressed);
			return -1;
		}
		/* uncompress the SWF buffer */

{
int i;
printf("%ld bytes: ", compressed_size);
for(i = 0; i < compressed_size; i++) {
printf("%02X ", compressed[i]);
}
printf("\n");
}

		if(uncompress(uncompressed, &size, compressed, compressed_size) != Z_OK) {
			fflush(stdout);
			fprintf(stderr, "ERROR: can't properly decompress the loss-less image data.\n");
			free(uncompressed);
			return -1;
		}

		/* to make sure, we "decompress" images with a colormap and/or
		 * with only 16 bits so we have full control over them
		 */
		image = malloc(width * height * 4);
		if(image == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: out of memory, can't save image.\n");
			free(uncompressed);
			return -1;
		}

/* TODO: the images with an alpha channel have to restore the components
 *       since the alpha is otherwise pre-computed!
 */

		is_24 = tag->tag == 20;
		switch(format + tag->tag * 256) {
		case 3 + 20 * 256:		/* paletted RGB */
			d = image;
			s = uncompressed + colormap_size;
			for(y = 0; y < height; y++) {
				for(x = 0; x < width; x++, s++, d += 3) {
					index = *s;
					if(index >= colormap_count) {
						/* use a default internal color of bright RED when this happens */
						d[2] = 255;
						d[1] = 0;
						d[0] = 0;
					}
					else {
						color = uncompressed + index * 3;
						d[2] = color[0];
						d[1] = color[1];
						d[0] = color[2];
					}
				}
				/* adjust the source pointer as required */
				x &= 3;
				if(x != 0) {
					s += 4 - x;
				}
			}
			break;

		case 3 + 36 * 256:		/* paletted RGBA */
			d = image;
			s = uncompressed + colormap_size;
			for(y = 0; y < height; y++) {
				for(x = 0; x < width; x++, s++, d += 4) {
					index = *s;
					if(index >= colormap_count) {
						/* use a default internal color of bright RED with this happens */
						d[2] = 255;
						d[1] = 0;
						d[0] = 0;
						d[3] = 255;
					}
					else {
						color = uncompressed + index * 4;
						d[2] = color[0];
						d[1] = color[1];
						d[0] = color[2];
						d[3] = color[3];
					}
				}
				/* adjust the source pointer as required */
				x &= 3;
				if(x != 0) {
					s += 4 - x;
				}
			}
			break;

		case 4 + 20 * 256:		/* paletted RGB */
fflush(stdout);
fprintf(stderr, "WARNING: format = 4 - not tested yet\n");
			d = image;
			s = uncompressed;
			cnt = width * height;
			while(cnt > 0) {
				cnt--;
				index = s[0] + s[1] * 256;
				d[2] = (index >> 10) & 0x1F;
				d[1] = (index >> 5) & 0x1F;
				d[0] = index & 0x1F;
				d += 3;
				s += 2;
			}
			break;

		case 4 + 36 * 256:		/* paletted RGB */
fflush(stdout);
fprintf(stderr, "WARNING: format = 4 - not tested yet\n");
			d = image;
			s = uncompressed;
			cnt = width * height;
			while(cnt > 0) {
				cnt--;
				index = s[0] + s[1] * 256;
				d[2] = (index >> 10) & 0x1F;
				d[1] = (index >> 5) & 0x1F;
				d[0] = index & 0x1F;
				d[3] = (unsigned char) (index >> 15);
				d += 4;
				s += 2;
			}
			break;

		case 5 + 20 * 256:
			d = image;
			s = uncompressed;
			cnt = width * height;
			while(cnt > 0) {
				cnt--;
				d[2] = s[1];
				d[1] = s[2];
				d[0] = s[3];
				d += 3;
				s += 4;
			}
			break;

		case 5 + 36 * 256:
			d = image;
			s = uncompressed;
			cnt = width * height;
			while(cnt > 0) {
				cnt--;
				d[0] = s[0];
				d[1] = s[3];
				d[2] = s[2];
				d[3] = s[1];
				d += 4;
				s += 4;
			}
			break;

		default:
			fflush(stdout);
			fprintf(stderr, "INTERNAL ERROR: format not found?!?\n");
			abort();

		}

		if(!is_24) {
			if(state->flags->tga24) {
				is_24 = 1;		/* user wants it anyway */
			}
			else {
				/* test the image to see if it makes use of the alpha channel */
				s = image;
				cnt = width * height;
				while(size > 0) {
					if(s[3] != 255) {
						break;
					}
					s += 4;
					size--;
				}
				is_24 = size == 0;
			}
			if(is_24) {
				/* ha! it doesn't need it */
				s = d = image;
				cnt = width * height;
				while(size > 0) {
					size--;
					d[0] = s[0];	/* keep only the RGB info */
					d[1] = s[1];
					d[2] = s[2];
					s += 4;
					d += 3;
				}
			}
		}

		/* create the file where the image is to be saved */
		snprintf(filename, sizeof(filename), "swf_dump-%ld.tga", id);
		f = fopen(filename, "wb");
		if(f == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: can't create dump image file \"%s\" (errno: %d).\n", filename, errno);
			free(image);
			free(uncompressed);
			return -1;
		}
		/* define common information in the header */
		memset(header, 0, sizeof(header));
		header[TGA_OFFSET_WIDTH_LO] = (unsigned char) width;
		header[TGA_OFFSET_WIDTH_HI] = (unsigned char) (width >> 8);
		header[TGA_OFFSET_HEIGHT_LO] = (unsigned char) height;
		header[TGA_OFFSET_HEIGHT_HI] = (unsigned char) (height >> 8);
		header[TGA_OFFSET_FLAGS] = 0x20 | (is_24 ? 0 : 8);	/* up side down (targa wise at least) - we could fix that... */
		header[TGA_OFFSET_IMAGE_TYPE] = 2;
		header[TGA_OFFSET_BITS_PER_PIXEL] = is_24 ? 24 : 32;
		if(fwrite(header, sizeof(header), 1, f) != 1) {
			fflush(stdout);
			fprintf(stderr, "ERROR: can't write header in image file.\n");
			fclose(f);
			free(image);
			free(uncompressed);
			return -1;
		}
		if(fwrite(image, width * height * (is_24 ? 3 : 4), 1, f) != 1) {
			fflush(stdout);
			fprintf(stderr, "ERROR: can't write header in image file.\n");
			fclose(f);
			free(image);
			free(uncompressed);
			return -1;
		}
		fclose(f);
		free(image);
		free(uncompressed);
		printf("  Image saved in: \"%s\".\n", filename);
	}
	else {
		/* if we don't read the data we need to skip it */
		skipdata(state, compressed_size);
	}

	return 0;
}


int scan_definetextfield(swf_dump_state *state, swf_dump_tag *tag)
{
	static const char	*flag_names[16] = {
		/* 1st byte */
		"HAS FONT",
		"HAS MAXLENGTH",
		"HAS COLOR",
		"READONLY",
		"PASSWORD",
		"MULTILINE",
		"WORD WRAP",
		"HAS TEXT",

		/* 2nd byte */
		"OUTLINE",
		"HTML",
		"RESERVED (0x0004)",
		"BORDER",
		"NO SELECT",
		"HAS LAYOUT",
		"AUTO-SIZE (V6.x)",
		"RESERVED (0x0080)",
	};
	swf_dump_rect	rect;
	long		id, flags, idx, ref, t, red, green, blue, alpha;
	char		*str;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);

	if(getrect(state, &rect) != 0) {
		return -1;
	}
	printf("  Bounding box: (%ld, %ld) - (%ld, %ld)\n",
			rect.x_min, rect.y_min, rect.x_max, rect.y_max);

/* get all the flags at once so we can read the info as it goes */
	/* WARNING: the flags are swapped in a word! */
	if(getword(state, &flags) != 0) {
		return -1;
	}

	/* first write a list of the flags which are ON */
	printf("  Flags (0x%02lX%02lX):", (flags >> 8) & 255, flags & 255);
	for(idx = 0; idx < 16; idx++) {
		if((flags & (1 << idx)) != 0) {
			printf(" %s", flag_names[idx]);
			idx++;
			break;
		}
	}
	while(idx < 16) {
		if((flags & (1 << idx)) != 0) {
			printf(", %s", flag_names[idx]);
		}
		idx++;
	}
	printf("\n");

	if((flags & 0x0001) != 0) {	/* 0x0100 in big endian */
		if(getword(state, &ref) != 0) {
			return -1;
		}
		printf("  Use font ID: #%ld\n", ref);
		if(getword(state, &t) != 0) {
			return -1;
		}
		printf("  Font height: %ld\n", t);
	}

	if((flags & 0x0004) != 0) {	/* 0x0400 in big endian */
		if(getcolor(state, &red, &green, &blue, &alpha) != 0) {
			return -1;
		}
		printf("  Color: (%ld, %ld, %ld, %ld)\n", red, green, blue, alpha);
	}

	if((flags & 0x0002) != 0) {	/* 0x0200 in big endian */
		if(getword(state, &t) != 0) {
			return -1;
		}
		printf("  Maximum length: %ld\n", t);
	}

	if((flags & 0x2000) != 0) {	/* 0x0020 in big endian */
		if(getbyte(state, &t) != 0) {
			return -1;
		}
		printf("  Alignment: %ld\n", t);
		if(getword(state, &t) != 0) {
			return -1;
		}
		printf("  Left margin: %ld\n", t);
		if(getword(state, &t) != 0) {
			return -1;
		}
		printf("  Right margin: %ld\n", t);
		if(getword(state, &t) != 0) {
			return -1;
		}
		printf("  Indent: %ld\n", t);
		if(getword(state, &t) != 0) {
			return -1;
		}
		printf("  Leading: %ld\n", t);
	}

	if(getstring(state, &str) != 0) {
		return -1;
	}
	printf("  Variable name: \"%s\"\n", str);
	free(str);

	if((flags & 0x0080) != 0) {	/* 0x8000 in big endian */
		if(getstring(state, &str) != 0) {
			return -1;
		}
		printf("  Initial text: \"%s\"\n", str);
		free(str);
	}

	return 0;
}


int scan_definesprite(swf_dump_state *state, swf_dump_tag *tag)
{
	long		r, id, count, start, current;

	start = (state->bufpos + 7) / 8;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);

	if(getword(state, &count) != 0) {
		return -1;
	}
	printf("  Frames count: %ld\n", count);

	printf("************** SPRITE TAGS START **************\n");
	do {
		r = scan(state);
		if(r < 0) {
			return -1;
		}
		current = (state->bufpos + 7) / 8;
	} while(r == 0 && current - start < tag->size);
	printf("++++++++++++++ SPRITE TAGS END ++++++++++++++++\n");

	return 0;
}


int scan_productinfo(swf_dump_state *state, swf_dump_tag *tag)
{
	long		value, high, low;

	if(getlong(state, &value) != 0) {
		return -1;
	}
	printf("  Product identifier: %ld (0x%08lX)\n", value, value);

	if(getlong(state, &value) != 0) {
		return -1;
	}
	printf("  Product edition: %ld (0x%08lX)\n", value, value);

	if(getbyte(state, &value) != 0) {
		return -1;
	}
	printf("  Major version: %ld (0x%08lX)\n", value, value);

	if(getbyte(state, &value) != 0) {
		return -1;
	}
	printf("  Minor version: %ld (0x%08lX)\n", value, value);

	if(getlong(state, &low) != 0) {
		return -1;
	}
	if(getlong(state, &high) != 0) {
		return -1;
	}
	printf("  Build number: 0x%08lX:%08lX\n", high, low);

	/* TODO: once we know of the correct format... */
	if(getlong(state, &low) != 0) {
		return -1;
	}
	if(getlong(state, &high) != 0) {
		return -1;
	}
	printf("  Compile date: 0x%08lX:%08lX\n", high, low);

	return 0;
}


int scan_framelabel(swf_dump_state *state, swf_dump_tag *tag)
{
	char		*label;
	long		size, flags;

	if(getstring(state, &label) != 0) {
		return -1;
	}
	printf("  Label: \"%s\"\n", label);
	size = tag->size - strlen(label) - 1;
	free(label);

	if(size > 0) {		/* size should be 1 */
		if(getbyte(state, &flags) != 0) {
			return -1;
		}
		printf("  Flags: 0x%02lX -", flags);
		if((flags & 1) != 0) {
			printf(" ANCHOR");
		}
		printf("\n");
	}

	return 0;
}


/* uses the scan_defineshape
int scan_definemorphshape(swf_dump_state *state, swf_dump_tag *tag)
{
	return scan_undefined(state, tag);
}
*/



int scan_definefont2(swf_dump_state *state, swf_dump_tag *tag)
{
	swf_dump_rect	rect;
	long		id, flags, c, l, idx, count, p, skip, oc;
	char		name[256], *ctrl;
	unsigned long	*offsets;
	unsigned short	*small_offsets;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Object ID: #%ld\n", id);

	if(getbyte(state, &flags) != 0) {
		return -1;
	}

	printf("  Font flags: 0x%02lX\n", flags);

	if((flags & 0x80) != 0) {
		printf("  Font has a layout.\n");
	}
	if((flags & 0x40) != 0) {
		printf("  Font uses SHIFTJIS encoding.\n");
	}
	if((flags & 0x20) != 0) {
		printf("  Font uses UNICODE encoding.\n");
	}
	if((flags & 0x10) != 0) {
		printf("  Font uses ANSII encoding.\n");
	}
	/* 0x08 - use wide offsets, that's not really an important info to the end user */
	if(state->flags->verbose > 1) {
		if((flags & 0x08) != 0) {
			printf("  Font uses wide offsets (large font!).\n");
		}
	}
	if((flags & 0x04) != 0) {
		printf("  Font uses wide characters (support up to 65536 characters).\n");
	}
	if((flags & 0x02) != 0) {
		printf("  Font is ITALIC.\n");
	}
	if((flags & 0x01) != 0) {
		printf("  Font is BOLD.\n");
	}

	if(getbyte(state, &c) != 0) {
		return -1;
	}
	print_language(c);

	if(getbyte(state, &l) != 0) {
		return -1;
	}
	if(l == 0) {
		printf("  Font doesn't have a name.\n");
	}
	else {
		if(getdata(state, name, l) != 0) {
			return -1;
		}
		name[l] = '\0';
		printf("  Font is named: \"%s\".\n", name);
	}

	if(getword(state, &count) != 0) {
		return -1;
	}

	printf("  Font defines %ld glyph%s.\n", count, count > 1 ? "s" : "");

	if(count > 0) {
		p = state->bufpos;		/* offsets are defined to this table position */
		offsets = malloc(count * sizeof(unsigned long));
		if(offsets == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: out of memory.\n");
			return -1;
		}
		l = count * ((flags & 0x08) == 0 ? sizeof(unsigned short) : sizeof(unsigned long));
		if(getdata(state, offsets, l) != 0) {
			free(offsets);
			return -1;
		}
		if((flags & 0x08) == 0) {
			/* we've got small offsets, expand them quickly */
			small_offsets = (unsigned short *) offsets;
			idx = count;
			while(idx > 0) {
				idx--;
				offsets[idx] = small_offsets[idx];
			}
		}
		/* the first offset should be equal to l - if not, warn about it */
		idx = offsets[0] - l;
		if(idx < 0) {
			fflush(stdout);
			fprintf(stderr, "ERROR: invalid 1st offset in a DefineFont2 tag. It is smaller than the table size.\n");
			free(offsets);
			return -1;
		}
		else if(idx != ((flags & 0x08) == 0 ? sizeof(unsigned short) : sizeof(unsigned long))) {
			if(idx == 0 && (flags & 0x80) == 0) {
				printf("  WARNING: font 1st offset and count are equal -- total size not available (but there is also no layout)\n");
			}
			else {
				printf("  ERROR: font 1st offset and count mismatch of %ld bytes.\n", idx);
				/*
				 * when this happens, idx will usually be 2 so we don't implement
				 * a heavy loop with new lines etc.
				 */
				if(idx > 0) {
					/* print out the data if it's not zero */
					printf("   ");
					while(idx > 0) {
						idx--;
						if(getbyte(state, &skip) != 0) {
							free(offsets);
							return -1;
						}
						printf(" %02lX", skip);
					}
					printf("\n");
				}
			}
		}
		else {
			/* 2 or 4 bytes (offset to the map) */
			while(idx > 0) {
				idx--;
				if(getbyte(state, &skip) != 0) {
					free(offsets);
					return -1;
				}
			}
		}
		/* we reached the shape array, read it now */
		for(idx = 0; idx < count; idx++) {
			printf("  Font glyph #%ld (offset from start of file: %ld)\n", idx, state->bufpos);
			/* verify the offset */
			l = offsets[idx] - (state->bufpos - p) / 8;
			if(l < 0) {
				fflush(stdout);
				fprintf(stderr, "ERROR: an offset seems to define two overlapping shapes.\n");
			}
			else if(l > 0) {
				fflush(stdout);
				fprintf(stderr, "WARNING: an offset is defining a new shape %ld bytes after the end of the previous shape.\n   ", l);
				/* this should NEVER occur... we'll see */
				while(l > 0) {
					l--;
					if(getbyte(state, &skip) != 0) {
						free(offsets);
						return -1;
					}
					fflush(stdout);
					fprintf(stderr, " %02lX", skip);
				}
				fflush(stdout);
				fprintf(stderr, "\n");
			}
			if(scan_shape(state, 0x7FFF, 0, 0, 1, 0) != 0) {
				free(offsets);
				return -1;
			}
			/* align the read pointer to next byte (probably not necessary but just in case...) */
			getdata(state, NULL, 0);
		}
		free(offsets);

		/* now we reached the map */
		if((flags & 4) == 0) {
			for(idx = 0; idx < count; idx++) {
				if(getbyte(state, &c) != 0) {
					return -1;
				}
				oc = c;
				if(c < 0x20) {
					ctrl = "^";
					c += '@';
				}
				else if(c == 0x7F) {
					ctrl = "^";
					c = '?';
				}
				else if(c >= 0x80 && c <= 0xA0) {
					ctrl = "~";
					c += '@' - 0x80;
				}
				else {
					ctrl = "";
				}
				printf("  Font glyph #%03ld maps to \"%s%c\" (0x%02lX).\n", idx, ctrl, (char) c, oc);
			}
		}
		else {
			for(idx = 0; idx < count; idx++) {
				if(getword(state, &c) != 0) {
					return -1;
				}
				oc = c;
				if(c < 0x20) {
					ctrl = "^";
					c += '@';
				}
				else if(c == 0x7F) {
					ctrl = "^";
					c = '?';
				}
				else if(c >= 0x80 && c <= 0xA0) {
					ctrl = "~";
					c += (long) '@' - 0x80;
				}
				else if(c >= 0x100) {
					/* at this time I assume that the output can't support wide characters as is */
					snprintf(name, sizeof(name), "0x%04lX", c);
					ctrl = name;
					c = '.';
				}
				else {
					ctrl = "";
				}
				printf("  Font glyph #%03ld maps to \"%s%c\" (0x%04lX).\n", idx, ctrl, (char) c, oc);
			}
		}
	}
	else {
		/* whatever happens, there is always at least one offset in a font definition (?) */
		if(getword(state, &c) != 0) {
			return -1;
		}
	}

/* display the layout? */
	if((flags & 0x80) != 0) {
		if(getword(state, &l) != 0) {
			return -1;
		}
		printf("  Font ascent: %d\n", (signed short) l);

		if(getword(state, &l) != 0) {
			return -1;
		}
		printf("  Font descent: %d\n", (signed short) l);

		if(getword(state, &l) != 0) {
			return -1;
		}
		printf("  Font leading height: %d\n", (signed short) l);

		/* read all the advance words */
		printf("  Font advance:\n");
		for(idx = 0; idx < count; idx++) {
			if((idx & 15) == 0) {
				printf("    %04lX-", idx);
			}
			if(getword(state, &l) != 0) {
				return -1;
			}
			printf(" %3d", (signed short) l);
			if((idx & 15) == 15) {
				printf("\n");
			}
		}
		if((idx & 15) != 0) {
			printf("\n");
		}

		/* read all the bounding boxes */
		printf("  Font bounding boxes:\n");
		for(idx = 0; idx < count; idx++) {
			getdata(state, 0, 0);	/* align ourself */
			if(getrect(state, &rect) != 0) {
				return -1;
			}
			printf("    Glyph #%03ld bounding box: (%ld, %ld) - (%ld, %ld)\n",
					idx, rect.x_min, rect.y_min, rect.x_max, rect.y_max);
		}

		/* check whether there is some kerning */
		if(getword(state, &count) != 0) {
			return -1;
		}
		if(count == 0) {
			printf("  Font has no kerning.\n");
		}
		else {
			printf("  Font has %ld kerning%s.\n", count, count > 1 ? "s" : "");
			if((flags & 4) == 0) {
				for(idx = 0; idx < count; idx++) {
					printf("    Kerning \"");
					if(getbyte(state, &c) != 0) {
						printf(" -- invalid\n");
						return -1;
					}
					printf("%c", (char) c);
					if(getbyte(state, &c) != 0) {
						printf(" -- invalid\n");
						return -1;
					}
					printf("%c\" with an advance of ", (char) c);
					if(getword(state, &l) != 0) {
						printf(" -- invalid\n");
						return -1;
					}
					printf("%d\n", (signed short) l);
				}
			}
			else {
				for(idx = 0; idx < count; idx++) {
					printf("    Kerning \"");
					if(getword(state, &c) != 0) {
						printf(" -- invalid\n");
						return -1;
					}
					/* TODO: c is a 16 bits value */
					printf("%c", (char) c);
					if(getword(state, &c) != 0) {
						printf(" -- invalid\n");
						return -1;
					}
					/* TODO: c is a 16 bits value */
					printf("%c\" with an advance of ", (char) c);
					if(getword(state, &l) != 0) {
						printf(" -- invalid\n");
						return -1;
					}
					printf("%d\n", (signed short) l);
				}
			}
		}
	}

	return 0;
}




int scan_defineinfo(swf_dump_state *state, swf_dump_tag *tag)
{
	long		version;
	char		*comment;

	if(getlong(state, &version) != 0) {
		return -1;
	}
	printf("  Tool version: #%ld (0x%08lX)\n", version, version);

	if(getstring(state, &comment) != 0) {
		return -1;
	}
	printf("  Generator information: \"%s\"\n", comment);
	free(comment);

	return 0;
}



int scan_external_symbols(swf_dump_state *state, const char *which)
{
	long		id, count;
	char		*name;

	if(getword(state, &count) != 0) {
		return -1;
	}
	if(count == 0) {
		printf("  WARNING: empty \"%s\" tag.\n", which);
		return 0;
	}

	if(count == 1) {
		printf("  One object to %s.\n", which);
	}
	else {
		printf("  Number of objects to %s: #%ld\n", which, count);
	}

	while(count > 0) {
		count--;
		if(getword(state, &id) != 0) {
			return -1;
		}
		if(getstring(state, &name) != 0) {
			return -1;
		}
		printf("    Object: %5ld, \"%s\"\n", id, name);
		free(name);
	}

	return 0;
}


int scan_export(swf_dump_state *state, swf_dump_tag *tag)
{
	return scan_external_symbols(state, "export");
}



int scan_import(swf_dump_state *state, swf_dump_tag *tag)
{
	char		*url;

	if(getstring(state, &url) != 0) {
		return -1;
	}
	printf("  Import from: \"%s\"\n", url);
	free(url);

	return scan_external_symbols(state, "import");
}



int scan_doinitaction(swf_dump_state *state, swf_dump_tag *tag)
{
	long		id;

	if(getword(state, &id) != 0) {
		return -1;
	}
	printf("  Target Sprite ID: #%ld\n", id);

	return scan_doaction_offset(state, tag, 2, 1);
}







typedef int		(*sub_scan_func)(swf_dump_state *state, swf_dump_tag *tag);


typedef struct SWF_DUMP_SCANTAG {
	long		tag;		/* the tag number */
	long		version;	/* valid in this version or better */
	long		flags;		/* see below */
	const char *	name;		/* a name for end users */
	sub_scan_func	func;		/* function to call to read the tag contents */
} swf_dump_scantag;

#define	DUMP_FLAG_HAS_ID	0x00000001


swf_dump_scantag	alltags[] =
{
	{  0,  1, 0,			"End",			scan_end },
	{  1,  1, 0,			"ShowFrame",		scan_showframe },
	{  2,  1, DUMP_FLAG_HAS_ID,	"DefineShape",		scan_defineshape },
	{  4,  1, 0,			"PlaceObject",		scan_placeobject },
	{  5,  1, 0,			"RemoveObject",		scan_removeobject },
	{  6,  1, 0,			"DefineBits",		scan_definebits },
	{  7,  1, 0,			"DefineButton",		scan_definebutton },
	{  8,  1, 0,			"JPEGTables",		scan_jpegtables },
	{  9,  1, 0,			"SetBackgroundColor",	scan_setbackgroundcolor },
	{ 10,  1, DUMP_FLAG_HAS_ID,	"DefineFont",		scan_definefont },
	{ 11,  1, 0,			"DefineText",		scan_definetext },
	{ 12,  1, 0,			"DoAction",		scan_doaction },
	{ 13,  1, 0,			"DefineFontInfo",	scan_definefontinfo },
	{ 14,  2, 0,			"DefineSound",		scan_definesound },
	{ 15,  2, 0,			"StartSound",		scan_startsound },
	{ 17,  2, 0,			"DefineButtonSound",	scan_definebuttonsound },
	{ 18,  2, 0,			"SoundStreamHead",	scan_soundstreamhead },
	{ 19,  2, 0,			"SoundStreamBlock",	scan_soundstreamblock },
	{ 20,  2, 0,			"DefineBitsLossless",	scan_definebitslossless },
	{ 21,  2, 0,			"DefineBitsJPEG2",	scan_definebits },
	{ 22,  2, DUMP_FLAG_HAS_ID,	"DefineShape2",		scan_defineshape },
	{ 23,  2, 0,			"DefineButtonCxform",	scan_definebuttoncxform },
	{ 24, -2, 0,			"Protect",		scan_protect },
	{ 26,  3, 0,			"PlaceObject2",		scan_placeobject2 },
	{ 28,  3, 0,			"RemoveObject2",	scan_removeobject },
	{ 32,  3, DUMP_FLAG_HAS_ID,	"DefineShape3",		scan_defineshape },
	{ 33,  3, 0,			"DefineText2",		scan_definetext },
	{ 34,  3, 0,			"DefineButton2",	scan_definebutton },
	{ 35,  3, 0,			"DefineBitsJPEG3",	scan_definebits },
	{ 36,  3, 0,			"DefineBitsLossless2",	scan_definebitslossless },
	{ 37,  4, 0,			"DefineTextField",	scan_definetextfield },
	{ 39,  3, 0,			"DefineSprite",		scan_definesprite },
	{ 41,  3, 0,			"ProductInfo",		scan_productinfo },
	{ 43,  3, 0,			"FrameLabel",		scan_framelabel },
	{ 45,  3, 0,			"SoundStreamHead2",	scan_soundstreamhead },
	{ 46,  3, 0,			"DefineMorphShape",	scan_defineshape },
	{ 48,  3, 0,			"DefineFont2",		scan_definefont2 },
	{ 49,  4, 0,			"DefineInfo",		scan_defineinfo },
	{ 56,  5, 0,			"Export",		scan_export },
	{ 57,  5, 0,			"Import",		scan_import },
	{ 58,  5, 0,			"DebugProtect",		scan_protect },
	{ 59,  6, 0,			"DoInitAction",		scan_doinitaction },
	{ 60,  6, 0,			"DefineVideoStream",	scan_definevideostream },
	{ 61,  6, 0,			"VideoFrame",		scan_videoframe },
	{ 62,  4, 0,			"DefineFontInfo2",	scan_definefontinfo },
	{ 64,  6, 0,			"DebugProtect2",	scan_protect },
	{ 65,  7, 0,			"ScriptLimits",		scan_scriptlimits },
	{ 66,  7, 0,			"SetTabIndex",		scan_settabindex },
	{ 69,  8, 0,			"FileAttributes",	scan_fileattributes },
	{ 77,  8, 0,			"Metadata",		scan_metadata },
	{ 78,  8, 0,			"DefineScalingGrid",	scan_definescalinggrid },
	{ 83,  8, DUMP_FLAG_HAS_ID,	"DefineShape4",		scan_defineshape },

	/* unknown tags are simply printed in hex. & ASCII on the screen */
	{ -1,  0, 0,			"Unknown",		scan_unknown }		/* end of the list of tags */
};




int scan(swf_dump_state *state)
{
	swf_dump_tag		tag;
	swf_dump_scantag	*t;
	long			offset, version;

	offset = (state->bufpos + 7) / 8;

	if(gettag(state, &tag) != 0) {
		return -1;
	}

	t = alltags;
	while(t->tag != -1) {
		if(t->tag == tag.tag) {
			break;
		}
		t++;
	}

	if(state->flags->object == -1) {
		printf("-------------- TAG: %s", t->name);
		if(state->flags->verbose > 0) {
			printf(" (%ld/0x%04lX)", tag.tag, tag.tag);
			if(t->version != 0) {
				version = labs(t->version);
				if(t->version < 0) {
					switch(tag.tag) {
					case 24:
						if(tag.size != 0) {
							version = 5;
						}
						break;

					}
				}
				printf(" V%ld.0", version);
			}
		}
		printf("\n");
		if(state->flags->verbose > 0) {
			printf("  Offset: %ld (0x%08lX)\n", offset, offset);
			printf("  Size: %ld%s\n", tag.size, tag.size > 62 ? " (large)" : "");
		}
		return (*t->func)(state, &tag);
	}

	/* the rest is tag specific */
	if((t->flags & DUMP_FLAG_HAS_ID) == 0) {
		skipdata(state, tag.size);
		return 0;
	}

	/* ha! the user has an ID so we need to let him take care of this */
	return (*t->func)(state, &tag);
}


int getheader(swf_dump_state *state)
{
	swf_dump_rect	rect;
	double		frame_rate;
	long		frame_count;
	unsigned long	file_size, sz;
	unsigned char	*compressed;

	/* read the header -- note that for compressed files we have to
	 * read the header with fread() instead of getdata() */
	if(fread(state->header, 1, 8, state->file) != 8) {
		fflush(stdout);
		fprintf(stderr, "ERROR: the file \"%s\" is not a Flash file.\n", state->filename);
		return -1;
	}
	state->bufpos = 8 * 8;		/* we are already at that position! */

	file_size = state->header[4] + (state->header[5] << 8) + (state->header[6] << 16) + (state->header[7] << 24);

	/* the magic code is written in little endian... starts with 'C' if compressed */
	if(state->header[2] == 'S' && state->header[1] == 'W' && state->header[0] == 'C') {
		/* compressed files need to be decompressed, we do it at once */
		sz = file_size * 11 / 10 + 256;
		if(sz > state->buf_size) {
			if(state->buffer != 0) {
				free(state->buffer);
			}
			state->buf_size = sz;
			state->buffer = malloc(sz);
		}
		compressed = malloc(state->buf_size);
		if(state->buffer == NULL || compressed == NULL) {
			fflush(stdout);
			fprintf(stderr, "ERROR: out of memory.\n");
			exit(1);
		}
		sz = fread(compressed, 1, sz, state->file);
		if(sz <= 0) {
			free(compressed);
			fflush(stdout);
			fprintf(stderr, "ERROR: can't read file (errno: %d)\n", errno);
			return -1;
		}
		state->max_bytes = state->buf_size;
		if(uncompress(state->buffer, (unsigned long *) &state->max_bytes, compressed, sz) != Z_OK) {
			free(compressed);
			fflush(stdout);
			fprintf(stderr, "ERROR: can't properly decompress the loss-less image data.\n");
			return -1;
		}
		free(compressed);
		if(state->max_bytes != file_size - 8) {
			fflush(stdout);
			fprintf(stderr, "WARNING: the decompressed file doesn't match the header size.\n");
		}
	}
	/* the magic code is written in little endian... */
	else if(state->header[2] != 'S' || state->header[1] != 'W' || state->header[0] != 'F') {
		fflush(stdout);
		fprintf(stderr, "ERROR: the file \"%s\" is not a Flash file.\n", state->filename);
		return -1;
	}
	/* we can't really test the version, only display it (see below) */

	/* read frame rectangle */
	if(getrect(state, &rect) != 0) {
		return -1;
	}
	/* read the number of frames per second */
	if(getfixed(state, 2, &frame_rate, 8) != 0) {	/* 2 bytes fixed value */
		return -1;
	}
	/* read the total number of frames */
	if(getword(state, &frame_count) != 0) {
		return -1;
	}

	if(state->flags->movie_header) {
		/* the user is only interested in the sizes */
		printf("%d, %ld, %ld, %g, %ld",
			state->header[3],
			rect.x_max - rect.x_min,
			rect.y_max - rect.y_min,
			frame_rate,
			frame_count);
		if(state->header[0] == 'C') {
			printf(", compressed");
		}
		printf("\n");
		exit(0);		/* we're done! */
	}

	/* print out the header now */
	if(state->flags->object == -1) {
		printf("============== Flash file \"%s\"\n", state->filename);
		printf("HEADER\n");
		printf("  Magic:       %c%c%c\n", state->header[2], state->header[1], state->header[0]);
		printf("  Version:     %d.0\n", (int) state->header[3]);
		printf("  File size:   %ld bytes\n", file_size);
		printf("  Frame rect.: (%ld, %ld) - (%ld, %ld) in TWIPS\n", rect.x_min, rect.y_min, rect.x_max, rect.y_max);
		printf("  Frame rate:  %g images per second\n", frame_rate);
		printf("  Frame count: %ld\n", frame_count);
	}

	return 0;
}


int parse(swf_dump_flags *flg, const char *filename)
{
	swf_dump_state	state;

	memset(&state, 0, sizeof(state));
	state.filename = filename;
	state.flags = flg;
	state.file = fopen(filename, "rb");
	if(state.file == NULL) {
		fflush(stdout);
		fprintf(stderr, "ERROR: can't open file \"%s\".\n", filename);
		return -1;
	}

	if(getheader(&state) == 0) {
		/* now we can scan the entire file, each entry is a set of tag + data */
		while(scan(&state) == 0);
	}

	fclose(state.file);

	return 0;
}



void usage(void)
{
fflush(stdout);
fprintf(stderr, "swf_dump v%s -- Copyright (c) 2002-2009 Made to Order Software Corp.\n", version);
fprintf(stderr, "\n");
fprintf(stderr, "Usage: swf_dump [-<opts>] <file> ...\n");
fprintf(stderr, " Where -<opts> is one or more of the following:\n");
fprintf(stderr, "   -d or --decompress     decompresses images\n");
fprintf(stderr, "   -h or --help           prints out this help screen\n");
fprintf(stderr, "   -m or --movie_header   only prints: version, width, height, frame rate, frame count; then exit\n");
fprintf(stderr, "   --no-alpha             save targa files in 24 bits only\n");
fprintf(stderr, "   -o or --object <id>    only object to be decompressed\n");
fprintf(stderr, "   -q or --quiet          make it quiet (no verbosity)\n");
fprintf(stderr, "   -v or --verbose        increase verbosity\n");
fprintf(stderr, "   --version              print out this tool version\n");
exit(1);
}


int main(int argc, char *argv[])
{
	int		i, j, len, errcnt, filename_only;
	swf_dump_flags	flg;

	memset(&flg, 0, sizeof(flg));
	flg.verbose = 1;
	flg.object = -1;		/* no specific object */
	filename_only = 0;
	errcnt = 0;
	i = 1;
	while(i < argc) {
		if(argv[i][0] == '-' && filename_only == 0) {
			if(argv[i][1] == '-') {
				/* this is a long name parameter */
				if(argv[i][2] == '\0') {
					/* ha! anything after that is a filename */
					filename_only = 1;
				}
				else if(strcmp(argv[i], "--decompress") == 0) {
					flg.decompress = 1;
				}
				else if(strcmp(argv[i], "--help") == 0) {
					usage();
				}
				else if(strcmp(argv[i], "--movie-header") == 0) {
					flg.movie_header = 1;
				}
				else if(strcmp(argv[i], "--noalpha") == 0) {
					flg.tga24 = 1;
				}
				else if(strcmp(argv[i], "--object") == 0) {
					i++;
					if(i >= argc) {
						fflush(stdout);
						fprintf(stderr, "ERROR: --object expects an ID (a value)\n");
						errcnt++;
					}
					flg.object = atol(argv[i]);
				}
				else if(strcmp(argv[i], "--quiet") == 0) {
					flg.verbose = 0;
				}
				else if(strcmp(argv[i], "--verbose") == 0) {
					flg.verbose++;
				}
				else if(strcmp(argv[i], "--version") == 0) {
					printf("%s\n", version);
					exit(0);
				}
				else {
					fflush(stdout);
					fprintf(stderr, "ERROR: don't know option \"%s\".\n", argv[i]);
					errcnt++;
				}
			}
			else {
				len = strlen(argv[i]);
				j = 1;
				while(j < len) {
					switch(argv[i][j]) {
					case 'd':
						flg.decompress = 1;
						break;

					case 'h':
						usage();
						break;

					case 'm':
						flg.movie_header = 1;
						break;

					case 'o':
						i++;
						if(i >= argc) {
							fflush(stdout);
							fprintf(stderr, "ERROR: -o expects an object ID (a value)\n");
							errcnt++;
						}
						flg.object = atol(argv[i]);
						break;

					case 'q':
						flg.verbose = 0;
						break;

					case 'v':
						flg.verbose++;
						break;

					default:
						fflush(stdout);
						fprintf(stderr, "ERROR: don't know option \"-%c\".\n", argv[i][j]);
						errcnt++;
						break;

					}
					j++;
				}
			}
		}
		else {
			/* if errors occured, we drop here */
			if(errcnt != 0) {
				break;
			}
			/* a filename, parse this entry */
			parse(&flg, argv[i]);
		}
		i++;
	}

	exit(errcnt == 0 ? 0 : 1);
	/*NOTREACHED*/
}

/* vim: ts=8
 */
