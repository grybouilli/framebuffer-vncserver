#pragma once

int init_mouse(const char *touch_device, int vnc_rotate);
int init_mouse_rel(int fb_xres, int fb_yres, const char *touch_device, int vnc_rotate);
void cleanup_mouse();
void injectMouseEvent(struct fb_var_screeninfo *scrinfo, int buttonMask, int x, int y);
