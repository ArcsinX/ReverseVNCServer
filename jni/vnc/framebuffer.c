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

#include <stdio.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

/* Android does not use /dev/fb0. */
#define FB_DEVICE "/dev/graphics/fb0"

static inline int
align_size(int size)
{
    return (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}

int
init_fb(struct fb_var_screeninfo *scrinfo,
        struct fb_fix_screeninfo *fscrinfo,
        unsigned short **fbmmap)
{
	size_t pixels;
	size_t bytespp;
    int fbfd;

	printf("Initializing framebuffer device " FB_DEVICE "...\n");

	if ((fbfd = open(FB_DEVICE, O_RDWR)) == -1)
	{
		printf("cannot open fb device %s\n", FB_DEVICE);
		return -1;
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, scrinfo) != 0)
	{
		printf("ioctl error\n");
        close(fbfd);
		return -1;
	}

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, fscrinfo) != 0)
    {
        printf("ioctl error\n");
        close(fbfd);
        return -1;
    }

	pixels = scrinfo->xres * scrinfo->yres;
	bytespp = scrinfo->bits_per_pixel / 8;

    size_t yres = scrinfo->yres * 2 > scrinfo->yres_virtual ?
                  scrinfo->yres * 2 :
                  scrinfo->yres_virtual;
    size_t fb_size = fscrinfo->line_length * yres;

	printf("Screen info: xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n"
           "\tline_length=%d, fb_size=%d, align_size(fb_size)=%d\n",
	  (int)scrinfo->xres, (int)scrinfo->yres,
	  (int)scrinfo->xres_virtual, (int)scrinfo->yres_virtual,
	  (int)scrinfo->xoffset, (int)scrinfo->yoffset,
	  (int)scrinfo->bits_per_pixel,
      (int)fscrinfo->line_length, (int)fb_size, align_size(fb_size));


	*fbmmap = mmap(NULL, align_size(fb_size), PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (*fbmmap == MAP_FAILED)
	{
		printf("mmap failed\n");
        close(fbfd);
		return -1;
	}

    return fbfd;
}

void
cleanup_fb(int fbfd)
{
	if(fbfd != -1)
		close(fbfd);
}

static int
update_fb_info(int fbfd, struct fb_var_screeninfo *scrinfo)
{
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, scrinfo) != 0)
    {
        printf("ioctl error\n");
        return -1;
    }
    return 0;
}

unsigned int *
readFrameBuffer(int fbfd, unsigned short int *fbmmap,
                struct fb_var_screeninfo *scrinfo)
{
  if (update_fb_info(fbfd, scrinfo) == -1)
    return NULL;
  return (unsigned int *)fbmmap;
}

