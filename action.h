#ifndef ACTION_H
#define ACTION_H
#include "finyl.h"

void set_cue(finyl_track& t);
void toggle_playing(finyl_track& t);
void loop_in_now(finyl_track& t);
void loop_out_now(finyl_track& t);
void loop_deactivate(finyl_track& t);
#endif

