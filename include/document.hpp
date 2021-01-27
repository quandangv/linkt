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

  class document : public string_ref, public addable {
  public:
    struct error : error_base { using error_base::error_base; };
    using map_type = std::map<string, string_ref_p2>;
    using addable::add;

    map_type map;


    string_ref_p2
    add(tstring path, string_ref_p&& value, bool ignore_dup = false);

    string_ref_p2
    get_child_ptr(tstring path) const;

    string get() const { return ""; }
  };
}
