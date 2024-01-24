# finyl
finyl is a rekordbox USB compatible multi-stems DJ system for Linux 64 bits that lets you mix audio sources separately. (vocal, instrumental, etc)  
It is designed to work well on Raspberry pi 3b+ RAM 1GB.

![finyl](preview.png)


## build
make  
(c++20 compiler needed)  

Depends on...  
in debian: alsa, ffmpeg(cli), libsdl2-dev, libx11-dev, libopenssl-dev, java  

## usage
./finyl <path to rekordbox usb: example /media/null/22BC-F655/ >

read controller.cpp:handleKey() to guess how to control finyl from terminal;
