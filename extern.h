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

extern finyl_track* adeck; //pointer to track, is a d
extern finyl_track* bdeck;
extern finyl_track* cdeck;
extern finyl_track* ddeck;


extern double a_gain;
extern double a0_gain;
extern double a1_gain;
extern double b_gain;
extern double b0_gain;
extern double b1_gain;
class Delay;
extern Delay a_delay;
extern Delay b_delay;
#endif
