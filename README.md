# framebuffer-vncserver

VNC server for Linux framebuffer devices.

![workflow](https://github.com/ponty/framebuffer-vncserver/actions/workflows/main.yml/badge.svg)

The goal is to access remote embedded Linux systems without X.
Implemented features: remote display, touchscreen, keyboard, rotation
Not implemented: file transfer, ..

Working configurations:

without rotation:
- [x]  1 bit/pixel
- [x]  8 bit/pixel
- [x]  16 bit/pixel
- [x]  24 bit/pixel
- [x]  32 bit/pixel

with rotation:
- [ ]  1 bit/pixel
- [ ]  8 bit/pixel
- [x]  16 bit/pixel
- [ ]  24 bit/pixel
- [x]  32 bit/pixel

The code is based on a LibVNC example for Android:
https://github.com/LibVNC/libvncserver/blob/master/examples/androidvncserver.c

### build

Dependency:

	sudo apt-get install libvncserver-dev

There are 2 options: CMake or qmake

Using cmake:

	mkdir -p build && cd build
	cmake ..
	make
	
Using qmake:

	mkdir -p buildqt && cd buildqt
	qmake ../framebuffer-vncserver.pro
	make

 

### command-line help 

	./framebuffer-vncserver [-f device] [-p port] [-t touchscreen] [-m mouse] [-k keyboard] [-r rotation] [-R touchscreen rotation] [-F FPS] [-v] [-h]
	-p port: VNC port, default is 5900
	-a authentication: path to password file generated with 'storepasswd'
	-A authentication: plain text password
	-c cursor: shared memory name for cursor data
	-f device: framebuffer device node, default is /dev/fb0
	-k device: keyboard device node (example: /dev/input/event0)
	-t device: touchscreen device node (example:/dev/input/event2)
	-m device: mouse device node (example:/dev/input/event2)
	-r degrees: framebuffer rotation, default is 0
	-R degrees: touchscreen rotation, default is same as framebuffer rotation
	-F FPS: Maximum target FPS, default is 10
	-v: verbose
	-h: print this help

### Handling the mouse cursor

In case the app you're running on your server isn't relying on an X server (that is, it manipulates the framebuffer directly), you will encounter the absolute mouse coordinates problem. More specifically, if it is impossible for you to retrieve the mouse absolute  coordinates with `EV_ABS` and `ABS_X`/`ABS_Y`, you will have to do the following:

* Write your own mouse driver to keep track of the cursor's absolute coordinates
* Make the driver write the mouse coordinates in a shared memory of chosen name (e.g `/cursor_pos`) using this struct:
```cpp
typedef struct {
    int x;
    int y;
} CursorPosition;
```
* Use the flag `-c` when starting the vnc server.

Here is a practical example using [lvgl](https://github.com/lvgl/lvgl) to manipulate the framebuffer:

```cpp
// -------------------------
// Driver's code

CursorPosition *cursor_shared_mem = NULL;

// Initialize shared memory
int init_shared_cursor_mem() {

    int fd = shm_open("/cursor_pos", O_CREAT | O_RDWR, 0666);

    if(fd < 0)
    {
        fprintf(stderr, "%s(): error while opening shared memory : %s", __FUNCTION__, strerror(errno));
    } 
    else
    {
        ftruncate(fd, sizeof(CursorPosition));
        cursor_shared_mem = (CursorPosition*) mmap(0, sizeof(CursorPosition), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    return fd;
}

// Custom driver
static void mouse_custom_driver(lv_indev_t * indev, lv_indev_data_t * data)
{
	// ... some custom computation ...

	// Writing the cursor position in the shared memory :
	if (cursor_shared_mem) {
        cursor_shared_mem->x = data->point.x;
        cursor_shared_mem->y = data->point.y;
    }
}

int main()
{
	// .. some init code ...
	init_shared_cursor_mem();
	lv_indev_t *touch = lv_indev_create();
    if(touch != nullptr){
        lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(touch, mouse_custom_driver); // attaching custom driver as read callback
        lv_indev_set_display(touch, disp);
	}

	// ...
	return 0;
}
```

Invoking the vnc server:
```bash
framebuffer-vncserver -c /cursor_pos -m /dev/input/event0 ...
```

## Run on startup as service

To run at startup as a service using systemd, edit the file `fbvnc.service` make sure the path and command line arguments are correct and then run:

```shell
sudo cp fbvnc.service /etc/systemd/system/
sudo systemctl enable fbvnc.service
sudo systemctl start fbvnc.service
```

## Vfb test

Linux Virtual Frame Buffer kernel object (vfb.ko) is used for this test.
https://cateee.net/lkddb/web-lkddb/FB_VIRTUAL.html

Local computer:
	
	# install
	sudo apt install vagrant virtualbox xtightvncviewer
	
	# after framebuffer-vncserver start
	vncviewer localhost

	vagrant up
	vagrant ssh

Inside vagrant box:

	sudo su
	cd /home/vagrant/build/

	# build framebuffer-vncserver
	make

	# set resolution, color depth
    fbset -g 640 480 640 480 16

	# restart framebuffer-vncserver
	killall framebuffer-vncserver;./framebuffer-vncserver -t /dev/input/ms -k /dev/input/kbd &

	# set test pattern or ..
	fb-test

	# draw random rectangles or ..
	rect
	
	# display a GUI or ...
	qmlscene -platform linuxfb -plugin evdevmouse:/dev/input/ms:abs -plugin evdevkeyboard:/dev/input/kbd:grab=1

### Automatic test 

This generates patterns with different resolutions and color depths (on local computer):
	
	python3 -m pip install fabric vncdotool python-vagrant entrypoint2
	python3 vfb.py

| rotation | color | fbtest                                  | qmlscene                                  | gradient                                  |
| -------: | ----: | --------------------------------------- | ----------------------------------------- | ----------------------------------------- |
|        0 |     1 |                                         | ![](/img/qmlscene_160x120_c1_rot0.png)    |                                           |
|        0 |     8 | ![](/img/fbtest_160x120_c8_rot0.png)    |                                           | ![](/img/gradient_160x120_c8_rot0.png)    |
|        0 |    16 | ![](/img/fbtest_160x120_c16_rot0.png)   | ![](/img/qmlscene_160x120_c16_rot0.png)   | ![](/img/gradient_160x120_c16_rot0.png)   |
|        0 |    24 | ![](/img/fbtest_160x120_c24_rot0.png)   | ![](/img/qmlscene_160x120_c24_rot0.png)   | ![](/img/gradient_160x120_c24_rot0.png)   |
|        0 |    32 | ![](/img/fbtest_160x120_c32_rot0.png)   | ![](/img/qmlscene_160x120_c32_rot0.png)   | ![](/img/gradient_160x120_c32_rot0.png)   |
|       90 |    16 | ![](/img/fbtest_160x120_c16_rot90.png)  | ![](/img/qmlscene_160x120_c16_rot90.png)  | ![](/img/gradient_160x120_c16_rot90.png)  |
|      180 |    16 | ![](/img/fbtest_160x120_c16_rot180.png) | ![](/img/qmlscene_160x120_c16_rot180.png) | ![](/img/gradient_160x120_c16_rot180.png) |
|      270 |    16 | ![](/img/fbtest_160x120_c16_rot270.png) | ![](/img/qmlscene_160x120_c16_rot270.png) | ![](/img/gradient_160x120_c16_rot270.png) |

## Testing single-touch

	$ (evtest /dev/input/event0 &) ;./framebuffer-vncserver -t /dev/input/event0 -v
	...
	Initializing touch device /dev/input/event0 ...
	x:(0 4095)  y:(0 4095) 
	...
	Supported events:
	Event type 0 (EV_SYN)
	Event type 1 (EV_KEY)
		Event code 330 (BTN_TOUCH)
	Event type 3 (EV_ABS)
		Event code 0 (ABS_X)
		Value   1970
		Min        0
		Max     4095
		Event code 1 (ABS_Y)
		Value   1745
		Min        0
		Max     4095
		Event code 24 (ABS_PRESSURE)
		Value      0
		Min        0
		Max      255
	...
	Got ptrevent: 0001 (x=186, y=570)
	Event: time 1580221917.655639, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 1
	Event: time 1580221917.655639, type 3 (EV_ABS), code 0 (ABS_X), value 1586
	Event: time 1580221917.655639, type 3 (EV_ABS), code 1 (ABS_Y), value 2733
	Event: time 1580221917.655639, -------------- SYN_REPORT ------------
	injectTouchEvent (screen(186,570) -> touch(1586,2733), mouse=1)
	...
	Got ptrevent: 0000 (x=186, y=570)
	Event: time 1580221918.516897, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
	Event: time 1580221918.516897, -------------- SYN_REPORT ------------
	injectTouchEvent (screen(186,570) -> touch(1586,2733), mouse=0)

## Testing multi-touch

	$ (evtest /dev/input/event2 &) ;./framebuffer-vncserver -t /dev/input/event2 -v
	...
	Supported events:
	Event type 0 (EV_SYN)
	Event type 1 (EV_KEY)
		Event code 330 (BTN_TOUCH)
	Event type 3 (EV_ABS)
		Event code 0 (ABS_X)
		Value    245
		Min        0
		Max      480
		Event code 1 (ABS_Y)
		Value    485
		Min        0
		Max      854
		Event code 47 (ABS_MT_SLOT)
		Value      4
		Min        0
		Max        4
		Event code 48 (ABS_MT_TOUCH_MAJOR)
		Value      0
		Min        0
		Max      255
		Event code 50 (ABS_MT_WIDTH_MAJOR)
		Value      0
		Min        0
		Max      255
		Event code 53 (ABS_MT_POSITION_X)
		Value      0
		Min        0
		Max      480
		Event code 54 (ABS_MT_POSITION_Y)
		Value      0
		Min        0
		Max      854
		Event code 57 (ABS_MT_TRACKING_ID)
		Value      0
		Min        0
		Max    65535
	...
	Initializing touch device /dev/input/event2 ...
	x:(0 480)  y:(0 854) 
	...
	Got ptrevent: 0001 (x=237, y=528)
	Event: time 1580221680.870277, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 0
	Event: time 1580221680.870277, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 1
	Event: time 1580221680.870277, type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 237
	Event: time 1580221680.870277, type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 528
	Event: time 1580221680.870277, type 3 (EV_ABS), code 0 (ABS_X), value 237
	Event: time 1580221680.870277, type 3 (EV_ABS), code 1 (ABS_Y), value 528
	Event: time 1580221680.870277, -------------- SYN_REPORT ------------
	injectTouchEvent (screen(237,528) -> touch(237,528), mouse=1)
	...
	Got ptrevent: 0000 (x=237, y=528)
	Event: time 1580221681.190716, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value -1
	Event: time 1580221681.190716, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
	Event: time 1580221681.190716, -------------- SYN_REPORT ------------
	injectTouchEvent (screen(237,528) -> touch(237,528), mouse=0)
