all: singlestream multistream deviceprovider

singlestream: 
	gcc singlestream.c -o libcamera-gstreamer-singlestream `pkg-config --cflags --libs gstreamer-1.0`

multistream:
	gcc multistream.c -o libcamera-gstreamer-multistream `pkg-config --cflags --libs gstreamer-1.0`

deviceprovider:
	gcc device-provider.c -o device-provider `pkg-config --cflags --libs gstreamer-1.0`