#pragma once

#include "base.hpp"
#include "fallback.hpp"

#include <chrono>
#include <cspace/processor.hpp>

namespace node {
  using steady_time = std::chrono::time_point<std::chrono::steady_clock>;

  struct meta : base<string>, with_fallback<string> {
    const base_s value;

    meta(const base_s& value, const base_s& fallback);

    template<class T> std::shared_ptr<T>
    copy(clone_context& context) const {
      return std::make_shared<T>(checked_clone<string>(value, context, "meta::copy"), fallback ? fallback->clone(context) : base_s());
    }
  };

  struct color : public meta {
    using meta::meta;
    cspace::processor processor;

    explicit operator string() const;
    base_s clone(clone_context&) const;
    bool is_fixed() const;
      static std::shared_ptr<color>
    parse(parse_context&, parse_preprocessed&);
  };

  struct env : public meta, settable<string> {
    using meta::meta;
    explicit operator string() const;
    bool set  (const string& value);
    base_s clone(clone_context&) const;
  };

  struct cmd : meta {
    using meta::meta;
    explicit operator string() const;
    base_s clone(clone_context&) const;
  };

  struct file : public meta, settable<string> {
    using meta::meta;
    explicit operator string() const;
    bool set(const string& value);
    base_s clone(clone_context&) const;
  };

  struct save : base<string> {
    base_s value;
    base_s target;

    explicit operator string() const;
    base_s clone(clone_context&) const;

      static std::shared_ptr<save>
    parse(parse_context&, parse_preprocessed&);
  };

  struct cache : base<string> {
    base_s source;
    std::shared_ptr<base<int>> duration_ms;
    mutable string cache_str;
    mutable steady_time cache_expire;

    explicit operator string() const;
    base_s clone  (clone_context&) const;
      static std::shared_ptr<cache>
    parse(parse_context&, parse_preprocessed&);
  };

  struct array_cache : base<string> {
    std::shared_ptr<base<int>> source;
    base_s calculator;
    mutable std::shared_ptr<std::vector<string>> cache_arr;

    explicit operator string() const;
    string get  (size_t index) const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<array_cache>
    parse(parse_context&, parse_preprocessed&);
  };

  struct map : base<float> {
    const std::shared_ptr<base<float>> value;
    float from_min{0}, from_range{0}, to_min{0}, to_range{0};

    map(std::shared_ptr<base<float>> value);
    explicit operator float() const;
    base_s clone(clone_context&) const;
    bool is_fixed() const;

      static std::shared_ptr<map>
    parse(parse_context&, parse_preprocessed&);
  };

  //struct smooth : base<float> {
  //  const base_s v

  struct clock : base<int> {
    std::chrono::milliseconds tick_duration;
    unsigned int loop;
    mutable steady_time zero_point;

    explicit operator int() const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<clock>
    parse(parse_context&, parse_preprocessed&);
  };

  struct string_interpolate : base<string> {
    struct replace_spot {
      int position;
      base_s replacement;
      replace_spot(int pos, base_s repl) : position(pos), replacement(repl) {}
    };
    string base;
    std::vector<replace_spot> spots;

    explicit operator string() const;
    base_s clone  (clone_context&) const;
    bool is_fixed() const;
  };
}
