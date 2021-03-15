#pragma once

#include "base.hpp"

namespace node {
  struct with_fallback {
    base_s fallback;

    with_fallback  () {}
    explicit with_fallback  (const base_s& fallback) : fallback(fallback) {}
    [[nodiscard]] string use_fallback  (const string& error_message) const;
  };

  struct fallback_wrapper : base<string>, settable, private with_fallback {
    const base_s value;

    fallback_wrapper(base_s value, base_s fallback);
    string get  () const;
    bool set  (const string& value);
    base_s clone  (clone_context&) const;
  };
}
