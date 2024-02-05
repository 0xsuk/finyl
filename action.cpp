#include "action.h"

void toggle_playing(finyl_track& t) {
  t.playing = !t.playing;
}

void cue(finyl_track& t) {
  loop_in_now(t);
  t.index = t.loop_in;
}

void loop_in_now(finyl_track& t) {
  double now = t.index;
  t.loop_in = finyl_get_quantized_index(t, now);
  t.loop_active = false;
}

void loop_out_now(finyl_track& t) {
  double now = t.index;
  auto tmp = finyl_get_quantized_index(t, now);
  if (tmp <= t.loop_in) {
    return;
  }

  if (t.loop_in == -1) {
    t.loop_in = 0;
  }
  t.loop_out = tmp;
  t.loop_active = true;
}

void loop_deactivate(finyl_track& t) {
  t.loop_active = false;
}
