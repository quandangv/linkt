#pragma once

#include "node.hpp"
#include "tstring.hpp"

#include <functional>

namespace lini::node {
  struct container {
    struct error : error_base { using error_base::error_base; };

    virtual base_p
    get_child_ptr(tstring path) const = 0;

    virtual void
    iterate_children(std::function<void(const string&, const base_p&)> processor) const = 0;

    virtual void
    iterate_children(std::function<void(const string&, const base&)> processor) const;

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
    virtual base_p&
    add(tstring path, const base_p& value) = 0;

    base_p
    add(tstring path, string& raw, tstring value);

    base_p
    add(tstring path, string raw);

    base_p
    make_ref(const tstring&, const base_p&);

    base_p
    make_address_ref(const tstring&, const base_p&);
  };
}