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

#define VERSION "1.3"
#if 0
#define DEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <assert.h>
#include <errno.h>

/* libvncserver */
#include "rfb/rfb.h"
#include "rfb/keysym.h"

#include <android/keycodes.h>

#include "framebuffer.h"

/* Android already has 5900 bound natively. */
#define VNC_PORT 5901

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static int view_only = FALSE;

static int screen_scale_percent  = 100;
static int cursor_zoom_percent   = 100;

rfbScreenInfoPtr    vncscr = NULL;
unsigned short int *vncbuf = NULL;
unsigned short int *fbbuf  = NULL;


static volatile int vnc_is_active = FALSE;

static void Cleanup();

static void InjectKeyEvent(int code)
{
    char keyevent_cmd[256];
    sprintf(keyevent_cmd, "input keyevent %d", code);
    system(keyevent_cmd);

#ifdef DEBUG
    printf("injectKeyEvent code=%d\n", code);
#endif
}

static void InjectTextEvent(int code)
{
    char keyevent_cmd[256];
    if ((char)code == '\'')
    {
        sprintf(keyevent_cmd, "input text \"%c\"", (char)code);
    }
    else
    {
        sprintf(keyevent_cmd, "input text '%c'", (char)code);
    }
    system(keyevent_cmd);

#ifdef DEBUG
    printf("injectTextEvent code=%c\n", (char)code);
#endif
}

static int KeySym2Text(rfbKeySym key)
{
    int code = (int)key;

    if (code >= 0x61 && code <= 0x7a) /* a..z */
    {
        return 'a' + (code - 0x61);
    }
    if (code >= 0x41 && code <= 0x5a) /* A..Z */
    {
        return 'A' + (code - 0x41);
    }
    if (code >= 0x30 && code <= 0x39) /* 0 .. 9 */
    {
        return '0' + (code - 0x30);
    }
    switch (code)
    {
        case 0x3a:
            return ':';
        case 0x3b:
            return ';';
        case 0x3c:
            return '<';
        case 0x3d:
            return '=';
        case 0x3e:
            return '>';
        case 0x3f:
            return '?';
        case 0x40:
            return '@';
        case 0x21:
            return '!';
        case 0x22:
            return '"';
        case 0x23:
            return '#';
        case 0x24:
            return '$';
        case 0x25:
            return '%';
        case 0x26:
            return '&';
        case 0x27:
            return '\'';
        case 0x28:
            return '(';
        case 0x29:
            return ')';
        case 0x2b:
            return '+';
        case 0x2c:
            return ',';
        case 0x2d:
            return '-';
        case 0x2e:
            return '.';
        case 0x2f:
            return '/';
        case 0x5b:
            return '[';
        case 0x5c:
            return '\\';
        case 0x5d:
            return ']';
        case 0x5e:
            return '^';
        case 0x5f:
            return '_';
        case 0x60:
            return '`';
        case 0x7b:
            return '{';
        case 0x7c:
            return '|';
        case 0x7d:
            return '}';
        case 0x7e:
            return '~';
        
    }
    return 0;
}

static int KeySym2ScanCode(rfbKeySym key, rfbClientPtr cl)
{
    int scancode = 0;

    int code = (int)key;

    switch (code)
    {
        case 0x0020:    scancode = AKEYCODE_SPACE;           break; // space
        case 0x002A:    scancode = AKEYCODE_STAR;            break; // *
        case 0xFF0D:    scancode = AKEYCODE_ENTER;           break; // Enter
        case 0xFF51:    scancode = AKEYCODE_DPAD_LEFT;       break; // left 
        case 0xFF53:    scancode = AKEYCODE_DPAD_RIGHT;      break; // right
        case 0xFF54:    scancode = AKEYCODE_DPAD_DOWN;       break; // down
        case 0xFF52:    scancode = AKEYCODE_DPAD_UP;         break; // up
        case 0xFF08:    scancode = AKEYCODE_DEL;             break; // Backspace
        case 0xFF1B:    scancode = AKEYCODE_BACK;            break; // ESC
        case 0xFF50:    scancode = AKEYCODE_HOME;            break; // Home
        case 0xFF55:    scancode = AKEYCODE_MENU;            break; // PgUp
        case 0xFFC0:    scancode = AKEYCODE_SEARCH;          break; // F3
        case 0xFFC7:    scancode = AKEYCODE_POWER;           break; // F10
        case 0xFFC8:    rfbShutdownServer(cl->screen,TRUE);  break; // F11
        case 0xFFC9:    vnc_is_active = FALSE;               break; // F12
        case 0xFFFF:    scancode = AKEYCODE_FORWARD_DEL;     break; // DEL
        case 0xFF6B:    scancode = AKEYCODE_BREAK;           break; // ctrl+c
    }

    return scancode;
} 


