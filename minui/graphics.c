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

#include <stdbool.h>
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

// MStar Android Patch Begin
#include "minui.h"
const unsigned cw_en=10;
#include "font_10x18_cn.h"
// MStar Android Patch End

// MStar Android Patch Begin
#ifdef USE_FBDEV
#include "fbdev_recovery.h"
#endif
// MStar Android Patch End

#if defined(RECOVERY_BGRA)
// MStar Android Patch Begin
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGBA_8888
// MStar Android Patch End

#define PIXEL_SIZE   4
#elif defined(RECOVERY_RGBX)
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGBX_8888
#define PIXEL_SIZE   4
#else
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGB_565
#define PIXEL_SIZE   2
#endif

#define NUM_BUFFERS 2

// MStar Android Patch Begin
typedef struct {
    GGLSurface texture;
    unsigned cwidth;
    unsigned cheight;
    unsigned ascent;
} GRFont;

static GRFont *gr_font = 0;
static GGLContext *gr_context = 0;
static GGLSurface gr_font_texture;
static GGLSurface gr_framebuffer[NUM_BUFFERS];
static GGLSurface gr_mem_surface;
static unsigned gr_active_fb = 0;
static unsigned double_buffering = 0;
// MStar Android Patch End

static int gr_fb_fd = -1;
// MStar Android Patch Begin
// We don't support kernel framebuffer devivce.
#ifndef USE_FBDEV
static int gr_vt_fd = -1;
#endif
// MStar Android Patch End

static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;

// MStar Android Patch Begin
int getUNICharID(unsigned short unicode) {
    int i;
    for (i = 0; i < UNICODE_NUM; i++) {
        if (unicode == unicodemap[i]) return i;
    }
    return -1;
}
// MStar Android Patch End

static int get_framebuffer(GGLSurface *fb)
{
    int fd;
    void *bits;

    // MStar Android Patch Begin
#ifdef USE_FBDEV
    fbdev_init();
    fd = fbdev_open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        perror("cannot open fb0");
        return -1;
    }

    if (fbdev_ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 vinfo");
        fbdev_close(fd);
        return -1;
    }
#else
    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        perror("cannot open fb0");
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }
#endif
    // MStar Android Patch End

    vi.bits_per_pixel = PIXEL_SIZE * 8;
    // MStar Android Patch Begin
    if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_RGBA_8888) {
    // MStar Android Patch End
      vi.red.offset     = 8;
      vi.red.length     = 8;
      vi.green.offset   = 16;
      vi.green.length   = 8;
      vi.blue.offset    = 24;
      vi.blue.length    = 8;
      vi.transp.offset  = 0;
      vi.transp.length  = 8;
    } else if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_RGBX_8888) {
      vi.red.offset     = 24;
      vi.red.length     = 8;
      vi.green.offset   = 16;
      vi.green.length   = 8;
      vi.blue.offset    = 8;
      vi.blue.length    = 8;
      vi.transp.offset  = 0;
      vi.transp.length  = 8;
    } else { /* RGB565*/
      vi.red.offset     = 11;
      vi.red.length     = 5;
      vi.green.offset   = 5;
      vi.green.length   = 6;
      vi.blue.offset    = 0;
      vi.blue.length    = 5;
      vi.transp.offset  = 0;
      vi.transp.length  = 0;
    }
    // MStar Android Patch Begin
#ifdef USE_FBDEV
    if (fbdev_ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("failed to put fb0 vinfo");
        fbdev_close(fd);
        return -1;
    }

    if (fbdev_ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb finfo");
        fbdev_close(fd);
        return -1;
    }

    bits = fbdev_mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        fbdev_close(fd);
        return -1;
    }
#else
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("failed to put fb0 info");
        close(fd);
        return -1;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }

    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        close(fd);
        return -1;
    }
#endif
    // MStar Android Patch End

    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length/PIXEL_SIZE;
    fb->data = bits;
    fb->format = PIXEL_FORMAT;
    memset(fb->data, 0, vi.yres * fi.line_length);

    fb++;

    /* check if we can use double buffering */
    // MStar Android Patch Begin
    /*
    if (vi.yres * fi.line_length * 2 > fi.smem_len)
        return fd;

    double_buffering = 1;
    */
    // MStar Android Patch End

    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length/PIXEL_SIZE;
    fb->data = (void*) (((unsigned) bits) + vi.yres * fi.line_length);
    fb->format = PIXEL_FORMAT;
    memset(fb->data, 0, vi.yres * fi.line_length);

    return fd;
}

static void get_memory_surface(GGLSurface* ms) {
  ms->version = sizeof(*ms);
  ms->width = vi.xres;
  ms->height = vi.yres;
  ms->stride = fi.line_length/PIXEL_SIZE;
  ms->data = malloc(fi.line_length * vi.yres);
  ms->format = PIXEL_FORMAT;
}

