#include "action.h"
#include "dsp.h"
#include <thread>
#include <functional>
#include "controller.h"
#include "dev.h"
#include <cctype>
#include <algorithm>

//if deck is adeck, return bdeck, vice versa
std::unique_ptr<Deck>& get_pair(Deck& deck) {
  if (deck.type == finyl_a) {
    return gApp.controller->bdeck;
  }
  return gApp.controller->adeck;
}

void toggle_playing(Deck& deck) {
  deck.pTrack->playing = !deck.pTrack->playing;
  print_deck_name(deck);
  printf("is playing:%d\n", deck.pTrack->playing.load());
}

void toggle_playing_velocity(Deck& deck, double velocity) {
  if (velocity != 0) {
    toggle_playing(deck);
  }
}

void set_speed(Deck& deck, double val) {
  deck.pTrack->set_speed(val);
  print_deck_name(deck);
  printf("speed %lf\n", val);
}

void set_gain0(Deck& deck, double val) {
  deck.gain0 = val;
  print_deck_name(deck);
  printf("gain0 %lf\n", val);
}
void set_gain1(Deck& deck, double val) {
  deck.gain1 = val;
  print_deck_name(deck);
  printf("gain1 %lf\n", val);
}
void set_gain(Deck& deck, double val) {
  deck.gain = val;
  print_deck_name(deck);
  printf("gain %lf\n", val);
}
double scale = 0.3;
void set_gain0_1(Deck& deck, double val) {
  if (val < 0.5) {
    val = val * 2; //0 to 1
    set_gain0(deck, scale);
    set_gain1(deck, val*scale);
  } else {
    val = (1 - val) * 2; //0 to 1
    set_gain0(deck, val*scale);
    set_gain1(deck, scale);
  }
}

void set_index_quantized(Deck& deck) {
  auto quantized = finyl_get_quantized_index(*deck.pTrack, deck.pTrack->get_refindex());
  deck.pTrack->set_index(quantized);
}

void do_while_velocity(double velocity, std::function<void()> off, std::function<void()> f, std::chrono::system_clock::time_point& time, bool& has_time) {
  if (velocity == 0) {
    off();
    return;
  }
  
  //moving because if f is lambda (in stack) it dies
  std::thread([&, ff = std::move(f)](){
    time = std::chrono::system_clock::now();
    has_time = true;
    
    while (has_time) {
      ff();
      
      auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time).count();

      bool is_first_call = diff < 500; //dont want to go next loop until 700 millisec elapses from the first call
      
      if (is_first_call) {
        usleep(500*1000);
      } else {
        usleep(10*1000);
      }
    }
  }).detach();
}

void inc_index(Deck& deck, double velocity) {
  do_while_velocity(velocity,
                    [&deck](){deck.action_state->last_inc_index.has_time = false;},
                    [&deck](){deck.pTrack->set_index(deck.pTrack->get_refindex() + 3000);},
                    deck.action_state->last_inc_index.time,
                    deck.action_state->last_inc_index.has_time);
}

void dec_index(Deck& deck, double velocity) {
  do_while_velocity(velocity,
                    [&deck](){deck.action_state->last_dec_index.has_time = false;},
                    [&deck](){deck.pTrack->set_index(deck.pTrack->get_refindex() - 3000);},
                    deck.action_state->last_dec_index.time,
                    deck.action_state->last_dec_index.has_time);
}

void inc_delta_index(Deck& deck, double velocity) {
  do_while_velocity(velocity,
                    [&deck](){deck.action_state->last_inc_index.has_time = false;},
                    [&deck](){deck.pTrack->set_index(deck.pTrack->get_refindex() + 300);},
                    deck.action_state->last_inc_index.time,
                    deck.action_state->last_inc_index.has_time);
}

void dec_delta_index(Deck& deck, double velocity) {
  do_while_velocity(velocity,
                    [&deck](){deck.action_state->last_dec_index.has_time = false;},
                    [&deck](){deck.pTrack->set_index(deck.pTrack->get_refindex() - 300);},
                    deck.action_state->last_dec_index.time,
                    deck.action_state->last_dec_index.has_time);
}

double millisec_to_index(double time) {
 return time * (gApp.audio->get_sample_rate() / 1000.0); 
}

