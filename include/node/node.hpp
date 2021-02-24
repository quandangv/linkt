#pragma once

#include "base.hpp"

#include <cspace/processor.hpp>

namespace lini::node {
  struct addable;
  struct address_ref : public ref, defaultable, settable {
    addable& ancestor;
    string path;

    address_ref(addable& ancestor, string&& path, const base_p& fallback)
        : ancestor(ancestor), path(move(path)), defaultable(fallback) {}
    string get() const;
    bool set(const string& value);
    base_p get_source() const;
  };

  struct hard_ref : public ref, defaultable, settable {
    base_p src;

    hard_ref(const base_p& src, const base_p& fallback)
        : src(src), defaultable(fallback) {}
    string get() const;
    bool set(const string& value);
    base_p get_source() const { return src; }
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
