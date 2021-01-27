#pragma once

#include "error.hpp"
#include "tstring.hpp"

#include <string>
#include <optional>
#include <memory>
#include <cspace/processor.hpp>

namespace lini {
  struct document;
  struct string_ref;
  using std::string;
  using string_ref_p = std::unique_ptr<string_ref>;
  using string_ref_p2 = std::shared_ptr<string_ref_p>;

  struct string_ref {
    struct error : error_base { using error_base::error_base; };

    virtual string
    get() const = 0;

    virtual ~string_ref() {}
  };

  struct settable {
    virtual bool
    readonly() const { return true; }

    virtual void
    set(const string&) {}
  };

  struct const_ref : public string_ref, settable {
    string val;

    const_ref(string&& val) : val(val) {}
    string get() const { return val; }
    bool readonly() const { return false; }
    void set(const string& value) { val = value; }
  };

  struct fallback_ref : public string_ref {
    string_ref_p fallback;

    fallback_ref() {}
    fallback_ref(string_ref_p&& fallback) : fallback(move(fallback)) {}
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

  struct string_interpolate_ref : public string_ref {
    struct replacement_list : public std::iterator<std::forward_iterator_tag, string> {
      struct iterator;
      std::vector<string_ref_p> list;

      iterator begin() const;
      iterator end() const;
    } replacements;
    string base;
    std::vector<size_t> positions;

    string get() const;
  };
}