static void deactivate_memory_cues(Deck& deck) {
  for (auto& cue: deck.pTrack->cues) {
    if (cue.type == CUE::ACTIVE_MEMORY_CUE) {
      cue.type = CUE::MEMORY_CUE;
    }
  }
}

void add_active_cue(Deck& deck) {
  //add cue
  //1   index = 1/sample_rate second
  //now index = 1/sample_rate * now second
  
  deactivate_memory_cues(deck);
  
  loop_in_now(deck);
  deck.pTrack->set_index(deck.pTrack->loop_in); //quantize refindex
  
  deck.pTrack->cues.push_back({.type=CUE::ACTIVE_MEMORY_CUE, .time=deck.pTrack->get_refindex()*(1.0/gApp.audio->get_sample_rate())*1000.0});
  print_deck_name(deck);
  printf("cue add %lf\n", deck.pTrack->get_refindex());
}

void set_memory_cue_active(Deck& deck, finyl_cue& cue) {
  cue.type = CUE::ACTIVE_MEMORY_CUE;
  
  loop_in_now(deck);
  deck.pTrack->set_index(deck.pTrack->loop_in);
  print_deck_name(deck);
  printf("cue set %lf\n", deck.pTrack->get_refindex());
}

finyl_cue& get_active_memory_cue(Deck& deck) {
  for (auto& cue: deck.pTrack->cues) {
    if (cue.type == CUE::ACTIVE_MEMORY_CUE) {
      return cue;
    }
  }

  throw std::runtime_error("get_active_memory_cue: no cue\n");
}

//if memory cue exist, then active memory_cue also exist
bool memory_cue_exist(Deck& deck) {
  for (auto& cue: deck.pTrack->cues) {
    if (cue.type == CUE::MEMORY_CUE || cue.type == CUE::ACTIVE_MEMORY_CUE) {
      return true;
    }
  }
  return false;
}

void jump_cue(Deck& deck) {
  if (!memory_cue_exist(deck)) return;
  
  auto cue = get_active_memory_cue(deck);
  
  deck.pTrack->jump_lock = true;
  deck.pTrack->set_index(millisec_to_index(cue.time));
  printf("jumped to %lf\n", deck.pTrack->get_refindex());
}

bool is_at_active_memory_cue(Deck& deck) {
  if (!memory_cue_exist(deck)) return false;
  double diff = abs(millisec_to_index(get_active_memory_cue(deck).time) - deck.pTrack->get_refindex());
  return diff < 0.1;
}

bool is_at_memory_cue(Deck& deck, finyl_cue& out) {
  if (!memory_cue_exist(deck)) return false;
  for (auto& cue: deck.pTrack->cues) {
    double diff = abs(millisec_to_index(cue.time) - deck.pTrack->get_refindex());
    if (diff < 0.1) {
      out = cue;
      return true;
    }
  }

  return false;
}

void press_cue_velocity(Deck& deck, double velocity) {
  if (deck.pTrack->playing) {
    //TODO and speed isnt small
    if (memory_cue_exist(deck)) {
      deck.pTrack->playing = false;
      jump_cue(deck);
    } else {
    }
  } else {
    //TODO when speed is small
    if (!is_at_active_memory_cue(deck) && velocity != 0) {
      finyl_cue cue;
      if (is_at_memory_cue(deck, cue)) {
        set_memory_cue_active(deck, cue);
      } else {
        add_active_cue(deck);
      }
      return;
    }
    if (!is_at_active_memory_cue(deck) && velocity == 0) {
      return;
    }
    
    //is at active cue
    if (velocity != 0) {
      deck.pTrack->playing = true;
    }
  }
}

void loop_in(Deck& deck, double index) {
  deck.pTrack->loop_in = index;
  deck.pTrack->loop_active = false;
}

void loop_in_now(Deck& deck) {
  double now = deck.pTrack->get_refindex();
  if (deck.quantize) {
    now = finyl_get_quantized_index(*deck.pTrack, now);
  }
  loop_in(deck, now);
}

void loop_out(Deck& deck, double index) {
  if (index <= deck.pTrack->loop_in) {
    return;
  }

  if (deck.pTrack->loop_in == -1) {
    deck.pTrack->loop_in = 0;
  }
  deck.pTrack->loop_out = index;
  deck.pTrack->loop_active = true;
  
}

