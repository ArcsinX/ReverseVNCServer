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
#ifndef _VNC_FRAME_BUFFER_H_
#define _VNC_FRAME_BUFFER_H_

#include <linux/fb.h>

extern int                init_fb(char *framebuffer_device);
extern void               cleanup_fb();
extern unsigned int      *read_fb();

extern int                fbfd;
extern unsigned short    *fbmmap;
int                       fbsize;
struct fb_var_screeninfo  scrinfo;
struct fb_fix_screeninfo  fscrinfo;


#endif /* _VNC_FRAME_BUFFER_H_ */


