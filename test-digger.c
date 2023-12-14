#include <stdio.h>
#include "digger.h"

char* usb = "/media/null/22BC-F655/";

int main() {
  playlist* pls;
  
  int res = get_playlists(pls, usb);

  printf("res is %d\n", res);
}
