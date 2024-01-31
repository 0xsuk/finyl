#ifndef DEV_H
#define DEV_H

#include "finyl.h"
#include <chrono>
#define NOW std::chrono::system_clock::now()
#define DURATION(a, start) printf("%s took %ld micro\n", a, std::chrono::duration_cast<std::chrono::microseconds>(NOW-start).count())

void print_track_meta(finyl_track_meta& tm);
void print_track(finyl_track& t);
void print_is_stem_same(finyl_stem& s1, finyl_stem& s2);
void report(finyl_buffer& buffer);
#endif
