#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "finyl.h"
#include <functional>
#include "action.h"
#include "rekordbox.h"

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
  void load_track(finyl_track** dest, int tid, finyl_deck_type deck);

  std::vector<Usb> usbs;
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
