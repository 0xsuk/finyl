#include "usb.h"

int main() {
  auto paths = listMountedUSBPaths();
  printf("%d\n", (int)paths.size());
  return 0;
}
