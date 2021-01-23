#pragma once

#include <istream>

#include "document.hpp"

namespace lini {
  using errorlist = std::vector<std::pair<std::string, std::string>>;
  std::istream& parse(std::istream&, document&, errorlist&, const std::string& initial_section = "");
  std::ostream& write(std::ostream&, const document&);
}
