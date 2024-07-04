#ifndef ACTION_H
#define ACTION_H
#include "finyl.h"
#include <chrono>

struct ActionTime {
  bool has_time = false;
  std::chrono::system_clock::time_point time;
};

struct ActionState {
  ActionTime last_inc_index;
  ActionTime last_dec_index;
};

void add_active_cue(Deck& deck);
void jump_cue(Deck& deck);
void set_speed(Deck& deck, double val);
void set_gain0(Deck& deck, double val);
void set_gain1(Deck& deck, double val);
void set_gain(Deck& deck, double val);
void inc_index(Deck& deck, double velocity);
void dec_index(Deck& deck, double velocity);
void inc_delta_index(Deck& deck, double velocity);
void dec_delta_index(Deck& deck, double velocity);
void toggle_playing(Deck& deck);
void toggle_playing_velocity(Deck& deck, double velocity);
void loop_in_now(Deck& deck);
void loop_out_now(Deck& deck);
void loop_in_velocity(Deck& deck, double velocity);
void loop_deactivate(Deck& deck);
void set_bqGainLow(Deck& deck, double val);
void set_delay_on(Deck& deck);
void set_delay_off(Deck& deck);
void toggle_delay(Deck& deck);
void inc_speed(Deck& deck);
void dec_speed(Deck& deck);
void sync_bpm(Deck& deck);
void press_cue_velocity(Deck& deck, double velocity);
#endif

