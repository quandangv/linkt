#pragma once

#include "error.hpp"
#include "tstring.hpp"

#include <string>
#include <optional>
#include <memory>
#include <cspace/processor.hpp>

namespace lini::node {
  struct base;
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

  struct plain : public base, settable {
    string val;

    explicit plain(string&& val) : val(val) {}
    string get() const { return val; }
    bool set(const string& value) { val = value; return true; }
  };

  struct defaultable : public base {
    base_p fallback;

    defaultable() {}
    explicit defaultable(base_p&& fallback) : fallback(move(fallback)) {}
    string use_fallback(const string& error_message) const;
  };

  struct ref : public defaultable, settable {
    base_pp src;

    ref(const base_pp& src, base_p&& fallback)
        : src(src), defaultable(move(fallback)) {}
    string get() const;
    bool set(const string& value);
  };

  struct meta : public defaultable {
    base_p value;
  };

  struct color : public meta {
    cspace::processor processor;

    string get() const;
  };

  struct env : public meta, settable {
    string get() const;
    bool set(const string& value);
  };

  struct cmd : public meta {
    string get() const;
  };

  struct file : public meta, settable {
    string get() const;
    bool set(const string& value);
    struct error : error_base { using error_base::error_base; };
  };

  struct map : public meta {
    float from_min, from_range, to_min, to_range;
    string get() const;
  };

  struct string_interpolate : public base {
    struct replace_spot {
      int position;
      std::string name;
      base_p replacement;
    };
    string base;
    std::vector<replace_spot> spots;

    string get() const;
  };
}
