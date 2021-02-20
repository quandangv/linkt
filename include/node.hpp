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
  using string_ref_p = std::unique_ptr<base>;
  using string_ref_p2 = std::shared_ptr<string_ref_p>;

  struct base {
    struct error : error_base { using error_base::error_base; };

    virtual string
    get() const = 0;

    virtual ~base() {}
  };

  struct settable {
    virtual bool
    readonly() const { return true; }

    virtual void
    set(const string&) {}
  };

  struct const_ref : public base, settable {
    string val;

    explicit const_ref(string&& val) : val(val) {}
    string get() const { return val; }
    bool readonly() const { return false; }
    void set(const string& value) { val = value; }
  };

  struct fallback_ref : public base {
    string_ref_p fallback;

    fallback_ref() {}
    explicit fallback_ref(string_ref_p&& fallback) : fallback(move(fallback)) {}
    string use_fallback(const string& error_message) const;
  };

  struct local_ref : public fallback_ref, settable {
    string_ref_p2 ref;

    local_ref(const string_ref_p2& ref, string_ref_p&& fallback)
        : ref(ref), fallback_ref(move(fallback)) {}
    string get() const;
    bool readonly() const;
    void set(const string& value);
  };

  struct meta_ref : public fallback_ref {
    string_ref_p value;
  };

  struct color_ref : public meta_ref {
    cspace::processor processor;

    string get() const;
  };

  struct env_ref : public meta_ref, settable {
    string get() const;
    bool readonly() const { return false; }
    void set(const string& value);
  };

  struct cmd_ref : public meta_ref {
    string get() const;
  };

  struct file_ref : public meta_ref, settable {
    string get() const;
    bool readonly() const { return false; }
    void set(const string& value);
    struct error : error_base { using error_base::error_base; };
  };

  struct map_ref : public meta_ref {
    float from_min, from_range, to_min, to_range;
    string get() const;
  };

  struct string_interpolate_ref : public base {
    struct replace_spot {
      int position;
      std::string name;
      string_ref_p replacement;
    };
    string base;
    std::vector<replace_spot> spots;

    string get() const;
  };
}
