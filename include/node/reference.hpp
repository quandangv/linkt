#pragma once

#include "base.hpp"

namespace node {
  struct ancestor_destroyed_error : std::logic_error { using logic_error::logic_error; };

  template<class T>
  struct address_ref : base<T>, settable<T> {
    std::weak_ptr<wrapper> ancestor_w;
    std::vector<string> indirect_paths;
    string direct_path;

    address_ref  (std::weak_ptr<wrapper> ancestor, tstring path);
    operator T() const;
    bool set  (const T& value);
    base_s clone  (clone_context&) const;
    std::shared_ptr<base<T>> get_source  () const;
    base_s* get_source_direct  () const;
    string get_path() const;
    bool is_fixed() const;
  };
  template struct address_ref<string>;

  template<class T>
  struct ref : base<T>, settable<T> {
    std::weak_ptr<base<T>> value;

    ref(std::weak_ptr<base<T>> value);
    operator T() const;
    bool set  (const T& value);
    base_s clone  (clone_context&) const;
    bool is_fixed() const;
  };
}

#include "reference.hxx"
