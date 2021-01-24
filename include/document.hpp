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
    string_ref_p2
    get_ptr(const string& section, const string& key) const;

  public:
    struct error : error_base { using error_base::error_base; };
    using sec_map = std::map<string, string_ref_p2>;
    using doc_map = std::map<string, sec_map>;

    doc_map map;

    void
    add(const string& section, const string& key, string&& value);

    string_ref_p2
    add(const string& section, const string& key);

    std::optional<string>
    get(const string& section, const string& key) const;

    string
    get(const string& section, const string& key, string&& fallback) const;

    static void
    optimize(string_ref_p2 value);

    bool
    set(const string& section, const string& key, const string& value);
  };
}
