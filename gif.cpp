#include "gif.h"
#include "finyl.h"
#include "interface.h"


static Uint32 get_pixel(SDL_Surface *Surface, int x, int y)
{
  Uint8* bits;
  Uint32 bpp;

  if(x < 0 || x >= Surface->w)
    return 0;  // Best I could do for errors

  bpp = Surface->format->BytesPerPixel;
  bits = ((Uint8*)Surface->pixels) + y*Surface->pitch + x*bpp;

  switch (bpp)
    {
    case 1:
      return *((Uint8*)Surface->pixels + y * Surface->pitch + x);
      break;
    case 2:
      return *((Uint16*)Surface->pixels + y * Surface->pitch/2 + x);
      break;
    case 3:
      {
        // Endian-correct, but slower
        Uint8 r, g, b;
        r = *((bits)+Surface->format->Rshift/8);
        g = *((bits)+Surface->format->Gshift/8);
        b = *((bits)+Surface->format->Bshift/8);
        return SDL_MapRGB(Surface->format, r, g, b);
      }
      break;
    case 4:
      return *((Uint32*)Surface->pixels + y * Surface->pitch/4 + x);
      break;
    }

  return 0;  // FIXME: Handle errors better
}


static void set_pixel(SDL_Surface* surface, int x, int y, Uint32 color)
{
  int bpp = surface->format->BytesPerPixel;
  Uint8* bits = ((Uint8 *)surface->pixels) + y*surface->pitch + x*bpp;

  /* Set the pixel */
  switch(bpp)
    {
    case 1:
      *((Uint8 *)(bits)) = (Uint8)color;
      break;
    case 2:
      *((Uint16 *)(bits)) = (Uint16)color;
      break;
    case 3: { /* Format/endian independent */
      Uint8 r,g,b;
      r = (color >> surface->format->Rshift) & 0xFF;
      g = (color >> surface->format->Gshift) & 0xFF;
      b = (color >> surface->format->Bshift) & 0xFF;
      *((bits)+surface->format->Rshift/8) = r;
      *((bits)+surface->format->Gshift/8) = g;
      *((bits)+surface->format->Bshift/8) = b;
    }
      break;
    case 4:
      *((Uint32 *)(bits)) = (Uint32)color;
      break;
    }
}

static SDL_Surface* gif_create_surface32(Uint32 width, Uint32 height)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#else
  return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#endif
}

SDL_Palette* SDLCALL GIF_CreatePalette(int ncolors, SDL_Color* colors)
{
  SDL_Palette* result = SDL_AllocPalette(ncolors);
    
  int i = 0;
  for(i = 0; i < ncolors; ++i)
    {
      result->colors[i] = colors[i];
    }
    
  return result;
}


