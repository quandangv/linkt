#pragma once

#include "base.hpp"

namespace node {
  struct strsub : base<string> {
    struct replace_spot {
      mutable size_t start, length;
      base_s replacement;
      replace_spot(size_t pos, base_s repl) : start(pos), length(0), replacement(repl) {}
      replace_spot(size_t start, size_t length, base_s repl)
          : start(start), length(length), replacement(repl) {}
    };
    mutable string base, tmp;
    std::vector<replace_spot> spots;

    explicit operator string() const;
    base_s clone  (clone_context&) const;
    bool is_fixed() const;
  };
}
