#ifndef DEV_H
#define DEV_H
#include "finyl.h"
#include <chrono>
extern std::vector<int> TIMES;
#define NOW std::chrono::system_clock::now()

void print_track_meta(finyl_track_meta& tm);
void print_track(finyl_track& t);
void print_is_stem_same(finyl_stem& s1, finyl_stem& s2);
void report(finyl_buffer& buffer);
void duration(std::chrono::system_clock::time_point start);
void profile();
void print_deck_name(Deck& deck); 

#endif
