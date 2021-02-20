#pragma once

#include "node.hpp"

#include <functional>

namespace lini {
  struct container {
    struct error : error_base { using error_base::error_base; };

    virtual string_ref_p2
    get_child_ptr(tstring path) const = 0;

    virtual void
    iterate_children(std::function<void(const string&, const base&)> processor) const = 0;

    bool
    has_child(const tstring& path) const;

    std::optional<string>
    get_child(const tstring& path) const;

    string
    get_child(const tstring& path, string&& fallback) const;

    base&
    get_child_ref(const tstring& path) const;

    bool
    set(const tstring& path, const string& value);
  };

  struct addable : public container {
    virtual string_ref_p2
    add(tstring path, string_ref_p&& value) = 0;

    string_ref_p2
    add(tstring path, string& raw, tstring value);

    string_ref_p2
    add(tstring path, string raw);

    string_ref_p
    make_local_ref(const tstring&, string_ref_p&&);
  };

  using local_ref_maker = std::function<string_ref_p(tstring&, string_ref_p&&)>;

  string_ref_p
  parse_string(string& raw, tstring& str, local_ref_maker ref_maker);

  string_ref_p
  parse_ref(string& raw, tstring& str, local_ref_maker ref_maker);
}
