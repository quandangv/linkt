#pragma once

#include "base.hpp"

namespace node {
  struct ancestor_destroyed_error : std::logic_error { using logic_error::logic_error; };

  struct address_ref : base<string>, settable {
    wrapper_w ancestor_w;
    std::vector<string> indirect_paths;
    string direct_path;

    address_ref  (wrapper_w ancestor, tstring path);
    string get  () const;
    bool set  (const string& value);
    base_s clone  (clone_context&) const;
    base_s get_source  () const;
    base_s* get_source_direct  () const;
    string get_path() const;
  };

  struct ref : base<string>, settable {
    base_w value;

    ref  (base_w value);
    string get  () const;
    bool set  (const string& value);
    base_s clone  (clone_context&) const;
  };
}
