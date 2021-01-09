#pragma once

#include "tstring.hpp"

#include <string>

class string_inter {
  std::string base;
  vector<int> positions;

  string_inter() {}

public:
  std::string get(std::iterator<std::string> replacements);
};
