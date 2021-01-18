#pragma once

#include "string_ref.hpp"
#include "error.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini {
  using std::string;
  using errorlist = std::vector<std::pair<int, string>>;
  using str_errlist = std::vector<std::pair<string, string>>;

  struct document {
    struct error : error_base { using error_base::error_base; };
    using sec_map = std::map<string, size_t>;
    using doc_map = std::map<string, sec_map>;

    doc_map map;
    std::vector<string_ref_p> values;

    bool add_onetime(const string& section, const string& key, string&& value);
    bool has(const string& section, const string& key) const;
    std::optional<size_t> find(const string& section, const string& key) const;
    opt_str get(const string& section, const string& key) const;
    string_ref& get_ref(const string& section, const string& key) const;
    string get(const string& section, const string& key, string&& fallback) const;
    string to_string() const;
  };
}
