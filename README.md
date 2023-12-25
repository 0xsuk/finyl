# finyl
![finyl](preview.png)

`finyl.c, finyl.h`: minimal multi-stems audio mixing library that provides a core mechanism to handle DJ operation (audio playback with bpm adjustment/sync, loop, beat snap, etc). Depends on `ALSA, ffmpeg(cli)`.


The finyl software is a linux application that implements GUI interface and audio playback controls on top of finyl library. The finyl software is designed to be memory efficient, so it works well even on Raspberry pi 3b+ 1GB RAM. Depends on `sdl2 x11 openssl`
