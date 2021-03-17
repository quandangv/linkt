#pragma once

#include "base.hpp"

namespace node {
    template<class T>
  struct with_fallback {
    std::shared_ptr<base<T>> fallback;

    with_fallback() {}

    explicit with_fallback(const std::shared_ptr<base<T>>& fallback) : fallback(fallback) {}

      [[nodiscard]] inline string
    use_fallback  (const string& message) const {
        if (!fallback) throw node_error("Failure: " + message + ". No fallback was found");
        return fallback->get();
    }
  };

    template<class T>
  struct fallback_wrapper : base<T>, settable<T>, protected with_fallback<T> {
    const std::shared_ptr<base<T>> source;

    fallback_wrapper(std::shared_ptr<base<T>> source, std::shared_ptr<base<T>> fallback)
    : with_fallback<T>(fallback), source(source) {
        if (!fallback || !source)
          throw required_field_null_error("fallback_wrapper::fallback_wrapper");
    }

    operator T() const {
        try {
          return source->operator T();
        } catch (const std::exception& e) {
          return with_fallback<T>::fallback->operator T();
        }
    }

    bool set(const T& value) {
        auto target = std::dynamic_pointer_cast<settable<T>>(source);
        if (target) {
          if (target->set(value))
            return true;
        }
        if (target = std::dynamic_pointer_cast<settable<T>>(with_fallback<T>::fallback))
          return target->set(value);
        return false;
    }

    base_s clone(clone_context& context) const {
        auto src_clone = checked_clone<T>(source, context, "fallback_wrapper::clone");
        auto fb_clone = checked_clone<T>(with_fallback<T>::fallback, context
            , "fallback_wrapper::clone");
        return std::make_shared<fallback_wrapper>(src_clone, fb_clone);
    }

    bool is_fixed() const {
        try {
          return source->is_fixed();
        } catch (const std::exception& e) {
          return false;
        }
    }
  };
}
