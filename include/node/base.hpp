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
  using base_p = std::shared_ptr<base>;
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
    std::vector<std::pair<const wrapper*, wrapper*>> ancestors;
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
    virtual base_p clone  (clone_context&) const = 0;
    base_p checked_clone  (clone_context&) const;
  };

  struct settable {
    virtual bool set  (const string&) = 0;
  };

  struct plain : base, settable {
    string val;

    explicit plain  (string&& val) : val(val) {}
    string get  () const { return val; }
    bool set  (const string& value) { val = value; return true; }
    base_p clone  (clone_context&) const { return std::make_shared<plain>(string(val)); }
  };

  struct defaultable {
    base_p fallback;

    defaultable  () {}
    explicit defaultable  (const base_p& fallback) : fallback(fallback) {}
    [[nodiscard]] string use_fallback  (const string& error_message) const;
  };

  struct wrapper;
  struct address_ref : base, defaultable, settable {
    wrapper& ancestor;
    string path;

    address_ref  (wrapper& ancestor, string&& path, const base_p& fallback)
        : defaultable(fallback), ancestor(ancestor), path(move(path)) {}
    string get  () const { return get_source()->get(); }
    bool set  (const string& value);
    base_p get_source  () const;
    base_p clone  (clone_context&) const;
  };

  struct meta : base, defaultable {
    const base_p value;

    meta(const base_p& value);

    template <typename T>
    std::shared_ptr<T> copy(clone_context& context) const {
      auto result = std::make_shared<T>(value->checked_clone(context));
      if (fallback)
        result->fallback = fallback->clone(context);
      return result;
    }
  };

  bool is_fixed(base_p node);

  struct parse_context {
    wrapper* parent{nullptr}, *current{nullptr};
    base_p* place{nullptr};
    bool parent_based_ref{false};

    wrapper& get_current();
    wrapper& get_parent();
    base_p& get_place();
    friend struct parse_context_base;
  };
  using parse_func = std::function<base_p(string& raw, tstring& str, parse_context&)>;
  struct parse_error : std::logic_error { using logic_error::logic_error; };
  base_p parse_raw  (string& raw, tstring& str, parse_context& context);
  base_p parse_escaped  (string& raw, tstring& str, parse_context& context);
}
