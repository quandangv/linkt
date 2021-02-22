#pragma once

#include "error.hpp"
#include "tstring.hpp"

#include <string>
#include <optional>
#include <memory>
#include <functional>
#include <cspace/processor.hpp>

namespace lini::node {
  struct base;
  struct addable;
  using std::string;
  using base_p = std::unique_ptr<base>;
  using base_pp = std::shared_ptr<base_p>;

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
    explicit defaultable(base_p&& fallback) : fallback(move(fallback)) {}
    string use_fallback(const string& error_message) const;
  };

  struct address_ref : public base, defaultable, settable {
    addable& ancestor;
    string path;

    address_ref(addable& ancestor, string&& path, base_p&& fallback)
        : ancestor(ancestor), path(move(path)), defaultable(move(fallback)) {}
    string get() const;
    bool set(const string& value);
  };

  struct ref : public base, defaultable, settable {
    base_pp src;

    ref(const base_pp& src, base_p&& fallback)
        : src(src), defaultable(move(fallback)) {}
    string get() const;
    bool set(const string& value);
  };

  struct meta : public base, defaultable {
    base_p value;
    
    base_p copy(std::unique_ptr<meta>&& dest, clone_handler handler) const;
  };

  struct color : public meta, clonable {
    cspace::processor processor;

    string get() const;
    base_p clone(clone_handler handler) const;
  };

  struct env : public meta, settable, clonable {
    string get() const;
    bool set(const string& value);
    base_p clone(clone_handler handler) const;
  };

  struct cmd : public meta, clonable {
    string get() const;
    base_p clone(clone_handler handler) const;
  };

  struct file : public meta, settable, clonable {
    struct error : error_base { using error_base::error_base; };
    string get() const;
    bool set(const string& value);
    base_p clone(clone_handler handler) const;
  };

  struct map : public meta, clonable {
    float from_min, from_range, to_min, to_range;

    string get() const;
    base_p clone(clone_handler handler) const;
  };

  struct string_interpolate : public base, clonable {
    struct replace_spot {
      int position;
      std::string name;
      base_p replacement;
    };
    string base;
    std::vector<replace_spot> spots;

    string get() const;
    base_p clone(clone_handler handler) const;
  };
}
