gcc abc.c -o abc `pkg-config --cflags --libs x264`
#等同于:
#gcc abc.c -o abc -DX264_API_IMPORTS -I/usr/local/include -L/usr/local/lib -lx264

