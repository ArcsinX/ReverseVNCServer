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
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


#include <fcntl.h>
#include <linux/fb.h>

#include <assert.h>
#include <errno.h>

/* libvncserver */
#include "rfb/rfb.h"
#include "rfb/keysym.h"

#include <android/keycodes.h>

/* Android does not use /dev/fb0. */
#define FB_DEVICE "/dev/graphics/fb0"

/* Android already has 5900 bound natively. */
#define VNC_PORT 5901

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static void
injectKeyEvent(int code, int done)
{
    if (done)
    {
        char keyevent_cmd[256];
        sprintf(keyevent_cmd, "input keyevent %d", code);
        system(keyevent_cmd);

#if 0
        printf("injectKeyEvent code=%d\n", code);
#endif
    }
}

static int
keysym2scancode(rfbKeySym key, rfbClientPtr cl)
{
    int scancode = 0;

    int code = (int)key;
    switch (code)
    {
        case 0xFF51:    scancode = AKEYCODE_DPAD_LEFT;       break; // left 
        case 0xFF53:    scancode = AKEYCODE_DPAD_RIGHT;      break; // right
        case 0xFF54:    scancode = AKEYCODE_DPAD_DOWN;       break; // down
        case 0xFF52:    scancode = AKEYCODE_DPAD_UP;         break; // up
        case 0xFF08:    scancode = AKEYCODE_BACK;            break; // Backspace
        case 0xFF1B:    scancode = AKEYCODE_BACK;            break; // ESC
        case 0xFF50:    scancode = AKEYCODE_HOME;            break; // Home
        case 0xFF55:    scancode = AKEYCODE_MENU;            break; // PgUp
        case 0xFFC0:    scancode = AKEYCODE_SEARCH;          break; // F3
        case 0xFFC7:    scancode = AKEYCODE_POWER;           break; // F10
        case 0xFFC8:    rfbShutdownServer(cl->screen,TRUE);  break; // F11
        case 0xFFC9:    exit(0);                             break; // F12
    }

    return scancode;
} 


static void
keyevent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	int scancode;

#if 0
	printf("Got keysym: %04x (down=%d)\n", (unsigned int)key, (int)down);
#endif

	if ((scancode = keysym2scancode(key, cl)))
	{
		injectKeyEvent(scancode, down);
	} 

}

static void
injectTapEvent(int x, int y)
{
    char tap_cmd[256];
    sprintf(tap_cmd, "input tap %d %d", x, y);
    system(tap_cmd);

#if 0
    printf("injectTapEvent (x=%d, y=%d)\n", x, y);
#endif
}

static void
injectSwipeEvent(int x1, int y1, int x2, int y2)
{
    char swipe_cmd[256];
    sprintf(swipe_cmd, "input swipe %d %d %d %d", x1, y1, x2, y2);
    system(swipe_cmd);

#if 0
    printf("injectSwipeEvent (x1=%d, y1=%d, x2=%d, y2=%d)\n",
           x1, y1, x2, y2);
#endif
}


static void
ptrevent(int buttonMask, int x, int y, rfbClientPtr cl)
{
    static int clicked = 0;
    static int prev_x, prev_y;

    if ((buttonMask & 1) && clicked)
        return;

	if(buttonMask & 1)
    {
        prev_x = x;
        prev_y = y;
        clicked = 1;
	} 
    else if (clicked)
    {
        if (x == prev_x && y == prev_y)
    		injectTapEvent(x, y);
        else
            injectSwipeEvent(prev_x, prev_y, x, y);
        clicked = 0;
    }
}


void
extract_host_port(char *str, char *rhost, int *rport)
{
    int len = strlen(str);
    char *p;

    strncpy(rhost, str, len);
    rhost[len] = '\0';

    /* extract port, if any */
    if ((p = strrchr(rhost, ':')) != NULL)
    {
        *rport = atoi(p + 1);
        if (*rport < 0)
        {
            *rport = -*rport;
        }
        else if (*rport == 0)
        {
            *rport = 5500;
        }
        *p = '\0';
    } 
}


