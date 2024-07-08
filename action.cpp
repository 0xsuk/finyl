#include "action.h"
#include "dsp.h"
#include <thread>
#include <functional>

//if deck is adeck, return bdeck, vice versa
Deck& get_pair(Deck& deck) {
  if (deck.type == finyl_a) {
    return bdeck;
  }
  return adeck;
}

void print_deck_name(Deck& deck) {
  if (deck.type == finyl_a) {
    printf("A ");
  } else if (deck.type == finyl_b) {
    printf("B ");
  }
}

void toggle_playing(Deck& deck) {
  deck.pTrack->playing = !deck.pTrack->playing;
  print_deck_name(deck);
  printf("is playing:%d\n", deck.pTrack->playing);
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
void set_gain0_1(Deck& deck, double val) {
  if (val < 0.5) {
    val = val*2;
    set_gain0(deck, 1);
    set_gain1(deck, val);
  } else {
    val = 1 - (val - 0.5) * 2;
    set_gain0(deck, val);
    set_gain1(deck, 1);
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
 return time * (sample_rate / 1000.0); 
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
  
  deck.pTrack->cues.push_back({.type=CUE::ACTIVE_MEMORY_CUE, .time=deck.pTrack->get_refindex()*(1.0/sample_rate)*1000.0});
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
  auto pair_deck = get_pair(deck);
  set_speed(deck, calc_bpm(deck, pair_deck));
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
  deck.delay->setMsize((sample_rate*60)/bpm*1.0);
  deck.delay->on = true;
  
  print_deck_name(deck);
  printf("delay on: %lf %lf\n", deck.delay->drymix, deck.delay->feedback);

}

void set_delay_off(Deck& deck) {
  deck.delay->on = false;
  print_deck_name(deck);
  printf("delay off\n");
}

void toggle_delay(Deck& deck) {
  if (deck.delay->on) {
    set_delay_off(deck);
  } else {
    set_delay_on(deck);
  }
}

void delay(Deck& deck, double val) {
  deck.delay->wetmix = val;
  print_deck_name(deck);
  printf("delay wet %lf\n", val);
}

void toggle_mute0(Deck& deck) {
  deck.mute0 = !deck.mute0;
  print_deck_name(deck);
  printf("mute0: %d\n", deck.mute0);
}
