#pragma once

#include "base.hpp"

#include <chrono>
#include <cspace/processor.hpp>

namespace node {
  using steady_time = std::chrono::time_point<std::chrono::steady_clock>;

  struct color : public meta {
    using meta::meta;
    cspace::processor processor;

    string get  () const;
    base_s clone  (clone_context&) const;
  };

  struct env : public meta, settable {
    using meta::meta;
    string get  () const;
    bool set  (const string& value);
    base_s clone  (clone_context&) const;
  };

  struct cmd : public meta {
    using meta::meta;
    string get  () const;
    base_s clone  (clone_context&) const;
  };

  struct file : public meta, settable {
    using meta::meta;
    string get  () const;
    bool set  (const string& value);
    base_s clone  (clone_context&) const;
  };

  struct save : base {
    base_s value;
    base_s target;

    string get  () const;
    base_s clone  (clone_context&) const;
  };

  struct cache : base {
    base_s source;
    base_s duration_ms;
    mutable string cache_str;
    mutable steady_time cache_expire;

    string get  () const;
    base_s clone  (clone_context&) const;
  };

  struct array_cache : base {
    base_s source, calculator;
    mutable std::shared_ptr<std::vector<string>> cache_arr;

    string get  () const;
    string get  (size_t index) const;
    base_s clone  (clone_context&) const;
  };

  struct map : meta {
    using meta::meta;
    float from_min{0}, from_range{0}, to_min{0}, to_range{0};

    string get  () const;
    base_s clone  (clone_context&) const;
  };

  struct clock : base {
    std::chrono::milliseconds tick_duration;
    size_t loop;
    mutable steady_time zero_point;

    string get  () const;
    base_s clone  (clone_context&) const;
  };

  struct string_interpolate : public base {
    struct replace_spot {
      int position{0};;
      base_s replacement;
    };
    string base;
    std::vector<replace_spot> spots;

    string get  () const;
    base_s clone  (clone_context&) const;
  };
}
