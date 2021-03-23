#include "base.hpp"

#include <optional>

namespace node {
    template<class T>
  struct cache : base<T> {
    std::shared_ptr<base<T>> source;
    std::shared_ptr<base<int>> duration_ms;
    mutable T cache_value;
    mutable steady_time cache_expire;

    explicit operator T() const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<cache<T>>
    parse(parse_context&, parse_preprocessed&);
  };

    template<class T>
  struct array_cache : base<T> {
    std::shared_ptr<base<int>> source;
    std::shared_ptr<base<T>> calculator;
    mutable std::shared_ptr<std::vector<std::optional<T>>> cache_arr;

    explicit operator T() const;
    T get  (size_t index) const;
    base_s clone  (clone_context&) const;

      static std::shared_ptr<array_cache<T>>
    parse(parse_context&, parse_preprocessed&);
  };
}

#include "cache.hxx"
