The project forks of https://bitbucket.org/droidperception/simplewebcam.

The project supports for usb camera preview and capture on android platform.

The project supports mjpeg format of preview frame.

To run this application, the following conditions should be satisfied.

1) The kernel is V4L2 enabled, e.g.,

 CONFIG_VIDEO_DEV=y

 CONFIG_VIDEO_V4L2_COMMON=y

 CONFIG_VIDEO_MEDIA=y

 CONFIG_USB_VIDEO_CLASS=y

 CONFIG_V4L_USB_DRIVERS=y

 CONFIG_USB_VIDEO_CLASS_INPUT_EVDEV=y

2) The permission of /dev/video0 is set 0666 in /ueventd.xxxx.rc

3) USB WebCam is UVC camera, and it supports 1280x720 resolution with YUYV format.

Supported platform : Iconia Tab A500.

 This application will also work on V4L2-enabled pandaboard and beagleboard.
