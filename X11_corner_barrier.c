/* Copyright © 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h> 

int no_of_displays = -1;

typedef struct{
    int top;
    int bottom;
    int left;
    int right;
    int inUse;
} DisplayParameters;

typedef struct{
    PointerBarrier topleft_left;
    PointerBarrier topleft_top;
    PointerBarrier topright_right;
    PointerBarrier topright_top;
    PointerBarrier bottomleft_left;
    PointerBarrier bottomleft_bottom;
    PointerBarrier bottomright_right;
    PointerBarrier bottomright_bottom;
} PointerBarrierHolder;

DisplayParameters* displayParameterArray;
PointerBarrierHolder* pointerBarrierArray;

int barrier_y;
Display *dpy;
Window window;

static void process_barrier_event (XIBarrierEvent *event) {
    printf("   root coordinates: %.2f/%.2f\n", event->root_x, event->root_y);
    printf("   delta: %.2f/%.2f\n", event->dx, event->dy);
}

void getDisplaysInfo(){
    XRRScreenResources* screen;
    XRRCrtcInfo* crtc_info;

    //dpy = XOpenDisplay(":0");
    screen = XRRGetScreenResources (dpy, DefaultRootWindow(dpy));
    no_of_displays = screen->noutput;

    displayParameterArray = malloc(sizeof(*displayParameterArray) * no_of_displays);

    for (int i = 0; i< screen->noutput; i++){
        crtc_info = XRRGetCrtcInfo(dpy, screen, screen->crtcs[i]); 
        
        displayParameterArray[i].inUse = 1;
        displayParameterArray[i].left = crtc_info->x;
        displayParameterArray[i].right = crtc_info->x + crtc_info->width;
        displayParameterArray[i].top = crtc_info->y;
        displayParameterArray[i].bottom = crtc_info->y + crtc_info->height;

        if (crtc_info-> width == 0 && crtc_info-> width == 0) {
            displayParameterArray[i].inUse = 0;
            continue;
        }
    }
    return;
}

int main (int argc, char **argv) {
    XEvent xev;
    XIEventMask mask;
    int major, minor;
    int opcode, evt, err;
    unsigned char mask_bits[XIMaskLen (XI_LASTEVENT)] = { 0 };

    dpy = XOpenDisplay(NULL);

    XRRCrtcInfo* crtc_info_array;
    getDisplaysInfo();

    int workspacewidth = 0;
    int workspaceheight = 0;

    for (int i = 0; i < no_of_displays; i++){
        printf("Display: %d X: %4d Y: %4d W: %4d H: %4d InUSE: %d\n", i, displayParameterArray[i].left, displayParameterArray[i].top, displayParameterArray[i].right, displayParameterArray[i].bottom, displayParameterArray[i].inUse);
        if (displayParameterArray[i].right > workspacewidth) {
            workspacewidth = displayParameterArray[i].right;
        }
        if (displayParameterArray[i].bottom > workspaceheight) {
            workspaceheight = displayParameterArray[i].bottom;
        }
    }

    if (!dpy) {
        printf("Failed to connect to X server\n");
        return 1;
    }

    if (!XQueryExtension (dpy, "XFIXES", &opcode, &evt, &err)) {
        printf("Need XFixes\n");
        return 1;
    }

    if (!XFixesQueryVersion (dpy, &major, &minor) ||
        (major * 10 + minor) < 50) {
        printf("Need XFixes 5.0\n");
        return 1;
    }

    if (!XQueryExtension (dpy, "XInputExtension", &opcode, &evt, &err)) {
        printf("Need XInput\n");
        return 1;
    }

    major = 2;
    minor = 3;

    if (XIQueryVersion (dpy, &major, &minor) != Success) {
        printf ("Need XInput 2.3\n");
        return 1;
    }

    if (((major * 10) + minor) < 22) {
        printf ("Need XInput 2.2\n");
        return 1;
    }

    window = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy),
                                  0, 0, workspacewidth, workspaceheight,
                                  0, BlackPixel(dpy, 0),
                                  BlackPixel(dpy, 0));

    int range = 50;
    int not_permissive = 0;
    int permissive_from_bottom = 1; // positive Y
    int permissive_from_top = 2; // positive Y
    int permissive_from_left = 3; // positive X
    int permissive_from_right = 4; // negative X

    pointerBarrierArray = malloc(sizeof(*pointerBarrierArray) * no_of_displays);

    for (int i = 0; i< no_of_displays; i++){
        if (!displayParameterArray[i].inUse) {continue;}

        //TOP
        //<-
        // |    permissive_from_left
        pointerBarrierArray[i].topleft_left = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].left, displayParameterArray[i].top, 
                                                        displayParameterArray[i].left, displayParameterArray[i].top + range, permissive_from_left, 0, NULL);
        // _    permissive_from_top
        pointerBarrierArray[i].topleft_top = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].left, displayParameterArray[i].top, 
                                                        displayParameterArray[i].left + range, displayParameterArray[i].top, permissive_from_top, 0, NULL);
        //->
        // |    permissive_from_right
        pointerBarrierArray[i].topright_right = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].right, displayParameterArray[i].top, 
                                                        displayParameterArray[i].right, displayParameterArray[i].top + range, permissive_from_right, 0, NULL);
        // _    permissive_from_top
        pointerBarrierArray[i].topright_top = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].right, displayParameterArray[i].top, 
                                                        displayParameterArray[i].right - range, displayParameterArray[i].top, permissive_from_top, 0, NULL);

        //BOTTOM
        //<-
        // |    permissive_from_left
        pointerBarrierArray[i].bottomleft_left = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].left, displayParameterArray[i].bottom, 
                                                        displayParameterArray[i].left, displayParameterArray[i].bottom - range, permissive_from_left, 0, NULL);
        
        // _    permissive_from_bottom
        pointerBarrierArray[i].bottomleft_bottom = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].left, displayParameterArray[i].bottom, 
                                                        displayParameterArray[i].left + range, displayParameterArray[i].bottom, permissive_from_bottom, 0, NULL);
        //->
        // |    permissive_from_right
        pointerBarrierArray[i].bottomright_right = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].right, displayParameterArray[i].bottom, 
                                                        displayParameterArray[i].right, displayParameterArray[i].bottom - range, permissive_from_right, 0, NULL);
        // _    permissive_from_bottom
        pointerBarrierArray[i].bottomright_bottom = XFixesCreatePointerBarrier(dpy, window, displayParameterArray[i].right, displayParameterArray[i].bottom, 
                                                        displayParameterArray[i].right - range, displayParameterArray[i].bottom, permissive_from_bottom, 0, NULL);
    }

    XISetMask (mask_bits, XI_BarrierHit);
    XISetMask (mask_bits, XI_BarrierLeave);
    mask.deviceid = XIAllMasterDevices;
    mask.mask = mask_bits;
    mask.mask_len = sizeof (mask_bits);
    XISelectEvents (dpy, window, &mask, 1);

    //XSync(dpy, False);
    while (1) {
        XNextEvent (dpy, &xev);
        switch (xev.type) {
        case GenericEvent:
            {
                XGenericEventCookie *cookie = &xev.xcookie;
                const char *type;

                if (cookie->extension != opcode)
                    break;

                switch(cookie->evtype) {
                    case XI_BarrierHit: type = "BarrierHit"; break;
                    case XI_BarrierLeave: type = "BarrierLeave"; break;
                }

                printf("Event type %s\n", type);
                if (cookie->evtype != XI_BarrierHit)
                    break;

                if (XGetEventData (dpy, cookie)) {
                    process_barrier_event (cookie->data);
                }
                XFreeEventData (dpy, cookie);
            }
            break;
        }
    }
 out:
    XCloseDisplay(dpy);
    return 0;
}
