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

extern int           init_fb(struct fb_var_screeninfo *scrinfo,
                             struct fb_fix_screeninfo *fscrinfo,
                             unsigned short **fbmmap);
extern void          cleanup_fb(int fbfd);
extern unsigned int *readFrameBuffer(int fbfd, unsigned short int *fbmmap,
                                     struct fb_var_screeninfo *scrinfo);



#endif /* _VNC_FRAME_BUFFER_H_ */


