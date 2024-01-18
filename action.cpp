#include "action.h"

void toggle_playing(finyl_track* t) {
  if (t == NULL) {
    return;
  }

  t->playing = !t->playing;
}

void loop_in_now(finyl_track* t) {
  double now = t->index;
  if (t == NULL) {
    return;
  }

  t->loop_in = finyl_get_quantized_index(*t, now);
  t->loop_active = false;
}

void loop_out_now(finyl_track* t) {
  double now = t->index;
  if (t == NULL) {
    return;
  }

  auto tmp = finyl_get_quantized_index(*t, now);
  if (tmp <= t->loop_in) {
    return;
  }

  if (t->loop_in == -1) {
    t->loop_in = 0;
  }
  t->loop_out = tmp;
  t->loop_active = true;
}
