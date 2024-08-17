#!/bin/bash
cd ..
make
cd examples #return
gcc exe_to_img.c ../spnglib.so -lz -o exe_to_img -fsanitize=address
gcc mipmap.c ../spnglib.so -lz -o mipmap -fsanitize=address -lm
gcc rgb_to_grayscale.c ../spnglib.so -lz -o rgb2grey
gcc show_info.c ../spnglib.so -lz -o pnginfo