#pragma once

#include <istream>

#include "document.hpp"

namespace lini {
  void delink(document&, str_errlist&);
  std::istream& parse(std::istream&, document&, errorlist&, const std::string& initial_section = "", char line_separator = '\n');
}
