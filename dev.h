#ifndef DEV_H
#define DEV_H

#include "finyl.h"

extern std::string usb;

std::string get_finyl_output_path();

void print_track_meta(finyl_track_meta& tm);
void print_track(finyl_track& t);
void print_is_stem_same(finyl_stem& s1, finyl_stem& s2);
#endif
