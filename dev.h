#ifndef DEV_H
#define DEV_H

#include "finyl.h"

extern std::string usb;

char* get_finyl_output_path();

void print_track_meta(finyl_track_meta& tm);
void print_track(finyl_track& t);

#endif
