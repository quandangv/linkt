#pragma once

#include "structs.hpp"

#include <string>
#include <memory>
#include <functional>
#include <cmath>

namespace node {
  struct node_error : std::logic_error { using logic_error::logic_error; };
  struct required_field_null_error : std::logic_error { using logic_error::logic_error; };
  struct clone_error : std::logic_error { using logic_error::logic_error; };
  struct parse_error : std::logic_error { using logic_error::logic_error; };

    template<>
  struct base<string> {
    virtual ~base() {}

      virtual explicit
    operator string() const = 0;

      virtual base_s
    clone(clone_context&) const = 0;

      virtual bool
    is_fixed() const {
      return false;
    }

    string get() const {
      return operator string();
    }
  };

    template<class T> std::shared_ptr<base<T>>
  checked_clone(base_s source, clone_context& context, const string& msg) {
      auto result = source->clone(context);
      auto converted = std::dynamic_pointer_cast<base<T>>(result
          ?: throw clone_error("clone_error: Empty clone result in: " + msg));
      return converted
          ?: throw clone_error("clone_error: Clone result in " + msg + " have invalid type: "
          + typeid(result.get()).name());
  }

    template<class T>
  struct base : base<string> {
    virtual explicit operator T() const = 0;
  };

    template<class T>
  struct settable {
    virtual bool set(const T&) = 0;
  };

    template<>
  struct base<int> : virtual base<string> {
    virtual explicit operator int() const = 0;

    explicit operator string() const {
      return std::to_string(operator int());
    }
  };

    template<>
  struct base<float> : base<int> {
      virtual explicit
    operator float() const = 0;

      virtual explicit
    operator int() const {
      return std::lround(operator float());
    }

    explicit operator string() const {
      std::string str = std::to_string (operator float());
      auto erase = str.find_last_not_of('0');
      return str.erase (str[erase] == '.' ? erase : (erase + 1), std::string::npos );
    }
  };

    template<class T>
  struct plain : base<T> {
    T value;
    plain(T&& value) : value(value) {}

    explicit operator T  () const {
      return value;
    }

    base_s clone  (clone_context&) const {
      return std::make_shared<plain<T>>(T(value));
    }

    bool is_fixed() const {
      return true;
    }

  };

    template<class T>
  struct settable_plain : plain<T>, settable<T> {
    using plain<T>::plain;

    base_s clone(clone_context&) const {
        return std::make_shared<settable_plain<T>>(T(plain<T>::value));
    }

    bool set(const T& newval) {
        plain<T>::value = newval;
        return true;
    }

    bool is_fixed() const {
        return false;
    }
  };

    template<class T> T
  parse(const char* str, size_t len);

    template<class T> inline T
  parse(const string& str, const string& msg) {
    try {
      return parse<T>(str.data(), str.size());
    } catch (const std::exception& e) {
      throw node_error("In " + msg + ": " + e.what());
    }
  }

    template<class Type, class Return> std::shared_ptr<Type>
  parse_plain(const tstring& value) {
    return std::make_shared<Type>(parse<Return>(value, "parse_plain"));
  }
}
