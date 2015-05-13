/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>

#include <pixelflinger/pixelflinger.h>

#include <png.h>

#include "minui.h"

extern char* locale;
// MStar Android Patch Begin
typedef int S8CPU;
/**
 *  Fast type for unsigned 8 bits. Use for parameter passing and local
 *  variables, not for storage
 */
typedef unsigned U8CPU;

/**
 *  Fast type for signed 16 bits. Use for parameter passing and local variables,
 *  not for storage
 */
typedef int S16CPU;

/**
 *  Fast type for unsigned 16 bits. Use for parameter passing and local
 *  variables, not for storage
 */
typedef unsigned U16CPU;

/**
 *  Meant to be faster than bool (doesn't promise to be 0 or 1,
 *  just 0 or non-zero
 */
typedef int RcyBool;

/**
 *  Meant to be a small version of bool, for storage purposes. Will be 0 or 1
 */
typedef uint8_t RcyBool8;

typedef uint32_t RcyPMColor;

#if 0
//#ifdef SK_CPU_BENDIAN
    #define RCY_R32_SHIFT    24
    #define RCY_G32_SHIFT    16
    #define RCY_B32_SHIFT    8
    #define RCY_A32_SHIFT    0
#else
    #define RCY_R32_SHIFT    0
    #define RCY_G32_SHIFT    8
    #define RCY_B32_SHIFT    16
    #define RCY_A32_SHIFT    24
#endif
// MStar Android Patch End

// MStar Android Patch Begin
/**
 *  Pack the components into a SkPMColor, checking (in the debug version) that
 *  the components are 0..255, and are already premultiplied (i.e. alpha >= color)
 */

static inline RcyPMColor RcyPackARGB32(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    return (a << RCY_A32_SHIFT) | (r << RCY_R32_SHIFT) |
              (g << RCY_G32_SHIFT) | (b << RCY_B32_SHIFT);
}
// MStar Android Patch End

// libpng gives "undefined reference to 'pow'" errors, and I have no
// idea how to convince the build system to link with -lm.  We don't
// need this functionality (it's used for gamma adjustment) so provide
// a dummy implementation to satisfy the linker.
double pow(double x, double y) {
    return x * y;
}

