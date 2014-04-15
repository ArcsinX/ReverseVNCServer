/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#if 0
#define USE_MMAP
#endif

#include <stdio.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef USE_MMAP
#include <sys/mman.h>
#endif
#include <errno.h>

/* Android does not use /dev/fb0. */
#define FB_DEVICE "/dev/graphics/fb0"
int                       fbfd     = -1;
#ifdef USE_MMAP
unsigned short           *fbmmap   = MAP_FAILED;
#else
unsigned short           *fbmmap   = NULL;
#endif
int                       fbsize   = 0;
struct fb_var_screeninfo  scrinfo  = {0};
struct fb_fix_screeninfo  fscrinfo = {0};

#ifdef USE_MMAP
static inline int
align_size(int size)
{
    return (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}
#endif

int
init_fb(char *framebuffer_device)
{
	size_t pixels;
	size_t bytespp;

    if (framebuffer_device == NULL)
        framebuffer_device = FB_DEVICE;

    printf("Initializing framebuffer device %s ... \n", framebuffer_device);

    if ((fbfd = open(framebuffer_device, O_RDONLY)) == -1)
    {
        printf("cannot open fb device %s\n", framebuffer_device);
        perror("open failed");
        return -1;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
    {
        perror("ioctl FBIOGET_VSCREENINFO failed");
        close(fbfd);
        fbfd = -1;
        return -1;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fscrinfo) != 0)
    {
        perror("ioctl FBIOGET_FSCREENINFO failed");
        close(fbfd);
        fbfd = -1;
        return -1;
    }

    pixels = scrinfo.xres * scrinfo.yres;
    bytespp = scrinfo.bits_per_pixel / 8;

    size_t yres = scrinfo.yres * 2 > scrinfo.yres_virtual ?
                  scrinfo.yres * 2 :
                  scrinfo.yres_virtual;
    fbsize = fscrinfo.line_length * yres;

    printf("Screen info:\n"
           "\txres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n"
           "\tline_length=%d, fbsize=%d\n",
           (int)scrinfo.xres, (int)scrinfo.yres,
           (int)scrinfo.xres_virtual, (int)scrinfo.yres_virtual,
           (int)scrinfo.xoffset, (int)scrinfo.yoffset,
           (int)scrinfo.bits_per_pixel,
           (int)fscrinfo.line_length, (int)fbsize);


#ifndef USE_MMAP
    fbmmap = malloc(fbsize);
    if (fbmmap == NULL)
    {
        perror("Framebuffer malloc fails");
        close(fbfd);
        fbfd = -1;
        return -1;
    }
#else
	fbmmap = mmap(NULL, align_size(fbsize), PROT_READ, 0, fbfd, 0);
	if (fbmmap == MAP_FAILED)
	{
        perror("mmap failed");
        close(fbfd);
        fbfd = -1;
		return -1;
	}
#endif

    return fbfd;
}

void
cleanup_fb()
{
    int fd;

	if (fbfd != -1)
    {
		close(fbfd);
        fbfd = -1;
    }
#ifdef USE_MMAP
    munmap(fbmmap, fbsize);
    fbmmap = MAP_FAILED;
#else
    free(fbmmap);
    fbmmap = NULL;
#endif
    /* HACK:
     * Old rk3188 video driver has the following problem:
     * closing file descriptor leads to screen turning off.
     * So we should enable it back by writing 1 into
     * /sys/class/graphics/fb0/enable
     */
    fd = open("/sys/class/graphics/fb0/enable", O_RDWR);
    if (fd == -1)
        return;
    write(fd, "1", 1);
    close(fd);
}

static int
update_fb_info()
{
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
    {
        perror("update_fb_info: ioctl error\n");
        return -1;
    }
    return 0;
}

unsigned int *
read_fb()
{
    if (update_fb_info() == -1)
        return NULL;
#ifndef USEMMAP
    if (lseek(fbfd, SEEK_SET, 0) == -1)
    {
        perror("lseek failed for framebuffer device\n");
        return NULL;
    }
    if (read(fbfd, fbmmap, fbsize) == -1)
    {
        perror("Framebuffer read failed");
        return NULL;
    }
#endif

    return (unsigned int *)fbmmap;
}