static void set_active_framebuffer(unsigned n)
{
// MStar Android Patch Begin
    if (n > 1) return;
    vi.yres_virtual = vi.yres * PIXEL_SIZE;
    vi.yoffset = n * vi.yres;
    vi.bits_per_pixel = PIXEL_SIZE * 8;
#ifdef USE_FBDEV
    if (fbdev_ioctl(gr_fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("active fb swap failed");
    }
#else
    if (ioctl(gr_fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("active fb swap failed");
    }
#endif
// MStar Android Patch End
}

void gr_flip(void)
{
    GGLContext *gl = gr_context;

    /* swap front and back buffers */
    // MStar Android Patch Begin
    gr_active_fb = (gr_active_fb + 1) & 1;
    // MStar Android Patch End

    /* copy data from the in-memory surface to the buffer we're about
     * to make active. */
    memcpy(gr_framebuffer[gr_active_fb].data, gr_mem_surface.data,
           fi.line_length * vi.yres);

    /* inform the display driver */
    set_active_framebuffer(gr_active_fb);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    GGLContext *gl = gr_context;
    GGLint color[4];
    color[0] = ((r << 8) | r) + 1;
    color[1] = ((g << 8) | g) + 1;
    color[2] = ((b << 8) | b) + 1;
    color[3] = ((a << 8) | a) + 1;
    gl->color4xv(gl, color);
}

int gr_measure(const char *s)
{
    return gr_font->cwidth * strlen(s);
}

void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}

// MStar Android Patch Begin
int gr_text(int x, int y, const char *s)
{
    GGLContext *gl = gr_context;
    GRFont *font = gr_font;
    unsigned off;
    const char *p;
    const char *tmp = "NO MANE";
    p = s;
    int id;
    int ret = -1;
    unsigned short unicode;
    y -= font->ascent;

    gl->bindTexture(gl, &font->texture);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    if (0 == strncmp(s,"  ",2)) {
        s = tmp;
    }

    while((off = *s++)) {
        if (off < 0x80){
            off -= 32;
            if (off < 96) {
                gl->texCoord2i(gl, (off * font->cwidth) - x, 0 - y);
                gl->recti(gl, x, y, x + font->cwidth, y + font->cheight);
            }
            x += cw_en;
        } else {
            char *gb = malloc(3);
            memset(gb, 0, 3);
            unsigned char *unicode_str = malloc(3);
            memset(unicode_str, 0, 3);
            gb[0] = off;
            gb[1] = *s;
            if (0 == strGB2Unicode(gb, unicode_str)) {
                printf("donot found Unicode\n");
                ret = 0;
                free(gb);
                free(unicode_str);
                break;
            }
            unicode =  unicode_str[0] * 0x100 + unicode_str[1];
            id = getUNICharID(unicode);
            if (id >= 0) {
                if ((x + font->cwidth) >= gr_fb_width()) {
                    free(gb);
                    free(unicode_str);
                    return x;
                 }
                gl->texCoord2i(gl, ((id % 96) * font->cwidth) - x, (id / 96 + 1) * font->cheight - y);
                gl->recti(gl, x, y, x + font->cwidth, y + font->cheight);
                x += font->cwidth;
            } else {
                printf("id<0\n");
                x += font->cwidth;
            }

            *s++;
            free(gb);
            free(unicode_str);
        }
        if (0 == ret) {
            break;
        }
    }

    if (0 == ret) {
        printf("UTF-8 to UNICODE\n");
        wchar_t uniBuf[30];
        memset(uniBuf,0,30*sizeof(wchar_t));
        char *tmp = malloc(3);
        UTF2Unicode(p,uniBuf);
        wchar_t *tmp1 = uniBuf;
        while (*tmp1) {
            if (*tmp1 < (unsigned char)0x80) {
                wcstombs(tmp,tmp1,2);
                off = *tmp;
                off -= 32;
                if (off < 96) {
                    if ((x + cw_en) >= gr_fb_width()) return x;
                    gl->texCoord2i(gl, (off * font->cwidth) - x, 0 - y);
                    gl->recti(gl, x, y, x + cw_en, y + font->cheight);
                }
                x += cw_en;
            } else {
                memset(tmp,0,3);
                wcstombs(tmp,tmp1,2);
                unicode = tmp[1] * 0x100 + tmp[0];
                id = getUNICharID(unicode);
                if (id >= 0) {
                    if ((x + font->cwidth) >= gr_fb_width()) {
                        free(tmp);
                        free(uniBuf);
                        return x;
                    }
                    gl->texCoord2i(gl, ((id % 96) * font->cwidth) - x, (id / 96 + 1) * font->cheight - y);
                    gl->recti(gl, x, y, x + font->cwidth, y + font->cheight);
                    x += font->cwidth;
                } else {
                    printf("id<0\n");
                    x += font->cwidth;
                }
            }
            tmp1++;
        }
        free(tmp);
        free(uniBuf);
    }
    return x;
}

void gr_texticon(int x, int y, gr_surface icon) {
    if (gr_context == NULL || icon == NULL) {
        return;
    }
    GGLContext* gl = gr_context;

    gl->bindTexture(gl, (GGLSurface*) icon);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    int w = gr_get_width(icon);
    int h = gr_get_height(icon);

    gl->texCoord2i(gl, -x, -y);
    gl->recti(gl, x, y, x+gr_get_width(icon), y+gr_get_height(icon));
}

void gr_fill(int x, int y, int w, int h)
{
    GGLContext *gl = gr_context;
    gl->disable(gl, GGL_TEXTURE_2D);
    gl->recti(gl, x, y, w, h);
}
// MStar Android Patch End

void gr_blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {
    // MStar Android Patch Begin
    if (gr_context == NULL) {
        return;
    }
    GGLContext *gl = gr_context;
    // MStar Android Patch End
    gl->bindTexture(gl, (GGLSurface*) source);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    gl->texCoord2i(gl, sx - dx, sy - dy);
    gl->recti(gl, dx, dy, dx + w, dy + h);
}

unsigned int gr_get_width(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->width;
}

unsigned int gr_get_height(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->height;
}

static void gr_init_font(void)
{
    // MStar Android Patch Begin
    GGLSurface *ftex;
    unsigned char *bits, *rle;
    unsigned char *in, data;

    gr_font = calloc(sizeof(*gr_font), 1);
    ftex = &gr_font->texture;

    bits = malloc(font.width * font.height);

    ftex->version = sizeof(*ftex);
    ftex->width = font.width;
    ftex->height = font.height;
    ftex->stride = font.width;
    ftex->data = (void*) bits;
    ftex->format = GGL_PIXEL_FORMAT_A_8;

    in = font.rundata;
    while((data = *in++)) {
        memset(bits, (data & 0x80) ? 255 : 0, data & 0x7f);
        bits += (data & 0x7f);
    }

    gr_font->cwidth = font.cwidth;
    gr_font->cheight = font.cheight;
    gr_font->ascent = font.cheight - 2;
    // MStar Android Patch End
}

int gr_init(void)
{
    gglInit(&gr_context);
    GGLContext *gl = gr_context;

    gr_init_font();
    // MStar Android Patch Begin
    // We don't support kernel framebuffer devivce.
#ifndef USE_FBDEV
    gr_vt_fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (gr_vt_fd < 0) {
        // This is non-fatal; post-Cupcake kernels don't have tty0.
        perror("can't open /dev/tty0");
    } else if (ioctl(gr_vt_fd, KDSETMODE, (void*) KD_GRAPHICS)) {
        // However, if we do open tty0, we expect the ioctl to work.
        perror("failed KDSETMODE to KD_GRAPHICS on tty0");
        gr_exit();
        return -1;
    }
#endif
    // MStar Android Patch End

    gr_fb_fd = get_framebuffer(gr_framebuffer);
    if (gr_fb_fd < 0) {
        gr_exit();
        return -1;
    }

    get_memory_surface(&gr_mem_surface);

    printf("framebuffer: fd %d (%d x %d)\n",
           gr_fb_fd, gr_framebuffer[0].width, gr_framebuffer[0].height);

        /* start with 0 as front (displayed) and 1 as back (drawing) */
    gr_active_fb = 0;
    set_active_framebuffer(0);
    gl->colorBuffer(gl, &gr_mem_surface);

    gl->activeTexture(gl, 0);
    gl->enable(gl, GGL_BLEND);
    gl->blendFunc(gl, GGL_SRC_ALPHA, GGL_ONE_MINUS_SRC_ALPHA);

    gr_fb_blank(true);
    gr_fb_blank(false);

    return 0;
}

void gr_exit(void)
{
    // MStar Android Patch Begin
#ifdef USE_FBDEV
    fbdev_close(gr_fb_fd);
#else
    close(gr_fb_fd);
#endif
    // MStar Android Patch End
    gr_fb_fd = -1;

    free(gr_mem_surface.data);

    // MStar Android Patch Begin
    // We don't support kernel framebuffer devivce.
#ifndef USE_FBDEV
    ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
    close(gr_vt_fd);
    gr_vt_fd = -1;
#endif
    // MStar Android Patch End
}

int gr_fb_width(void)
{
// MStar Android Patch Begin
    return gr_framebuffer[0].width;
// MStar Android Patch End
}

int gr_fb_height(void)
{
// MStar Android Patch Begin
    return gr_framebuffer[0].height;
// MStar Android Patch End
}

gr_pixel *gr_fb_data(void)
{
    return (unsigned short *) gr_mem_surface.data;
}

void gr_fb_blank(bool blank)
{
    int ret;

    ret = ioctl(gr_fb_fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    if (ret < 0)
        perror("ioctl(): blank");
}
