#ifndef SDL_H
#define SDL_H

#include <SDL2/SDL.h>
#include <functional>


template <typename T, void(*D)(T*)>
class Container {
 protected:
  T* ptr;

 public:
 Container(T* p = nullptr) : ptr(p) {}
  Container(const Container&) = delete;
  Container& operator=(const Container&) = delete;
  Container(Container&& other) noexcept {
    ptr = other.ptr;
    other.ptr = nullptr;
  }
  
  Container& operator=(Container&& other) noexcept {
    if (this != &other) {
      if (ptr != nullptr) {
        D(ptr);  // Call the custom destructor on the resource
      }
      ptr = other.ptr;
      other.ptr = nullptr;
    }
    return *this;
  }

  ~Container() {
    if (ptr != nullptr) {
      D(ptr);  // Call the custom destructor on the resource
      ptr = nullptr;
    }
  }

  T* get() const {
    return ptr;
  }

  // Optionally, provide a method to manually call the destructor
  void reset(T* p = nullptr) {
    if (ptr != nullptr) {
      D(ptr);  // Call the custom destructor on the resource
    }
    ptr = p;
  }
};

using Surface = Container<SDL_Surface, SDL_FreeSurface>;
using Texture = Container<SDL_Texture, SDL_DestroyTexture>;

#endif