int res_create_surface(const char* name, gr_surface* pSurface) {
    char resPath[256];
    GGLSurface* surface = NULL;
    int result = 0;
    unsigned char header[8];
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;

    *pSurface = NULL;

    snprintf(resPath, sizeof(resPath)-1, "/res/images/%s.png", name);
    resPath[sizeof(resPath)-1] = '\0';
    FILE* fp = fopen(resPath, "rb");
    if (fp == NULL) {
        result = -1;
        goto exit;
    }
    // MStar Android Patch Begin
    /*
    size_t bytesRead = fread(header, 1, sizeof(header), fp);
    if (bytesRead != sizeof(header)) {
        result = -2;
        goto exit;
    }

    if (png_sig_cmp(header, 0, sizeof(header))) {
        result = -3;
        goto exit;
    }
    */
    // MStar Android Patch End
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        result = -4;
        goto exit;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        result = -5;
        goto exit;
    }

    // MStar Android Patch Begin
    png_init_io(png_ptr, fp);
    // png_set_sig_bytes(png_ptr, sizeof(header));
    png_read_info(png_ptr, info_ptr);

    size_t width = info_ptr->width;
    size_t height = info_ptr->height;
    size_t stride = 4 * width;
    size_t pixelSize = stride * height;
    // MStar Android Patch End

    int color_type = info_ptr->color_type;
    int bit_depth = info_ptr->bit_depth;
    int channels = info_ptr->channels;
    // MStar Android Patch Begin
    /* tell libpng to strip 16 bit/color files down to 8 bits/color */
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }
    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
     * byte into separate bytes (useful for paletted and grayscale images). */
    if (bit_depth < 8) {
        png_set_packing(png_ptr);
    }
    /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_gray_1_2_4_to_8(png_ptr);
    }

    /* Make a grayscale image into RGB. */
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    if (!(bit_depth == 8 &&
          ((channels == 3 && color_type == PNG_COLOR_TYPE_RGB) ||
           (channels == 4 && color_type == PNG_COLOR_TYPE_RGBA) ||
           (channels == 1 && color_type == PNG_COLOR_TYPE_PALETTE) ||
           (channels == 1 && color_type == PNG_COLOR_TYPE_GRAY)))) {
        return -7;
        goto exit;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        result = -6;
        goto exit;
    }
    // MStar Android Patch End

    surface = malloc(sizeof(GGLSurface) + pixelSize);
    if (surface == NULL) {
        result = -8;
        goto exit;
    }
    unsigned char* pData = (unsigned char*) (surface + 1);
    surface->version = sizeof(GGLSurface);
    surface->width = width;
    surface->height = height;
    surface->stride = width; /* Yes, pixels, not bytes */
    surface->data = pData;
    // MStar Android Patch Begin
    surface->format = GGL_PIXEL_FORMAT_RGBX_8888;

    int alpha = 0;
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    png_color_16p   transpColor = NULL;
    int             numTransp = 0;
    RcyPMColor      theTranspColorp;
    bool                hasAlphap;

    png_get_tRNS(png_ptr, info_ptr, NULL, &numTransp, &transpColor);

    bool valid = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);
    if (valid){
        png_set_tRNS_to_alpha(png_ptr);
        alpha = 1;
    }

    if (valid && numTransp == 1 && transpColor != NULL){
        /*  Compute our transparent color, which we'll match against later.
            We don't really handle 16bit components properly here, since we
            do our compare *after* the values have been knocked down to 8bit
            which means we will find more matches than we should. The real
            fix seems to be to see the actual 16bit components, do the
            compare, and then knock it down to 8bits ourselves.
        */
        if (color_type & PNG_COLOR_MASK_COLOR){
            if (16 == bit_depth) {
                theTranspColorp = RcyPackARGB32(0xFF, transpColor->red >> 8,
                          transpColor->green >> 8, transpColor->blue >> 8);
            } else {
                theTranspColorp = RcyPackARGB32(0xFF, transpColor->red,
                                  transpColor->green, transpColor->blue);
            }
        } else {
            if (16 == bit_depth) {
                theTranspColorp = RcyPackARGB32(0xFF, transpColor->gray >> 8,
                          transpColor->gray >> 8, transpColor->gray >> 8);
            } else {
                theTranspColorp = RcyPackARGB32(0xFF, transpColor->gray,
                                      transpColor->gray, transpColor->gray);
            }
        }
    }

    int y;
    if (channels == 3 || (channels == 1 && !alpha)) {
        for (y = 0; y < height; ++y) {
            unsigned char* pRow = pData + y * stride;
            png_read_row(png_ptr, pRow, NULL);

            int x;
            for(x = width - 1; x >= 0; x--) {
                int sx = x * 3;
                int dx = x * 4;
                unsigned char r = pRow[sx];
                unsigned char g = pRow[sx + 1];
                unsigned char b = pRow[sx + 2];
                unsigned char a = 0xff;
                pRow[dx    ] = r;
                pRow[dx + 1] = g;
                pRow[dx + 2] = b;
                pRow[dx + 3] = a;
            }
        }
    } else if (channels == 4) {
        for (y = 0; y < height; ++y) {
            unsigned char* pRow = pData + y * stride;
            png_read_row(png_ptr, pRow, NULL);
            unsigned char e;
            int x;
            for(x = width - 1; x >= 0; x--) {
                int sx = x * 3;
                int dx = x * 4;
                unsigned char r = pRow[dx];
                unsigned char g = pRow[dx + 1];
                unsigned char b = pRow[dx + 2];
                unsigned char a = 0xff;
                pRow[dx    ] = r;
                pRow[dx + 1] = g;
                pRow[dx + 2] = b;
                pRow[dx + 3] = a;
            }
        }
    // MStar Android Patch End
    } else {
        for (y = 0; y < height; ++y) {
            unsigned char* pRow = pData + y * stride;
            png_read_row(png_ptr, pRow, NULL);
        }
    }

    *pSurface = (gr_surface) surface;

