#ifndef controller_H
#define controller_H
#include "finyl.h"

void load_track(finyl_track** dest, int tid);

void* controller(void* args);
#endif
