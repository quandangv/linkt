#pragma once

#include "base.hpp"

#include <cspace/processor.hpp>

namespace lini::node {
  struct color : public meta {
    cspace::processor processor;

    string get  () const;
    base_p clone  (clone_context&) const;
  };

  struct env : public meta, settable {
    string get  () const;
    bool set  (const string& value);
    base_p clone  (clone_context&) const;
  };

  struct cmd : public meta {
    string get  () const;
    base_p clone  (clone_context&) const;
  };

  struct file : public meta, settable {
    string get  () const;
    bool set  (const string& value);
    base_p clone  (clone_context&) const;
  };

  struct map : public meta {
    float from_min, from_range, to_min, to_range;

    string get  () const;
    base_p clone  (clone_context&) const;
  };

  struct string_interpolate : public base {
    struct replace_spot {
      int position;
      base_p replacement;
    };
    string base;
    std::vector<replace_spot> spots;

    string get  () const;
    base_p clone  (clone_context&) const;
  };
}
