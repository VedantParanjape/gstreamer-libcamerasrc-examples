all:
	gcc main.c -o libcamera-gstreamer `pkg-config --cflags --libs gstreamer-1.0`