static int
init_fb(struct fb_var_screeninfo *scrinfo,
        struct fb_fix_screeninfo *fscrinfo,
        unsigned short **fbmmap)
{
	size_t pixels;
	size_t bytespp;
    int fbfd;

	if ((fbfd = open(FB_DEVICE, O_RDONLY)) == -1)
	{
		printf("cannot open fb device %s\n", FB_DEVICE);
		exit(EXIT_FAILURE);
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, scrinfo) != 0)
	{
		printf("ioctl error\n");
		exit(EXIT_FAILURE);
	}

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, fscrinfo) != 0)
    {
        printf("ioctl error\n");
        exit(EXIT_FAILURE);
    }

	pixels = scrinfo->xres * scrinfo->yres;
	bytespp = scrinfo->bits_per_pixel / 8;

	printf("Screen info: xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n", 
	  (int)scrinfo->xres, (int)scrinfo->yres,
	  (int)scrinfo->xres_virtual, (int)scrinfo->yres_virtual,
	  (int)scrinfo->xoffset, (int)scrinfo->yoffset,
	  (int)scrinfo->bits_per_pixel);

    size_t yres = scrinfo->yres * 2 > scrinfo->yres_virtual ?
                  scrinfo->yres * 2 :
                  scrinfo->yres_virtual;
    size_t fb_size = fscrinfo->line_length * yres;

	*fbmmap = mmap(NULL, fb_size, PROT_READ, MAP_SHARED, fbfd, 0);
	if (*fbmmap == MAP_FAILED)
	{
		printf("mmap failed\n");
		exit(EXIT_FAILURE);
	}

    return fbfd;
}

static void
cleanup_fb(int fbfd)
{
	if(fbfd != -1)
	{
		close(fbfd);
	}
}

static rfbScreenInfoPtr
init_vnc_server(int port, struct fb_var_screeninfo *scrinfo,
                 unsigned short int *vncbuf)
{
    rfbScreenInfoPtr vncscr;

    printf("Initializing server...\n");

    vncscr = rfbGetScreen(NULL, NULL, scrinfo->xres, scrinfo->yres, 5, 2,
                          (scrinfo->bits_per_pixel / 8));
    assert(vncscr != NULL);

    vncscr->desktopName = "Android";
    vncscr->frameBuffer = (char *)vncbuf;
    vncscr->alwaysShared = TRUE;
    vncscr->httpDir = NULL;
    vncscr->port = port;

    vncscr->kbdAddEvent = keyevent;
    vncscr->ptrAddEvent = ptrevent;

    vncscr->serverFormat.redShift = scrinfo->red.offset;
    vncscr->serverFormat.greenShift = scrinfo->green.offset;
    vncscr->serverFormat.blueShift = scrinfo->blue.offset;

    vncscr->serverFormat.redMax = (( 1 << scrinfo->red.length) -1);
    vncscr->serverFormat.greenMax = (( 1 << scrinfo->green.length) -1);
    vncscr->serverFormat.blueMax = (( 1 << scrinfo->blue.length) -1);

    vncscr->serverFormat.trueColour = TRUE; 
    vncscr->serverFormat.bitsPerPixel = scrinfo->bits_per_pixel;

    rfbInitServer(vncscr);

    /* Mark as dirty since we haven't sent any updates at all yet. */
    rfbMarkRectAsModified(vncscr, 0, 0, scrinfo->xres, scrinfo->yres);

    return vncscr;
}

static void
update_fb_info(int fbfd, struct fb_var_screeninfo *scrinfo)
{
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, scrinfo) != 0)
    {
        printf("ioctl error\n");
        exit(EXIT_FAILURE);
    }
}

static unsigned int *
readFrameBuffer(int fbfd, unsigned short int *fbmmap,
                struct fb_var_screeninfo *scrinfo)
{
  update_fb_info(fbfd, scrinfo);
  return (unsigned int *)fbmmap;
}

#define OUT 32
#include "updatescreen.c"
#undef OUT
#define OUT 16
#include "updatescreen.c"
#undef OUT
#define OUT 8
#include "updatescreen.c"
#undef OUT

