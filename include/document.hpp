#pragma once

#include "container.hpp"
#include "error.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini {
  using std::string;

  struct document : public string_ref, public addable {
    struct error : error_base { using error_base::error_base; };
    using map_type = std::map<string, string_ref_p2>;
    using addable::add;

    map_type map;
    string_ref_p value;

    string_ref_p2
    add(tstring path, string_ref_p&& value);

    string_ref_p2
    get_child_ptr(tstring path) const;

    void
    iterate_children(std::function<void(const string&, const string_ref&)> processor) const;

    string
    get() const;
  };
}
