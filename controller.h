#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "finyl.h"
#include <functional>
#include "action.h"
#include "rekordbox.h"
#include <SDL2/SDL.h>

struct ActionToFunc {
  char* base_action;
  std::function<void(double)> func;
};

#define ifvelocity(exp) if (velocity!=0) {exp}

class Interface;

class Controller {
 public:
  Controller();
  void run();
  void load_track_nstems(finyl_track** dest, int tid, finyl_deck_type deck, int n);
  void _load_track_nstems(finyl_track** dest, int tid, finyl_deck_type deck, int n);
  void load_track(finyl_track** dest, int tid, finyl_deck_type deck);
  void handle_sdl_key(const SDL_Event& event);
  void scan_usbs();

  std::vector<Usb> usbs; //usbs that are scanned and recognized as rekordbox usb
  std::unique_ptr<Deck> adeck; //initialized when on_period_size_change
  std::unique_ptr<Deck> bdeck;
  
  //val and velocity is 0 to 1
  std::vector<ActionToFunc> actionToFuncMap;
 
 private:
  void keyboard_handler();
  void handle_key(char x);
  void handle_midi(int len, unsigned char buf[]);
  
};

#endif
