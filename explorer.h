#ifndef EXPLORER_H
#define EXPLORER_H

#include "IWidget.h"
#include "finyl.h"
#include "list.h"
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>

class Explorer : public IWidget {
public:
  Explorer();
  void draw();
private:
  List list;
};

#endif
