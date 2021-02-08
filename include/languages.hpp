#pragma once

#include <istream>

#include "document.hpp"

namespace lini {
  using errorlist = std::vector<std::pair<std::string, std::string>>;
  std::istream& parse_ini(std::istream&, document&, errorlist&);
  std::ostream& write_ini(std::ostream&, const container&, const string& prefix = "");
  std::istream& parse_yml(std::istream&, document&, errorlist&);
  std::ostream& write_yml(std::ostream&, const container&, int indent = 0);
}
