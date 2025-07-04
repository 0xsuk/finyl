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
void set_gain0_1(Deck& deck, double val);
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
void delay(Deck& deck, double val);
void toggle_delay(Deck& deck);
void inc_speed(Deck& deck);
void dec_speed(Deck& deck);
void sync_bpm(Deck& deck);
void press_cue_velocity(Deck& deck, double velocity);
void toggle_mute0(Deck& deck);
void toggle_master(Deck& deck);

// Key shift functions
void inc_key_shift(Deck& deck);
void dec_key_shift(Deck& deck);
void reset_key_shift(Deck& deck);
void set_key_shift_semitones(Deck& deck, int semitones);

// Key shift align functions
void align_key_to_other_deck(Deck& deck);
int get_key_semitones_from_string(const std::string& key);
int calculate_key_difference(const std::string& key1, const std::string& key2);

#endif

