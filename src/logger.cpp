#include "logger.hpp"

#include <iostream>

#include "format.hpp"

namespace lg {
  ostream& inf() {
    return std::cerr << FORMAT_BLUE_BOLD(INFO:) " ";
  }
  ostream& err() {
    return std::cerr << FORMAT_RED_BOLD(ERROR:) " ";
  }
  ostream& wrn() {
    return std::cerr << FORMAT_YELLOW_BOLD(WARN:) " ";
  }
  ostream& deb() {
    return std::cerr << FORMAT_GREEN_BOLD(DEBUG:) " ";
  }
}