static void KeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    int scancode;

#ifdef DEBUG
    printf("Got keysym: %04x (down=%d)\n", (unsigned int)key, (int)down);
#endif

    if (down)
    {
        if ((scancode = KeySym2Text(key)))
        {
            InjectTextEvent(scancode);
        }
        else if ((scancode = KeySym2ScanCode(key, cl)))
        {
            InjectKeyEvent(scancode);
        }
    }
}

static void InjectTapEvent(int x, int y)
{
    char tap_cmd[256];
    sprintf(tap_cmd, "input tap %d %d", x, y);
    system(tap_cmd);

#ifdef DEBUG
    printf("injectTapEvent (x=%d, y=%d)\n", x, y);
#endif
}

static void InjectSwipeEvent(int x1, int y1, int x2, int y2)
{
    char swipe_cmd[256];
    sprintf(swipe_cmd, "input swipe %d %d %d %d", x1, y1, x2, y2);
    system(swipe_cmd);

#ifdef DEBUG
    printf("injectSwipeEvent (x1=%d, y1=%d, x2=%d, y2=%d)\n",
           x1, y1, x2, y2);
#endif
}


static void PtrEvent(int button_mask, int x, int y, rfbClientPtr cl)
{
    static int clicked = 0;
    static int prev_x, prev_y;

    if (cursor_zoom_percent != 100)
    {
        x = (int)(((double)x * 100) / cursor_zoom_percent);
        y = (int)(((double)y * 100) / cursor_zoom_percent);
    }

#ifdef DEBUG
    printf("buttonMask = 0x%x, x=%d, y=%d, prev_x=%d, prev_y=%d, clicked=%d\n", button_mask, x, y, prev_x, prev_y, clicked);
#endif
    if ((button_mask & 1) && clicked)
        return;

    if(button_mask & 1)
    {
        prev_x = x;
        prev_y = y;
        clicked = 1;
    } 
    else if (clicked)
    {
        if (x == prev_x && y == prev_y)
            InjectTapEvent(x, y);
        else
            InjectSwipeEvent(prev_x, prev_y, x, y);
        clicked = 0;
    }
}


void ExtractHostPort(char *str, char *rhost, int *rport)
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

static enum rfbNewClientAction NewVncClient(rfbClientPtr cl)
{

    if (screen_scale_percent != 100)
    {
        rfbScalingSetup(cl,
                        cl->screen->width  * screen_scale_percent / 100,
                        cl->screen->height * screen_scale_percent / 100);
    }
    cl->viewOnly = view_only;

    return RFB_CLIENT_ACCEPT;
}

