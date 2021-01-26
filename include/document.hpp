#pragma once

#include "string_ref.hpp"
#include "error.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini {
  using std::string;

  class document : public string_ref, public addable {
    string_ref_p
    parse_ref(string& raw, tstring& str);
  public:
    struct error : error_base { using error_base::error_base; };
    using map_type = std::map<string, string_ref_p2>;

    map_type map;


    string_ref_p2
    add(tstring path, string_ref_p&& value, bool duplicate = false);

    void
    add(tstring path, string& raw, tstring value);

    void
    add(tstring path, string raw);

    string_ref_p2
    get_child_ptr(tstring path) const;

    string get() const { return ""; }

    string_ref_p
    parse_string(string& raw, tstring& str);
  };
}
