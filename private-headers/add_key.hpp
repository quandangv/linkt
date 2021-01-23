#pragma once

#include "document.hpp"
#include "tstring.hpp"

#include <vector>

namespace lini {
  void add_key(document& doc, const std::string& section, const std::string& key, std::string& raw, tstring pos);
}
