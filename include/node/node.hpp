#pragma once

#include "base.hpp"
#include "fallback.hpp"

#include <chrono>
#include <cspace/processor.hpp>
#include <poll.h>

namespace node {
  using steady_time = std::chrono::time_point<std::chrono::steady_clock>;

    template<class T>
  struct base_deep : base<T> {
    const std::shared_ptr<base<T>> value;

    base_deep(std::shared_ptr<base<T>> value) : value(value) {
      if (!value) throw required_field_null_error("base_deep::base_deep");
    }

    bool is_fixed() const {
      return value->is_fixed();
    }
  };

  struct meta : base_deep<string>, with_fallback<string> {
    meta(const base_s& value, const base_s& fallback)
        : base_deep(value), with_fallback(fallback) {}

      template<class T> std::shared_ptr<T>
    copy(clone_context& context) const {
        return std::make_shared<T>(checked_clone<string>(value, context, "meta::copy")
            , fallback ? fallback->clone(context) : base_s());
    }
  };

  struct color : public meta {
    using meta::meta;
    cspace::processor processor;

    explicit operator string() const;
    base_s clone(clone_context&) const;
      static std::shared_ptr<color>
    parse(parse_context&, parse_preprocessed&);
  };

  struct env : public meta, settable<string> {
    using meta::meta;
    explicit operator string() const;
    bool set  (const string& value);
    base_s clone(clone_context&) const;
    bool is_fixed() const { return false; }
  };

  struct cmd : meta {
    using meta::meta;
    explicit operator string() const;
    base_s clone(clone_context&) const;
    bool is_fixed() const { return false; }
  };

  struct poll : base<string>, with_fallback<string>, settable<string> {
    base_s cmd;
    mutable pollfd pfd{0, POLLIN, 0};

    poll(const base_s& cmd, const base_s& fallback);
    ~poll();
    explicit operator string() const;
    base_s clone(clone_context&) const;
    void start_cmd() const;
    bool is_fixed() const { return false; }
    bool set(const string& value);
  };

  struct file : public meta, settable<string> {
    using meta::meta;
    explicit operator string() const;
    bool set(const string& value);
    base_s clone(clone_context&) const;
    bool is_fixed() const { return false; }
  };

  struct save : base<string> {
    base_s value;
    base_s target;
    char delimiter{'\n'};

    explicit operator string() const;
    base_s clone(clone_context&) const;

      static std::shared_ptr<save>
    parse(parse_context&, parse_preprocessed&);
  };

  struct map : base_deep<float> {
    float from_min{0}, from_range{0}, to_min{0}, to_range{0};

    using base_deep<float>::base_deep;
    explicit operator float() const;
    base_s clone(clone_context&) const;

      static std::shared_ptr<map>
    parse(parse_context&, parse_preprocessed&);
  };

  struct smooth : base_deep<float> {
    float spring, drag;
    mutable float current{0}, velocity{0};

    using base_deep<float>::base_deep;
    explicit operator float() const;
    base_s clone(clone_context&) const;

      static std::shared_ptr<smooth>
    parse(parse_context&, parse_preprocessed&);
  };

  struct clock : base<int> {
    std::chrono::milliseconds tick_duration;
    unsigned int loop;
    mutable steady_time zero_point;

    explicit operator int() const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<clock>
    parse(parse_context&, parse_preprocessed&);
  };
}
