#pragma once

#include "base.hpp"

namespace node {

  struct parse_context {
    bool parent_based_ref{false};

    string raw, current_path;
    wrapper_s parent, current, root;
    base_s* place{nullptr};

    wrapper_s get_current();
    wrapper_s get_parent();
    base_s& get_place();
    friend struct parse_context_base;
  };

  class parse_preprocessed {
    base_s fallback;
  public:
    std::array<tstring, 7> tokens;
    unsigned char token_count{0};
    tstring process(tstring&);
    void set_fallback(base_s fb) { fallback = fb; }
    bool has_fallback() { return fallback.get(); }
    base_s pop_fallback() { auto result = std::move(fallback); return result; }
  };

    template<class T> std::shared_ptr<base<T>>
  parse_raw(parse_context& context, tstring& value);

    template<class T> std::shared_ptr<base<T>>
  parse_escaped(parse_context& context, tstring& value);

    template<class T> std::shared_ptr<base<T>>
  checked_parse_raw(parse_context& context, tstring& value);
}
