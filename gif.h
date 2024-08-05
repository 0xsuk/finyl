#ifndef GIF_H
#define GIF_H

#include "IWidget.h"
#include <string>
#include <gif_lib.h>
#include <SDL2/SDL.h>
#include "sdl.h"


// Represents a single image (or animation frame)
struct GIF_Frame
{
  int left_offset, top_offset;  // Relative to containing image canvas
  Uint16 width, height;
    
  SDL_Palette* local_palette;
    
  SDL_bool use_transparent_color;
  SDL_Color transparent_color;
    
  SDL_bool use_transparent_index;
  Uint8 transparent_index;
    
    
  SDL_bool overlay_previous;  // GIF disposal method
  Uint32 delay;  // In milliseconds (though GIF stores hundredths, so it is truncated when saved).  Defaults to 100.
    
    
  // Loaded frames will have an SDL_Surface
  SDL_Surface* surface;
  SDL_bool owns_surface;
    
  // You can save frames by just palette indices
  Uint8* indices;
};

// GIF container
// Represents the data needed to render a full GIF file
struct GIF_Image
{
  Uint16 width, height;
  SDL_Palette* global_palette;
    
  SDL_bool use_background_color;  // SDL_FALSE is default and uses background_index
  Uint8 background_index;  // Defaults to 0
  SDL_Color background_color;
    
  Uint16 num_loops;  // 0 for loop animation forever
    
  Uint16 num_frames;
  Uint32 frame_storage_size;
  GIF_Frame** frames;
    
  Uint16 num_comments;
  char** comments;
    
};

class GIF: public IWidget {
 public:
  GIF(std::string filename, int x, int y, int w, int h);
  void draw();
  void set_effective_loop_duration(double ms); //gif loop start timing aligns to beat
  void align_loop(); //gif loop end aligns to bpm  
 
 private:
  void set_width_height_scale();
  int get_max_width();
  int get_max_height();
  void gen_texture();
  void inc_iframe();
  void dec_iframe();
  void calc_closest_iframe();
  
  std::string filename;
  int x;
  int y;
  int w;
  int h;
  double w_scale;
  double h_scale;
  GIF_Image* gif;
  double tick = 0; //milli sec at previous draw for iframe
  int iframe = 0; //depicted frame in the previous draw
  int loop_duration = 0; //milli sec original gif takes for a loop
  double effective_loop_duration = 0; //milli sec stretched gif takes for a loop
  double delay_multiplier = 1; //used to change gif speed
  
  Texture texture;
};

#endif