void loop_out_now(Deck& deck) {
  double now = deck.pTrack->get_refindex();
  if (deck.quantize) {
    now = finyl_get_quantized_index(*deck.pTrack, now);
  }
  loop_out(deck, now);
}

void loop_deactivate(Deck& deck) {
  deck.pTrack->loop_active = false;
}

static double calc_bpm(Deck& sync, Deck& master) {
  double ratio = (double)master.pTrack->meta.bpm / sync.pTrack->meta.bpm;
  
  //ratio+x = 1;
  //ratio*2 = 1+x;
  //then, ratio * 3 = 2; 
  //ratio = 2/3 = 0.66;
  //if ratio < 0.66, 
  if (ratio < 0.66) {
    ratio *= 2;
  }
  
  //ratio-x = 1;
  //ratio/2 = 1-x;
  //then, ratio * 3/2 = 2;
  //ratio = 2*2/3 = 4/3 = 1.33;
  else if (ratio > 1.33) {
    ratio /=2;
  }
  
  return master.pTrack->get_speed() * ratio;
}

void sync_bpm(Deck& deck) {
  auto& pair_deck = get_pair(deck);
  set_speed(deck, calc_bpm(deck, *pair_deck));
}

void inc_speed(Deck& deck) {
  set_speed(deck, deck.pTrack->get_speed()+0.01);
}

void dec_speed(Deck& deck) {
  set_speed(deck, deck.pTrack->get_speed() - 0.01);
}

void set_bqGainLow(Deck& deck, double val) {
  //scale val to -23 to 12;, 0.5 to 0.0
  val = val*2 - 1; //range: [-1 1]
  if (val < 0) val = val*23.0;
  else val = val*12.0;
  
  deck.bqisoState->bqGainLow = val;
  
  print_deck_name(deck);
  printf("bqisoState.bqGainLow: %lf\n", deck.bqisoState->bqGainLow);
}
      
void set_delay_on(Deck& deck) {
  //bpm beats is 44100*60 samples
  //1 beat is 44100*60/bpm samples 
  double bpm = (deck.pTrack->meta.bpm/100.0)*deck.pTrack->get_speed();
  deck.delayState->setMsize((gApp.audio->get_sample_rate()*60)/bpm*1.0);
  deck.delayState->on = true;
  
  print_deck_name(deck);
  printf("delay on: %lf %lf\n", deck.delayState->drymix, deck.delayState->feedback);

}

void set_delay_off(Deck& deck) {
  deck.delayState->on = false;
  print_deck_name(deck);
  printf("delay off\n");
}

void toggle_delay(Deck& deck) {
  if (deck.delayState->on) {
    set_delay_off(deck);
  } else {
    set_delay_on(deck);
  }
}

void delay(Deck& deck, double val) {
  deck.delayState->wetmix = val;
  print_deck_name(deck);
  printf("delay wet %lf\n", val);
}

void toggle_mute0(Deck& deck) {
  deck.mute0 = !deck.mute0;
  print_deck_name(deck);
  printf("mute0: %d\n", deck.mute0);
}

void toggle_master(Deck &deck) {
  deck.master = !deck.master;
}

// Key shift functions
void inc_key_shift(Deck& deck) {
  if (deck.pTrack == nullptr) return;
  deck.pTrack->inc_key_shift();
  print_deck_name(deck);
  printf("key shift +1 semitone, total: %d semitones\n", deck.pTrack->get_key_shift_semitones());
}

void dec_key_shift(Deck& deck) {
  if (deck.pTrack == nullptr) return;
  deck.pTrack->dec_key_shift();
  print_deck_name(deck);
  printf("key shift -1 semitone, total: %d semitones\n", deck.pTrack->get_key_shift_semitones());
}

void reset_key_shift(Deck& deck) {
  if (deck.pTrack == nullptr) return;
  deck.pTrack->reset_key_shift();
  print_deck_name(deck);
  printf("key shift reset to 0 semitones\n");
}

void set_key_shift_semitones(Deck& deck, int semitones) {
  if (deck.pTrack == nullptr) return;
  deck.pTrack->set_key_shift_semitones(semitones);
  print_deck_name(deck);
  printf("key shift set to %d semitones\n", semitones);
}

