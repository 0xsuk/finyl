#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "finyl.h"
#include <functional>
#include "action.h"

struct ActionToFunc {
  char* base_action;
  std::function<void(double)> func;
};
extern ActionToFunc actionToFuncMap[];
extern const int len_actionToFuncMap;

void load_track(finyl_track** dest, int tid);
void* controller();
#endif