void print_usage(char **argv)
{
    puts("%s [-h] [-c host:port] [-r] [-p localport]\n"
        "-c host:port : Reverse connection host and port\n"
        "-r : reconnect on reverse connections lost\n"
        "-p localport : Local port for incoming connections. Default if 5901\n"
        "-h : print this help");
}

int main(int argc, char **argv)
{
    void (*update_screen)(rfbScreenInfoPtr,
         int,
         unsigned short int *,
         unsigned short int *,
         unsigned short int *,
         struct fb_var_screeninfo *) = NULL;

    char rhost[256] = {0};
    int  rport = 5500;
    int  port = 5901;
    rfbClientPtr reverse_client = NULL;
    int  reconnect_on_lost = FALSE;

    rfbScreenInfoPtr vncscr;

    struct fb_var_screeninfo scrinfo;
    struct fb_fix_screeninfo fscrinfo;
    int                      fbfd;
    unsigned short int      *fbmmap;
    unsigned short int      *fbbuf;
    unsigned short int      *vncbuf;

	if(argc > 1)
	{
		int i=1;
		while(i < argc)
		{
			if(*argv[i] == '-')
			{
				switch(*(argv[i] + 1))
				{
					case 'h':
						print_usage(argv);
						exit(0);
						break;
					case 'c':
						i++;
                        extract_host_port(argv[i], rhost, &rport);
						break;
                    case 'p':
                        i++;
                        port = atoi(argv[i]);
                        break;
                    case 'r':
                        i++;
                        reconnect_on_lost = TRUE;
                        break;
				}
			}
			i++;
		}
	}

	printf("Initializing framebuffer device " FB_DEVICE "...\n");
	fbfd = init_fb(&scrinfo, &fscrinfo, &fbmmap);

	vncbuf = calloc(scrinfo.xres * scrinfo.yres, scrinfo.bits_per_pixel / 8);
	assert(vncbuf != NULL);
	fbbuf = calloc(scrinfo.xres * scrinfo.yres, scrinfo.bits_per_pixel / 8);
	assert(fbbuf != NULL);

	vncscr = init_vnc_server(port, &scrinfo, vncbuf);

	printf("Initializing VNC server:\n");
	printf("	width:  %d\n", (int)scrinfo.xres);
	printf("	height: %d\n", (int)scrinfo.yres);
	printf("	bpp:    %d\n", (int)scrinfo.bits_per_pixel);
	printf("	port:   %d\n", port);

    if (vncscr->serverFormat.bitsPerPixel == 32)
        update_screen = update_screen_32;
    else if (vncscr->serverFormat.bitsPerPixel == 16)
        update_screen = update_screen_16;
    else if (vncscr->serverFormat.bitsPerPixel == 8)
        update_screen = update_screen_8;

    assert(update_screen != NULL);

    if (rhost[0] != '\0')
    {
        reverse_client = rfbReverseConnection(vncscr, rhost, rport);
        if (reverse_client == NULL)
            printf("Couldn't connect to remote host: %s at port %d\n",
                   rhost, rport);
        else
        {
            reverse_client->onHold = FALSE;
            rfbStartOnHoldClient(reverse_client);
        }
    }

	while (1)
	{
        /* Reconnectio on reverse connection lost */
        if (reverse_client && reconnect_on_lost)
        {
            rfbClientPtr cl;

            for (cl = vncscr->clientHead;
                 cl != NULL && cl != reverse_client;
                 cl = cl->next);

            if (cl == NULL || cl->sock == -1)
            {
                cl = rfbReverseConnection(vncscr, rhost, rport);
                if (cl == NULL)
                    printf("Couldn't connect to remote host: %s at port %d\n",
                           rhost, rport);
                else
                {
                    reverse_client = cl;
                    reverse_client->onHold = FALSE;
                    rfbStartOnHoldClient(reverse_client);
                }
            }
        }

        if (!reverse_client)
    		while (vncscr->clientHead == NULL)
	    		rfbProcessEvents(vncscr, 10000);

		rfbProcessEvents(vncscr, 10000);
		update_screen(vncscr, fbfd, fbbuf, vncbuf, fbmmap, &scrinfo);
	}

	printf("Cleaning up...\n");
	cleanup_fb(fbfd);
    free(vncbuf);
    free(fbbuf);
}
