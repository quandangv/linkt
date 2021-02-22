#pragma once

#include "node.hpp"

#include <functional>

namespace lini::node {
  struct container {
    struct error : error_base { using error_base::error_base; };

    virtual base_pp
    get_child_ptr(tstring path) const = 0;

    virtual void
    iterate_children(std::function<void(const string&, const base_p&)> processor) const = 0;

    virtual void
    iterate_children(std::function<void(const string&, const base&)> processor) const;

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
    virtual base_pp
    add(tstring path, base_p&& value) = 0;

    base_pp
    add(tstring path, string& raw, tstring value);

    base_pp
    add(tstring path, string raw);

    base_p
    make_ref(const tstring&, base_p&&);

    base_p
    make_address_ref(const tstring&, base_p&&);
  };

  using ref_maker = std::function<base_p(tstring&, base_p&&)>;

  base_p
  parse_string(string& raw, tstring& str, ref_maker ref_maker);

  base_p
  parse(string& raw, tstring& str, ref_maker ref_maker);
}
