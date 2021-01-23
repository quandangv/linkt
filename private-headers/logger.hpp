#pragma once

#include "common.hpp"

namespace lg {
  using std::string;
  
  ostream& inf();
  ostream& err();
  ostream& wrn();
  ostream& deb();

  template<const char* scope>
  constexpr bool has_scope() {
    return DEBUG_SCOPES.find(scope) != std::string_view::npos;
  }
}

constexpr char copy_scope[] = "copy";