static void InitVncServer(int port, struct fb_var_screeninfo *scrinfo,
                          unsigned short int *vncbuf)
{
    printf("Initializing server...\n");

    vncscr = rfbGetScreen(NULL, NULL, scrinfo->xres, scrinfo->yres, 5, 2,
                          (scrinfo->bits_per_pixel / 8));
    assert(vncscr != NULL);

    vncscr->desktopName = "Android";
    vncscr->frameBuffer = (char *)vncbuf;
    vncscr->alwaysShared = TRUE;
    vncscr->httpDir = "webclients/";
    vncscr->port = port;

    vncscr->kbdAddEvent = KeyEvent;
    vncscr->ptrAddEvent = PtrEvent;

    vncscr->serverFormat.redShift = scrinfo->red.offset;
    vncscr->serverFormat.greenShift = scrinfo->green.offset;
    vncscr->serverFormat.blueShift = scrinfo->blue.offset;

    vncscr->serverFormat.redMax = (( 1 << scrinfo->red.length) -1);
    vncscr->serverFormat.greenMax = (( 1 << scrinfo->green.length) -1);
    vncscr->serverFormat.blueMax = (( 1 << scrinfo->blue.length) -1);

    vncscr->serverFormat.trueColour = TRUE; 
    vncscr->serverFormat.bitsPerPixel = scrinfo->bits_per_pixel;

    vncscr->deferUpdateTime = 5;
    vncscr->deferPtrUpdateTime = 9999999;
    vncscr->handleEventsEagerly = TRUE;

    vncscr->newClientHook = NewVncClient;

    rfbInitServer(vncscr);

    /* Mark as dirty since we haven't sent any updates at all yet. */
    rfbMarkRectAsModified(vncscr, 0, 0, scrinfo->xres, scrinfo->yres);
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

static void PrintUsage(char **argv)
{
    printf("%s [-h] [-c host:port] [-r] [-v] [-p localport]\n"
        "-c host:port : Reverse connection host and port\n"
        "-r : reconnect on reverse connections lost\n"
        "-v : view only\n"
        "-p localport : Local port for incoming connections (default is 5901)\n"
        "-s scale : scale percent (default is 100)\n"
        "-d framebuffer device (default is /dev/graphics/fb0)\n"
        "-z zoom : specify zoom of cursor coordinates in precents\n"
        "-h : print this help\n", argv[0]);
}

static void Cleanup()
{
    printf("Cleaning up...\n");

    if (vncscr)
    {
        rfbScreenCleanup(vncscr);
        vncscr = NULL;
    }
    CleanupFb();
    free(vncbuf);
    vncbuf = NULL;
    free(fbbuf);
    fbbuf = NULL;
}

static void TerminationHandler(int signo)
{
    /* Prevent warning */
    (void)signo;
    Cleanup();
    exit(0);
}


int main(int argc, char **argv)
{
    int (*update_screen)() = NULL;

    char rhost[256] = {0};
    int  rport      = 5500;
    int  port       = 5901;

    rfbClientPtr reverse_client    = NULL;
    int          reconnect_on_lost = FALSE;

    char        *fbdevice = NULL;

    if (argc == 2)
    {
        if (strcmp(argv[1], "--version") == 0)
        {
            printf("%s\n", VERSION);
            return 0;
        }
    }

    puts("User options set:");
    if (argc > 1)
    {
        int i = 1;
        while (i < argc)
        {
            if(*argv[i] == '-')
            {
                switch(*(argv[i] + 1))
                {
                    case 'h':
                        PrintUsage(argv);
                        exit(0);
                        break;
                    case 'c':
                        i++;
                        ExtractHostPort(argv[i], rhost, &rport);
                        printf("\tReverse connection string: %s\n", argv[i]);
                        break;
                    case 'p':
                        i++;
                        port = atoi(argv[i]);
                        printf("\tLocal port: %d\n", port);
                        break;
                    case 'r':
                        reconnect_on_lost = TRUE;
                        puts("\tReconnect on reverse connection lost");
                        break;
                    case 'v':
                        view_only = TRUE;
                        puts("\tView only mode enabled");
                        break;
                    case 's':
                        i++;
                        screen_scale_percent = atoi(argv[i]);
                        printf("\tscreen scale: %d %%\n", screen_scale_percent);
                        break;
                    case 'd':
                        i++;
                        fbdevice = argv[i];
                        printf("\tframebuffer device: %s\n", fbdevice);
                        break;
                    case 'z':
                        i++;
                        cursor_zoom_percent = atoi(argv[i]);
                        printf("\tcursor coordinates zoom percent: %d\n",
                               cursor_zoom_percent);
                        break;
                }
            }
            i++;
        }
    }

    (void)signal(SIGINT,  TerminationHandler);
    (void)signal(SIGHUP,  TerminationHandler);
    (void)signal(SIGTERM, TerminationHandler);

    fbfd = InitFb(fbdevice);
    if (fbfd == -1)
    {
        puts("Failed to initialize frame buffer");
        exit(EXIT_FAILURE);
    }

    vncbuf = calloc(scrinfo.xres * scrinfo.yres, scrinfo.bits_per_pixel / 8);
    assert(vncbuf != NULL);
    fbbuf = calloc(scrinfo.xres * scrinfo.yres, scrinfo.bits_per_pixel / 8);
    assert(fbbuf != NULL);

    InitVncServer(port, &scrinfo, vncbuf);

    printf("Initializing VNC server:\n");
    printf("	width:         %d\n", (int)scrinfo.xres);
    printf("	height:        %d\n", (int)scrinfo.yres);
    printf("	bpp:           %d\n", (int)scrinfo.bits_per_pixel);
    printf("	port:          %d\n", port);
    printf("	screen scale:  %d\n", screen_scale_percent);

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
            (void)NewVncClient(reverse_client);
        }
    }

    vnc_is_active = TRUE;
    while (vnc_is_active)
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
                    (void)NewVncClient(reverse_client);
                }
            }
        }

        if (!reverse_client)
            while (vncscr->clientHead == NULL)
                rfbProcessEvents(vncscr, vncscr->deferUpdateTime * 1000);

        rfbProcessEvents(vncscr, vncscr->deferUpdateTime * 1000);
        if (update_screen() == -1)
        {
            puts("Failed to update screen");
            Cleanup();
            exit(EXIT_FAILURE);
        }
    }

    rfbShutdownServer(vncscr, TRUE);
    Cleanup();
}
