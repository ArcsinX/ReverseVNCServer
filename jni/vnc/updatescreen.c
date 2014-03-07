/*
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
*/

#ifndef CONCAT2
#define CONCAT2(a,b) a##b
#endif
#ifndef CONCAT2E
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif
#ifndef CONCAT3
#define CONCAT3(a,b,c) a##b##c
#endif
#ifndef CONCAT3E
#define CONCAT3E(a,b,c) CONCAT3(a,b,c)
#endif

#define OUT_T CONCAT3E(uint,OUT,_t)
#define FUNCTION CONCAT2E(update_screen_,OUT)

#ifndef PIXEL_TO_VIRTUALPIXEL_FB
#define PIXEL_TO_VIRTUALPIXEL_FB(i,j) ((j+scrinfo->yoffset)*scrinfo->xres_virtual+i+scrinfo->xoffset)
#endif

static int
FUNCTION(rfbScreenInfoPtr vncscr,
         int fbfd, int fb_size,
         unsigned short int *fbbuf,
         unsigned short int *vncbuf,
         unsigned short int *fbmmap,
         struct fb_var_screeninfo *scrinfo)
{  
    int i, j;
    int offset, pixelToVirtual;
    int max_x = 0, max_y = 0, min_x = INT_MAX, min_y = INT_MAX;
    int unchanged = TRUE;

    OUT_T* a = (OUT_T*)fbbuf;
    OUT_T* b = (OUT_T*)readFrameBuffer(fbfd, fb_size, fbmmap, scrinfo);

    if (b == NULL)
        return -1;

    for (j = 0; j < vncscr->height; j++)
    {
        for (i = 0; i < vncscr->width; i++)
        {
            offset = j * vncscr->width;

            pixelToVirtual = PIXEL_TO_VIRTUALPIXEL_FB(i,j);

            if (a[i + offset] != b[pixelToVirtual])
            {
                a[i + offset] = b[pixelToVirtual];
                if (i > max_x)
                  max_x = i;
                if (i < min_x)
                  min_x = i;

                if (j > max_y)
                  max_y = j;
                if (j < min_y)
                  min_y = j;

                unchanged = FALSE;
            }
        }
    }

    if (!unchanged)
    {
        memcpy(vncbuf, a,
               scrinfo->xres * scrinfo->yres *
               scrinfo->bits_per_pixel / 8);

        rfbMarkRectAsModified(vncscr, min_x, min_y, max_x, max_y);
    }

    return 0;
}

