#include <stdio.h>
#include "audio.h"

int main() {
  int out = get_playlists("/media/null/22BC-F655/");
  printf("out = %d\n", out);
}