// Key shift align functions
int get_key_semitones_from_string(const std::string& key) {
  if (key.empty() || key == "-") return 0;
  
  // Convert key string to semitone offset from C
  // Common key formats: "C", "C#", "Db", "1A", "2B", etc.
  
  // Handle Camelot notation (1A-12B format)
  if (key.length() >= 2 && isdigit(key[0]) && (key[1] == 'A' || key[1] == 'B')) {
    int number = key[0] - '0';
    if (key.length() >= 3 && isdigit(key[1])) {
      number = number * 10 + (key[1] - '0');
    }
    
    // Camelot wheel mapping to semitones
    // 1A=Ab, 2A=Eb, 3A=Bb, 4A=F, 5A=C, 6A=G, 7A=D, 8A=A, 9A=E, 10A=B, 11A=F#, 12A=Db
    // 1B=B, 2B=F#, 3B=Db, 4B=Ab, 5B=Eb, 6B=Bb, 7B=F, 8B=C, 9B=G, 10B=D, 11B=A, 12B=E
    if (key.back() == 'A') {
      int camelot_to_semitone[] = {8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6, 1}; // Ab, Eb, Bb, F, C, G, D, A, E, B, F#, Db
      if (number >= 1 && number <= 12) {
        return camelot_to_semitone[number - 1];
      }
    } else if (key.back() == 'B') {
      int camelot_to_semitone[] = {11, 6, 1, 8, 3, 10, 5, 0, 7, 2, 9, 4}; // B, F#, Db, Ab, Eb, Bb, F, C, G, D, A, E
      if (number >= 1 && number <= 12) {
        return camelot_to_semitone[number - 1];
      }
    }
    return 0;
  }
  
  // Handle standard musical notation (C, C#, Db, etc.)
  char root = toupper(key[0]);
  int semitones = 0;
  
  switch (root) {
    case 'C': semitones = 0; break;
    case 'D': semitones = 2; break;
    case 'E': semitones = 4; break;
    case 'F': semitones = 5; break;
    case 'G': semitones = 7; break;
    case 'A': semitones = 9; break;
    case 'B': semitones = 11; break;
    default: return 0;
  }
  
  // Handle sharps and flats
  if (key.length() > 1) {
    if (key[1] == '#' || key[1] == 's') {
      semitones += 1;
    } else if (key[1] == 'b' || key[1] == 'f') {
      semitones -= 1;
    }
  }
  
  // Wrap around if necessary
  if (semitones < 0) semitones += 12;
  if (semitones >= 12) semitones -= 12;
  
  return semitones;
}

int calculate_key_difference(const std::string& key1, const std::string& key2) {
  int semitones1 = get_key_semitones_from_string(key1);
  int semitones2 = get_key_semitones_from_string(key2);
  
  int diff = semitones2 - semitones1;
  
  // Find the shortest path around the circle of fifths
  if (diff > 6) {
    diff -= 12;
  } else if (diff < -6) {
    diff += 12;
  }
  
  return diff;
}

void align_key_to_other_deck(Deck& deck) {
  if (deck.pTrack == nullptr) {
    print_deck_name(deck);
    printf("No track loaded on this deck\n");
    return;
  }
  
  // Get the other deck
  std::unique_ptr<Deck>& other_deck = get_pair(deck);
  if (other_deck->pTrack == nullptr) {
    print_deck_name(deck);
    printf("No track loaded on the other deck\n");
    return;
  }
  
  // Get the keys from both tracks
  std::string current_key = deck.pTrack->meta.musickey;
  std::string target_key = other_deck->pTrack->meta.musickey;
  
  if (current_key.empty() || current_key == "-" || target_key.empty() || target_key == "-") {
    print_deck_name(deck);
    printf("Key information not available for one or both tracks\n");
    printf("Current track key: %s, Other track key: %s\n", current_key.c_str(), target_key.c_str());
    return;
  }
  
  // Calculate the key difference
  int key_diff = calculate_key_difference(current_key, target_key);
  
  // Apply the key shift
  set_key_shift_semitones(deck, key_diff);
  
  print_deck_name(deck);
  printf("Key align: %s -> %s (shift: %+d semitones)\n", 
         current_key.c_str(), target_key.c_str(), key_diff);
}
