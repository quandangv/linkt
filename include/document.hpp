#pragma once

#include "string_ref.hpp"
#include "error.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini {
  using std::string;

  class document {
    std::vector<string_ref_p2> values;
  public:
    struct error : error_base { using error_base::error_base; };
    using sec_map = std::map<string, size_t>;
    using doc_map = std::map<string, sec_map>;

    doc_map map;

    void
    add(const string& section, const string& key, string&& value);

    string_ref_p2
    add(const string& section, const string& key);

    std::optional<size_t>
    find(const string& section, const string& key) const;

    string_ref_p&
    get_ptr(size_t index) const;

    std::optional<string>
    get(const string& section, const string& key) const;

    string
    get(const string& section, const string& key, string&& fallback) const;
  };
}
