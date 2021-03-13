#pragma once

#include "base.hpp"

namespace node {
  struct parse_context {
    wrapper_s parent, current;
    base_s* place{nullptr};
    bool parent_based_ref{false};

    wrapper_s get_current();
    wrapper_s get_parent();
    base_s& get_place();
    friend struct parse_context_base;
  };
  using parse_func = std::function<base_s(string& raw, tstring& str, parse_context&)>;
  struct parse_error : std::logic_error { using logic_error::logic_error; };
  base_s parse_raw  (string& raw, tstring& str, parse_context& context);
  base_s parse_escaped  (string& raw, tstring& str, parse_context& context);
}
