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
#if 0
#include <sys/mman.h>
#endif
#include <errno.h>

/* Android does not use /dev/fb0. */
#define FB_DEVICE "/dev/graphics/fb0"

#if 1
static inline int
align_size(int size)
{
    return (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}
#endif

int
init_fb(char *framebuffer_device, struct fb_var_screeninfo *scrinfo,
        struct fb_fix_screeninfo *fscrinfo,
        unsigned short **fbmmap, int *fb_size)
{
	size_t pixels;
	size_t bytespp;
    int fbfd;

    if (framebuffer_device == NULL)
        framebuffer_device = FB_DEVICE;

    printf("Initializing framebuffer device %s ... \n", framebuffer_device);

    if ((fbfd = open(framebuffer_device, O_RDONLY)) == -1)
    {
        printf("cannot open fb device %s\n", framebuffer_device);
        perror("open failed");
        return -1;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, scrinfo) != 0)
    {
        perror("ioctl FBIOGET_VSCREENINFO failed");
        close(fbfd);
        return -1;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, fscrinfo) != 0)
    {
        perror("ioctl FBIOGET_FSCREENINFO failed");
        close(fbfd);
        return -1;
    }

    pixels = scrinfo->xres * scrinfo->yres;
    bytespp = scrinfo->bits_per_pixel / 8;

    size_t yres = scrinfo->yres * 2 > scrinfo->yres_virtual ?
                  scrinfo->yres * 2 :
                  scrinfo->yres_virtual;
    *fb_size = fscrinfo->line_length * yres;

    printf("Screen info:\n"
           "\txres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n"
           "\tline_length=%d, fb_size=%d\n",
           (int)scrinfo->xres, (int)scrinfo->yres,
           (int)scrinfo->xres_virtual, (int)scrinfo->yres_virtual,
           (int)scrinfo->xoffset, (int)scrinfo->yoffset,
           (int)scrinfo->bits_per_pixel,
           (int)fscrinfo->line_length, (int)(*fb_size));


#if 1
    *fbmmap = malloc(*fb_size);
    if (*fbmmap == NULL)
    {
        perror("Framebuffer malloc fails");
        close(fbfd);
        return -1;
    }
#else
	*fbmmap = mmap(NULL, align_size(*fb_size), PROT_READ, 0, fbfd, 0);
	if (*fbmmap == MAP_FAILED)
	{
        perror("mmap failed");
        close(fbfd);
		return -1;
	}
#endif

    return fbfd;
}

void
cleanup_fb(int fbfd, void *fbmmap)
{
	if(fbfd != -1)
		close(fbfd);
    free(fbmmap);
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
readFrameBuffer(int fbfd, int fb_size, unsigned short int *fbmmap,
                struct fb_var_screeninfo *scrinfo)
{
    if (update_fb_info(fbfd, scrinfo) == -1)
        return NULL;
#if 1
    if (lseek(fbfd, SEEK_SET, 0) == -1)
    {
        perror("lseek failed for framebuffer device\n");
        return NULL;
    }
    if (read(fbfd, fbmmap, fb_size) == -1)
    {
        perror("Framebuffer read failed");
        return NULL;
    }
#else
    (void)fb_size;
#endif

    return (unsigned int *)fbmmap;
}

