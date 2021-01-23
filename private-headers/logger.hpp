#pragma once

#include "common.hpp"

namespace logger {
  using std::string;
  
  void info(const string& text);
  void error(const string& text);
  void warn(const string& text);
  void debug(const string& text);

  template<const char* scope>
  constexpr bool has_scope() {
    return DEBUG_SCOPES.find(scope) != std::string_view::npos;
  }

  template<const char* scope>
  void debug(const string& text) {
    if constexpr(has_scope<scope>())
      debug(text);
  }
  constexpr const char* dumb = "dumb";
}

constexpr char copy_scope[] = "copy";
