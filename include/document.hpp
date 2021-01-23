#pragma once

#include "string_ref.hpp"
#include "error.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>

namespace lini {
  using std::string;

  struct document {
    struct error : error_base { using error_base::error_base; };
    using sec_map = std::map<string, size_t>;
    using doc_map = std::map<string, sec_map>;

    doc_map map;
    std::vector<string_ref_p> values;

    void add(const string& section, const string& key, string&& value);
    std::optional<size_t> find(const string& section, const string& key) const;
    std::optional<string> get(const string& section, const string& key) const;
    string get(const string& section, const string& key, string&& fallback) const;
  };
}
