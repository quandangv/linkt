#pragma once

#include "base.hpp"

namespace node {
  struct ancestor_destroyed_error : std::logic_error { using logic_error::logic_error; };

    template<class T>
  struct ref_base : base<T> {
  public:
    virtual base_s get_source() const = 0;
  };

    template<class T>
  struct address_ref : ref_base<T>, settable<T> {
    std::weak_ptr<wrapper> ancestor_w;
    std::vector<string> indirect_paths;
    string direct_path;

    address_ref(std::weak_ptr<wrapper> ancestor, tstring path);
    operator T() const;
    bool set(const T& value);
    base_s clone(clone_context&) const;
    string get_path() const;
    bool is_fixed() const;
    base_s get_source() const;
  private:
    base_s* get_source_direct() const;
  };

    template<class T>
  struct ref : ref_base<T>, settable<T> {
    std::weak_ptr<base<T>> value;

    ref(std::weak_ptr<base<T>> value);
    operator T() const;
    bool set(const T& value);
    base_s clone(clone_context&) const;
    bool is_fixed() const;
    base_s get_source() const { return value.lock(); }
  };
  
    template<class T>
  struct adapter : ref_base<T>, settable<T> {
    std::weak_ptr<base<string>> source_w;
    adapter(base_s source) : source_w(source) {}
    explicit operator T() const;
    bool set(const T& value);
    base_s clone(clone_context&) const;
    base_s get_source() const { return source_w.lock(); }
  };

}

#include "reference.hxx"
