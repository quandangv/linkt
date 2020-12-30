#pragma once

#include <istream>

#include "document.hpp"

namespace lini {
  void delink(document&, str_errlist&);
  void parse(std::istream& is, document&, errorlist&);
}
