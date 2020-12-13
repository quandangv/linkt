#include "logger.hpp"

#include <iostream>

#include "format.hpp"

namespace logger {
  void info(const string& text) {
    std::cerr << FORMAT_BLUE_BOLD(INFO:) " " << text << std::endl;
  }
  void error(const string& text) {
    std::cerr << FORMAT_RED_BOLD(ERROR:) " " << text << std::endl;
  }
  void warn(const string& text) {
    std::cerr << FORMAT_YELLOW_BOLD(WARN:) " " << text << std::endl;
  }
  void debug(const string& text) {
    std::cerr << FORMAT_GREEN_BOLD(DEBUG:) " " << text << std::endl;
  }
}
