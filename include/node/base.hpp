#pragma once

#include "tstring.hpp"

#include <string>
#include <memory>
#include <functional>

namespace linked_nodes::node {
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
  struct base {
    virtual ~base() {}

    virtual string get  () const = 0;
    virtual base_p clone  (clone_context& context) const = 0;
    base_p clone() const;
  };

  struct settable {
    virtual bool set  (const string&) = 0;
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
    string get  () const { return get_source()->get(); }
    bool set  (const string& value);
    base_p get_source  () const;
    base_p clone  (clone_context&) const;
  };

  struct meta : public base, defaultable {
    base_p value;
    
    base_p copy(std::shared_ptr<meta>&& dest, clone_context& context) const;
  };

  bool is_fixed(base_p node);

  using ref_maker = std::function<std::shared_ptr<address_ref>(tstring& path, const base_p& fallback)>;

  struct parse_error : std::logic_error { using logic_error::logic_error; };
  base_p parse_string  (string& raw, tstring& str, ref_maker ref_maker);
  base_p parse  (string& raw, tstring& str, ref_maker ref_maker);
}
