#pragma once

#include "string_ref.hpp"

namespace lini {
  struct container {
    struct error : error_base { using error_base::error_base; };
    
    virtual string_ref_p2
    get_child_ptr(tstring path) const { return {}; }

    bool
    has_child(const tstring& path) const;

    std::optional<string>
    get_child(const tstring& path) const;

    string
    get_child(const tstring& path, string&& fallback) const;

    string_ref&
    get_child_ref(const tstring& path) const;

    bool
    set(const tstring& path, const string& value);
  };

  struct addable : public container {
    virtual string_ref_p2
    add(tstring path, string_ref_p&& value, bool ignore_dup = false) = 0;

    void
    add(tstring path, string& raw, tstring value);

    void
    add(tstring path, string raw);

    string_ref_p
    parse_string(string& raw, tstring& str);
  private:
    string_ref_p
    parse_ref(string& raw, tstring& str);
  };
}
