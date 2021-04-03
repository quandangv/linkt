#pragma once

#include "base.hpp"

#include <optional>

namespace node {
    template<class T>
  struct cache : base<T> {
    std::shared_ptr<base<T>> calculator;
    std::shared_ptr<base<int>> duration_ms;
    mutable T cache_value;
    mutable steady_time cache_expire;

    explicit operator T() const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<cache<T>>
    parse(parse_context&, parse_preprocessed&);
  };

    template<class T>
  struct refcache : base<T> {
    base_s source;
    std::shared_ptr<base<T>> calculator;
    int duration_ms;
    mutable T cache_value;
    mutable string prevsrc;
    mutable steady_time cache_expire;
    mutable bool unset;

    explicit operator T() const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<refcache<T>>
    parse(parse_context&, parse_preprocessed&);
  };

    template<class T>
  struct arrcache : base<T> {
    std::shared_ptr<base<int>> source;
    std::shared_ptr<base<T>> calculator;
    mutable std::vector<std::optional<T>> cache_arr;

    explicit operator T() const;
    T get  (size_t index) const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<arrcache<T>>
    parse(parse_context&, parse_preprocessed&);
  };
}

#include "cache.hxx"