// Load a GIF image from a file.
GIF_Image* SDLCALL GIF_LoadImage(const char* filename)
{
  int error;
  GifFileType* gif = DGifOpenFileName(filename, &error);
  if(gif == NULL)
    return NULL;

  if(DGifSlurp(gif) == GIF_ERROR)
    {
      DGifCloseFile(gif, &error);
      return NULL;
    }
    
  GIF_Image* result = (GIF_Image*)SDL_malloc(sizeof(GIF_Image));
  memset(result, 0, sizeof(GIF_Image));
  result->width = gif->SWidth;
  result->height = gif->SHeight;
    
  // Unused?
  //gif->SColorResolution  // Number of bits per color channel, roughly.
  // Binary 111 (7) -> 8 bits per channel
  // Binary 001 (1) -> 2 bits per channel
  //gif->AspectByte
  //gif->SBackGroundColor
    
  SDL_Palette* global_palette = NULL;
  int i;
  int j;
    
  if(gif->SColorMap != NULL)
    {
      SDL_Color* global_colors = (SDL_Color*)SDL_malloc(sizeof(SDL_Color)*gif->SColorMap->ColorCount);
      for(i = 0; i < gif->SColorMap->ColorCount; ++i)
        {
          global_colors[i].r = gif->SColorMap->Colors[i].Red;
          global_colors[i].g = gif->SColorMap->Colors[i].Green;
          global_colors[i].b = gif->SColorMap->Colors[i].Blue;
          global_colors[i].a = 255;
        }
        
      global_palette = GIF_CreatePalette(gif->SColorMap->ColorCount, global_colors);
    }
    
  for(j = 0; j < gif->ExtensionBlockCount; ++j)
    {
      if(gif->ExtensionBlocks[j].Function == APPLICATION_EXT_FUNC_CODE)
        {
          if(gif->ExtensionBlocks[j].ByteCount >= 14)
            {
              char name[12];
              memcpy(name, gif->ExtensionBlocks[j].Bytes, 11);
              if(strcmp(name, "NETSCAPE2.0") == 0)
                {
                  result->num_loops = gif->ExtensionBlocks[j].Bytes[12] + gif->ExtensionBlocks[j].Bytes[13]*256;
                }
            }
        }
    }
    
  result->num_frames = gif->ImageCount;
  result->frames = (GIF_Frame**)SDL_malloc(sizeof(GIF_Frame*)*result->num_frames);
  memset(result->frames, 0, sizeof(GIF_Frame*)*result->num_frames);
    
  for(i = 0; i < gif->ImageCount; ++i)
    {
      SavedImage* img = &gif->SavedImages[i];
        
      GIF_Frame* frame = (GIF_Frame*)SDL_malloc(sizeof(GIF_Frame));
      memset(frame, 0, sizeof(GIF_Frame));
        
      result->frames[i] = frame;
        
      // Load basic attributes
      frame->width = img->ImageDesc.Width;
      frame->height = img->ImageDesc.Height;
      frame->left_offset = img->ImageDesc.Left;
      frame->top_offset = img->ImageDesc.Top;
        
        
      // Load palette
      if(img->ImageDesc.ColorMap != NULL)
        {
          SDL_Color* local_colors = (SDL_Color*)SDL_malloc(sizeof(SDL_Color)*img->ImageDesc.ColorMap->ColorCount);
          for(j = 0; j < img->ImageDesc.ColorMap->ColorCount; ++j)
            {
              local_colors[j].r = img->ImageDesc.ColorMap->Colors[j].Red;
              local_colors[j].g = img->ImageDesc.ColorMap->Colors[j].Green;
              local_colors[j].b = img->ImageDesc.ColorMap->Colors[j].Blue;
              local_colors[j].a = 255;
            }
            
          frame->local_palette = GIF_CreatePalette(img->ImageDesc.ColorMap->ColorCount, local_colors);
        }
        
      SDL_Palette* pal = global_palette;
      if(frame->local_palette != NULL)
        pal = frame->local_palette;
        
      // Look for graphics extension to get delay and transparency
      for(j = 0; j < img->ExtensionBlockCount; ++j)
        {
          if(img->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE)
            {
              Uint8 block[4];
              memcpy(block, img->ExtensionBlocks[j].Bytes, 4);
            
              // Check for transparency
              if(block[0] & 0x01)
                frame->use_transparent_index = SDL_TRUE;
              frame->transparent_index = block[3];
                
              // Disposal mode
              frame->overlay_previous = SDL_bool((block[0] & 0x08) != 0x08);
                
              // Reconstruct delay
              frame->delay = 10*(block[1] + block[2]*256);
                
              // Get transparent color
              if(pal != NULL && frame->transparent_index < pal->ncolors)
                frame->transparent_color = pal->colors[frame->transparent_index];
            }
        }
        
      int count = img->ImageDesc.Width * img->ImageDesc.Height;
      frame->surface = gif_create_surface32(frame->width, frame->height);
      frame->owns_surface = SDL_TRUE;
      if(pal != NULL)
        {
          for(j = 0; j < count; ++j)
            {
              SDL_Color c = pal->colors[img->RasterBits[j]];
              if(frame->transparent_index == img->RasterBits[j])
                c.a = 0;
                
              set_pixel(frame->surface, j%frame->width, j/frame->width, SDL_MapRGBA(frame->surface->format, c.r, c.g, c.b, c.a));
            }
        }
    }
    
  DGifCloseFile(gif, &error);

  return result;
}

void GIF::set_width_height_scale() {
  w_scale = (double)w / get_max_width();
  h_scale = (double)h / get_max_height();
}

GIF::GIF(std::string filename, int x, int y, int w, int h): filename(filename), x(x), y(y), w(w), h(h) {
  gif = GIF_LoadImage(filename.data());
  if (gif == nullptr) {
    printf("gif does not exist\n");
    exit(1);
  }

  for (int i = 0; i<gif->num_frames; i++) {
    loop_duration += gif->frames[i]->delay;
  }

  set_width_height_scale();
}

void GIF::inc_iframe() {
  iframe++;
  if (iframe+1 > gif->num_frames) {
    iframe = 0;
  }
};

void GIF::set_effective_loop_duration(double ms) {
  effective_loop_duration = ms;
  delay_multiplier = effective_loop_duration / loop_duration;
}

void GIF::dec_iframe() {
  iframe--;
  if (iframe < 0){
    iframe = 0;
  }
};

void GIF::gen_texture() {
  texture = Texture(SDL_CreateTextureFromSurface(gApp.interface->renderer, gif->frames[iframe]->surface));
}

void GIF::calc_closest_iframe() {
  double now = SDL_GetTicks();
  double minimal_diff = abs(now - tick);

  double next_tick = tick;
  while (1){
     next_tick += gif->frames[iframe]->delay*delay_multiplier;
     double new_diff = abs(next_tick - now);
     if (new_diff >= minimal_diff) {
       break;
     }
     minimal_diff = new_diff;
     tick = next_tick;
     inc_iframe();
  }
}

int GIF::get_max_width() {
  int max = 0;
  for (int i = 0; i<gif->num_frames; i++) {
    int w = gif->frames[i]->surface->w;
    if (max < w) max = w;
  }
  return max;
}

int GIF::get_max_height() {
  int max = 0;
  for (int i = 0; i<gif->num_frames; i++) {
    int h = gif->frames[i]->surface->h;
    if (max < h) max = h;
  }
  return max;
}

void GIF::draw() {
  calc_closest_iframe();
  gen_texture();
  
  SDL_Rect dst;
  dst.w = gif->frames[iframe]->surface->w * w_scale;
  dst.h = gif->frames[iframe]->surface->h * h_scale;
  dst.x = x + gif->frames[iframe]->left_offset;
  dst.y = y + gif->frames[iframe]->top_offset;
      
  SDL_RenderCopy(gApp.interface->renderer, texture.get(), NULL, &dst);

}
