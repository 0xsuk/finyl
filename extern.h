#ifndef EXTERN_H
#define EXTERN_H

#include "rekordbox.h"
#include "interface.h"
extern std::vector<Usb> usbs;
extern Interface interface;

extern std::string device;
extern int fps;
extern snd_pcm_uframes_t period_size;
extern snd_pcm_uframes_t period_size_2;
extern int sample_rate;
extern bool finyl_running;
extern Deck adeck;
extern Deck bdeck;
#endif
