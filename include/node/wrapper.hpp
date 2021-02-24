#pragma once

#include "container.hpp"
#include "error.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini::node {
  using std::string;

  struct wrapper : public base, addable {
    struct error : error_base { using error_base::error_base; };
    using map_type = std::map<string, base_p>;
    using addable::add;

    map_type map;
    base_p value;

    wrapper(const base_p& value) : value(value) {}
    wrapper() {}

    static wrapper&
    wrap(base_p& node);

    base_p&
    add(tstring path, const base_p& value);

    base_p
    get_child_ptr(tstring path) const;

    void
    iterate_children(std::function<void(const string&, const base_p&)> processor) const;

    string
    get() const;
  };
}
