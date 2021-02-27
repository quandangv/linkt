#pragma once

#include "base.hpp"

#include <cspace/processor.hpp>

namespace lini::node {
  struct color : public meta, clonable {
    cspace::processor processor;

    string get  () const;
    base_p clone  (clone_handler handler, clone_mode mode) const;
  };

  struct env : public meta, settable, clonable {
    string get  () const;
    bool set  (const string& value);
    base_p clone  (clone_handler handler, clone_mode mode) const;
  };

  struct cmd : public meta, clonable {
    string get  () const;
    base_p clone  (clone_handler handler, clone_mode mode) const;
  };

  struct file : public meta, settable, clonable {
    struct error : error_base { using error_base::error_base; };
    string get  () const;
    bool set  (const string& value);
    base_p clone  (clone_handler handler, clone_mode mode) const;
  };

  struct map : public meta, clonable {
    float from_min, from_range, to_min, to_range;

    string get  () const;
    base_p clone  (clone_handler handler, clone_mode mode) const;
  };

  struct string_interpolate : public base, clonable {
    struct replace_spot {
      int position;
      base_p replacement;
    };
    string base;
    std::vector<replace_spot> spots;

    string get  () const;
    base_p clone  (clone_handler handler, clone_mode mode) const;
  };
}
