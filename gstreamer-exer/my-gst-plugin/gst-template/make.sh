#!/bin/bash

echo "compile plugin."
source /home/fang/anaconda3/bin/activate pyth36
rm -rf meson_build
/home/fang/meson/meson.py meson_build
ninja -C meson_build/

echo "copy libgstmyfilter.so to gstreamer path."
sudo cp meson_build/gst-plugin/libgstmyfilter.so /usr/local/lib/gstreamer-1.0/

echo "done."
