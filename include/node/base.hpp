#pragma once

#include "error.hpp"
#include "tstring.hpp"

#include <string>
#include <memory>
#include <functional>

namespace lini::node {
  struct base;
  using std::string;
  using base_p = std::shared_ptr<base>;

  struct base {
    struct error : error_base { using error_base::error_base; };
    virtual ~base() {}

    virtual string get  () const = 0;
  };

  struct settable {
    virtual bool set  (const string&) { return false; }
  };

  enum class clone_mode {
    exact = 0,
    optimize = 2,
    no_dependency = 1,
  };
  using clone_handler = std::function<base_p(const base&)>;
  struct clonable {
    virtual base_p clone  (clone_handler handler, clone_mode mode) const = 0;
  };

  struct plain : public base, settable, clonable {
    string val;

    explicit plain  (string&& val) : val(val) {}
    string get  () const { return val; }
    bool set  (const string& value) { val = value; return true; }
    base_p clone  (clone_handler, clone_mode) const { return std::make_shared<plain>(string(val)); }
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
    void invalidate  () { path = ""; }
  };

  struct meta : public base, defaultable {
    base_p value;
    
    base_p copy(std::shared_ptr<meta>&& dest, clone_handler handler, clone_mode mode) const;
  };

  inline clone_mode operator&(clone_mode a, clone_mode b)
  { return (clone_mode)((int)a & (int)b); }

  inline clone_mode operator|(clone_mode a, clone_mode b)
  { return (clone_mode)((int)a | (int)b); }

  base_p clone  (const base& source, clone_mode mode = clone_mode::exact);
  base_p clone  (const base_p& source, clone_mode mode = clone_mode::exact);
  base_p clone  (const base_p& source, clone_handler handler, clone_mode mode);
  base_p clone  (const base& source, clone_handler handler, clone_mode mode);
  bool is_fixed(base_p node);

  using ref_maker = std::function<std::shared_ptr<address_ref>(tstring& path, const base_p& fallback)>;

  struct parse_error : error_base { using error_base::error_base; };
  base_p parse_string  (string& raw, tstring& str, ref_maker ref_maker);
  base_p parse  (string& raw, tstring& str, ref_maker ref_maker);
}