exit:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    if (fp != NULL) {
        fclose(fp);
    }
    if (result < 0) {
        if (surface) {
            free(surface);
        }
    }
    return result;
}

static int matches_locale(const char* loc) {
    if (locale == NULL) return 0;

    if (strcmp(loc, locale) == 0) return 1;

    // if loc does *not* have an underscore, and it matches the start
    // of locale, and the next character in locale *is* an underscore,
    // that's a match.  For instance, loc == "en" matches locale ==
    // "en_US".

    int i;
    for (i = 0; loc[i] != 0 && loc[i] != '_'; ++i);
    if (loc[i] == '_') return 0;

    return (strncmp(locale, loc, i) == 0 && locale[i] == '_');
}

int res_create_localized_surface(const char* name, gr_surface* pSurface) {
    char resPath[256];
    GGLSurface* surface = NULL;
    int result = 0;
    unsigned char header[8];
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;

    *pSurface = NULL;

    snprintf(resPath, sizeof(resPath)-1, "/res/images/%s.png", name);
    resPath[sizeof(resPath)-1] = '\0';
    FILE* fp = fopen(resPath, "rb");
    if (fp == NULL) {
        result = -1;
        goto exit;
    }

    size_t bytesRead = fread(header, 1, sizeof(header), fp);
    if (bytesRead != sizeof(header)) {
        result = -2;
        goto exit;
    }

    if (png_sig_cmp(header, 0, sizeof(header))) {
        result = -3;
        goto exit;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        result = -4;
        goto exit;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        result = -5;
        goto exit;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        result = -6;
        goto exit;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sizeof(header));
    png_read_info(png_ptr, info_ptr);

    size_t width = info_ptr->width;
    size_t height = info_ptr->height;
    size_t stride = 4 * width;

    int color_type = info_ptr->color_type;
    int bit_depth = info_ptr->bit_depth;
    int channels = info_ptr->channels;

    if (!(bit_depth == 8 &&
          (channels == 1 && color_type == PNG_COLOR_TYPE_GRAY))) {
        return -7;
        goto exit;
    }

    unsigned char* row = malloc(width);
    int y;
    for (y = 0; y < height; ++y) {
        png_read_row(png_ptr, row, NULL);
        int w = (row[1] << 8) | row[0];
        int h = (row[3] << 8) | row[2];
        int len = row[4];
        char* loc = row+5;

        if (y+1+h >= height || matches_locale(loc)) {
            printf("  %20s: %s (%d x %d @ %d)\n", name, loc, w, h, y);

            surface = malloc(sizeof(GGLSurface));
            if (surface == NULL) {
                result = -8;
                goto exit;
            }
            unsigned char* pData = malloc(w*h);

            surface->version = sizeof(GGLSurface);
            surface->width = w;
            surface->height = h;
            surface->stride = w; /* Yes, pixels, not bytes */
            surface->data = pData;
            surface->format = GGL_PIXEL_FORMAT_A_8;

            int i;
            for (i = 0; i < h; ++i, ++y) {
                png_read_row(png_ptr, row, NULL);
                memcpy(pData + i*w, row, w);
            }

            *pSurface = (gr_surface) surface;
            break;
        } else {
            int i;
            for (i = 0; i < h; ++i, ++y) {
                png_read_row(png_ptr, row, NULL);
            }
        }
    }

exit:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    if (fp != NULL) {
        fclose(fp);
    }
    if (result < 0) {
        if (surface) {
            free(surface);
        }
    }
    return result;
}

void res_free_surface(gr_surface surface) {
    GGLSurface* pSurface = (GGLSurface*) surface;
    if (pSurface) {
        free(pSurface);
    }
}
