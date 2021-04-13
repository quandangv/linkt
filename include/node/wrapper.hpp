#pragma once

#include "base.hpp"
#include "tstring.hpp"

#include <map>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace node {
  struct parse_context;
  using std::string;
  struct wrapper_error : std::logic_error { using logic_error::logic_error; };

  struct wrapper : base<string>, std::enable_shared_from_this<wrapper> {
    using map_type = std::map<string, base_s>;

    map_type map{};

    explicit wrapper(const base_s& value) : map{{"", value}} {}
    wrapper() {}

    static wrapper_s wrap(base_s& node);

    base_s& add(tstring path);
    base_s& add(tstring path, const base_s& value);
    base_s& add(tstring path, parse_context& context, tstring& value);
    wrapper_s add_wrapper(const string& path);

    base_s get_child_ptr(tstring path) const;
    base_s* get_child_place(tstring path);
    string get_child(const tstring& path, string&& fallback) const;
    string get_child(const tstring& path) const;
    std::optional<string> get_child_safe(const tstring& path) const;
    wrapper_s get_wrapper(const string& path) const;

    void iterate_children(std::function<void(const string&, const base_s&)> processor) const;

    void merge(const const_wrapper_s& source, clone_context&);
    void optimize(clone_context&);
    operator string() const;
    base_s clone(clone_context&) const;
    bool is_fixed() const;

    template<class T> bool
    set(const tstring& path, const T& value) {
      auto target = std::dynamic_pointer_cast<settable<T>>(get_child_ptr(path));
      return target ? target->set(value) : false;
    }
  };
}
