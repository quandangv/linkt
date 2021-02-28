#pragma once

#include "error.hpp"
#include "tstring.hpp"

#include <string>
#include <memory>
#include <functional>

namespace lini::node {
  struct base;
  struct wrapper;
  using std::string;
  using base_p = std::shared_ptr<base>;

  struct errorlist : std::vector<std::pair<std::string, std::string>> {
    void report_error  (int line, const std::string& msg);
    void report_error  (int line, const std::string& key, const std::string& msg);
    void report_error  (const std::string& key, const std::string& msg);
    bool extract_key  (tstring& line, int linecount, char separator, tstring& key);
    string merge_errors() const;
  };
  struct clone_error : error_base { using error_base::error_base; };
  struct clone_context {
    std::string current_path;
    std::vector<std::pair<const wrapper*, wrapper*>> ancestors;
    bool optimize{false}, no_dependency{false};
    errorlist errors;

    void report_error(const string& msg);
  };
  struct base {
    struct error : error_base { using error_base::error_base; };
    virtual ~base() {}

    virtual string get  () const = 0;
    virtual base_p clone  (clone_context& context) const = 0;
    base_p clone() const;
  };

  struct settable {
    virtual bool set  (const string&) { return false; }
  };

  struct plain : public base, settable {
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
    string use_fallback  (const string& error_message) const;
  };

  struct wrapper;
  struct address_ref : public base, defaultable, settable {
    wrapper& ancestor;
    string path;

    address_ref  (wrapper& ancestor, string&& path, const base_p& fallback)
        : ancestor(ancestor), path(move(path)), defaultable(fallback) {}
    string get  () const;
    bool set  (const string& value);
    base_p get_source  () const;
    base_p clone  (clone_context&) const;
    void invalidate  () { path = ""; }
  };

  struct meta : public base, defaultable {
    base_p value;
    
    base_p copy(std::shared_ptr<meta>&& dest, clone_context& context) const;
  };

  bool is_fixed(base_p node);

  using ref_maker = std::function<std::shared_ptr<address_ref>(tstring& path, const base_p& fallback)>;

  struct parse_error : error_base { using error_base::error_base; };
  base_p parse_string  (string& raw, tstring& str, ref_maker ref_maker);
  base_p parse  (string& raw, tstring& str, ref_maker ref_maker);
}
