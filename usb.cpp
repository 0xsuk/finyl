#include <iostream>
#include <fstream>
#include "usb.h"

//TODO: experimental
std::vector<std::string> listMountedUSBPaths() {
  std::vector<std::string> paths;
  
  std::ifstream mounts("/proc/mounts");
  if (!mounts.is_open()) {
    std::cerr << "Failed to open /proc/mounts" << std::endl;
    return {};
  }

  std::string line;
  while (std::getline(mounts, line)) {
    if (line.find("/dev/sd") != std::string::npos || line.find("/dev/usb") != std::string::npos) {
      auto pos = line.find(' ');
      if (pos != std::string::npos) {
        // std::string device = line.substr(0, pos);
        std::string mountPoint = line.substr(pos + 1, line.find(' ', pos + 1) - pos - 1);

        paths.push_back(mountPoint);
      }
    }
  }

  return paths;
}
