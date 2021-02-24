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

    virtual string
    get() const = 0;

    virtual ~base() {}
  };

  struct settable {
    virtual bool
    set(const string&) { return false; }
  };

  using clone_handler = std::function<base_p(const base&)>;
  struct clonable {
    virtual base_p
    clone(clone_handler handler) const = 0;
  };
  base_p clone(const base& source);
  base_p clone(const base_p& source);
  base_p clone(const base_p& source, clone_handler handler);
  base_p clone(const base& source, clone_handler handler);

  struct plain : public base, settable, clonable {
    string val;

    explicit plain(string&& val) : val(val) {}
    string get() const { return val; }
    bool set(const string& value) { val = value; return true; }
    base_p clone(clone_handler handler) const { return std::make_unique<plain>(string(val)); }
  };

  struct defaultable {
    base_p fallback;

    defaultable() {}
    explicit defaultable(const base_p& fallback) : fallback(fallback) {}
    string use_fallback(const string& error_message) const;
  };

  struct meta : public base, defaultable {
    base_p value;
    
    base_p copy(std::unique_ptr<meta>&& dest, clone_handler handler) const;
  };

  using ref_maker = std::function<base_p(tstring& path, const base_p& fallback)>;

  base_p
  parse_string(string& raw, tstring& str, ref_maker ref_maker);

  base_p
  parse(string& raw, tstring& str, ref_maker ref_maker);
}
