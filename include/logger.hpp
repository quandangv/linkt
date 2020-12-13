#pragma once

#include <string>

#include "option.hpp"

namespace logger {
  using std::string;
  
  void info(const string& text);
  void error(const string& text);
  void warn(const string& text);
  void debug(const string& text);

  template<const char* scope>
  void debug(const string& text) {
    if constexpr(DEBUG_SCOPES.find(scope) != std::string_view::npos)
      debug(text);
  }
  constexpr const char* dumb = "dumb";
}
