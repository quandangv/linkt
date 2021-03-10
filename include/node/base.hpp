#pragma once

#include "tstring.hpp"

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace node {
  struct base;
  struct wrapper;
  using std::string;
  using base_s = std::shared_ptr<base>;
  using wrapper_s = std::shared_ptr<wrapper>;
  using const_wrapper_s = std::shared_ptr<const wrapper>;
  using wrapper_w = std::weak_ptr<wrapper>;

  struct node_error : std::logic_error { using logic_error::logic_error; };

  struct errorlist : std::vector<std::pair<std::string, std::string>> {
    void report_error  (int line, const std::string& msg)
    { emplace_back("line " +std::to_string(line), msg); }

    void report_error  (int line, const std::string& key, const std::string& msg)
    { emplace_back("line " +std::to_string(line) + ", key " + key, msg); }

    void report_error  (const std::string& key, const std::string& msg)
    { emplace_back(key, msg); }

    bool extract_key  (tstring& line, int linecount, char separator, tstring& key);
  };

  struct clone_context {
    std::string current_path;
    std::vector<std::pair<const_wrapper_s, wrapper_s>> ancestors;
    bool optimize{false}, no_dependency{false};
    errorlist errors;

    void report_error(const string& msg)
    { errors.report_error(current_path, msg); }
  };

  struct throwing_clone_context : public clone_context {
    ~throwing_clone_context() noexcept(false);
  };

  struct base {
    virtual ~base() {}

    virtual string get  () const = 0;
    virtual base_s clone  (clone_context&) const = 0;
    base_s checked_clone  (clone_context&) const;
  };

  struct settable {
    virtual bool set  (const string&) = 0;
  };

  struct plain : base, settable {
    string val;

    explicit plain  (string&& val) : val(val) {}
    string get  () const { return val; }
    bool set  (const string& value) { val = value; return true; }
    base_s clone  (clone_context&) const { return std::make_shared<plain>(string(val)); }
  };

  struct defaultable {
    base_s fallback;

    defaultable  () {}
    explicit defaultable  (const base_s& fallback) : fallback(fallback) {}
    [[nodiscard]] string use_fallback  (const string& error_message) const;
  };

  struct wrapper;
  struct ancestor_destroyed_error : std::logic_error { using logic_error::logic_error; };

  struct address_ref : base, defaultable, settable {
    wrapper_w ancestor_w;
    string path;

    address_ref  (wrapper_w ancestor, string&& path, const base_s& fallback)
        : defaultable(fallback), ancestor_w(ancestor), path(move(path)) {}
    string get  () const { return get_source()->get(); }
    bool set  (const string& value);
    base_s get_source  () const;
    base_s clone  (clone_context&) const;
  };

  struct meta : base, defaultable {
    const base_s value;

    meta(const base_s& value);

    template <typename T>
    std::shared_ptr<T> copy(clone_context& context) const {
      auto result = std::make_shared<T>(value->checked_clone(context));
      if (fallback)
        result->fallback = fallback->clone(context);
      return result;
    }
  };

  bool is_fixed(base_s node);

  struct parse_context {
    wrapper_s parent, current;
    base_s* place{nullptr};
    bool parent_based_ref{false};

    wrapper_s get_current();
    wrapper_s get_parent();
    base_s& get_place();
    friend struct parse_context_base;
  };
  using parse_func = std::function<base_s(string& raw, tstring& str, parse_context&)>;
  struct parse_error : std::logic_error { using logic_error::logic_error; };
  base_s parse_raw  (string& raw, tstring& str, parse_context& context);
  base_s parse_escaped  (string& raw, tstring& str, parse_context& context);
}
