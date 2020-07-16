#!/bin/bash

g++ -std=c++11 -Wall -g -o repl repl.cpp \
    -I/home/teaonly/opt/ffmpeg/include \
    -L/home/teaonly/opt/ffmpeg/lib \
    -lavutil -lswresample -lswscale -lavcodec -lavformat -lavfilter -lavdevice -lffhelp -lreadline -pthread
