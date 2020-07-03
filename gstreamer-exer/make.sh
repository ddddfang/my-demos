#gcc tutorial03.c -o abc `pkg-config --cflags --libs gstreamer-1.0`
#终端敲 gcc tutorial01.c -o abc `pkg-config --cflags --libs gstreamer-1.0` 自动转换为下面:
gcc tutorial04.c -o abc -pthread -I/usr/local/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -L/usr/local/lib -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0

