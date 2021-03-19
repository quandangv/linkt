#pragma once

#include "tstring.hpp"

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <cmath>

namespace node {
  template<class T> struct base;
  template<> struct base<string>;
  struct wrapper;
  using std::string;
  using base_s = std::shared_ptr<base<string>>;
  using wrapper_s = std::shared_ptr<wrapper>;
  using const_wrapper_s = std::shared_ptr<const wrapper>;

  struct node_error : std::logic_error { using logic_error::logic_error; };
  struct required_field_null_error : std::logic_error { using logic_error::logic_error; };
  struct clone_error : std::logic_error { using logic_error::logic_error; };
  struct parse_error : std::logic_error { using logic_error::logic_error; };

  struct errorlist : std::vector<std::pair<std::string, std::string>> {
    void report_error(int line, const std::string& msg) {
        emplace_back("line " +std::to_string(line), msg);
    }

    void report_error(int line, const std::string& key, const std::string& msg) {
        emplace_back("line " +std::to_string(line) + ", key " + key, msg);
    }

    void report_error(const std::string& key, const std::string& msg) {
        emplace_back(key, msg);
    }

    bool extract_key(tstring& line, int linecount, char separator, tstring& key);
  };

  struct clone_context {
    std::string current_path;
    std::vector<std::pair<const_wrapper_s, wrapper_s>> ancestors;
    bool optimize{false}, no_dependency{false};
    errorlist errors;

    void report_error(const string& msg) {
        errors.report_error(current_path, msg);
    }
  };

  struct throwing_clone_context : public clone_context {
    ~throwing_clone_context() noexcept(false);
  };

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
  struct base<int> : base<string> {
    virtual explicit operator int() const = 0;

    explicit operator string() const {
      return std::to_string((int)*this);
    }
  };

    template<>
  struct base<float> : base<int> {
      virtual explicit
    operator float() const = 0;

      virtual explicit
    operator int() const {
        return std::lround((float)*this);
    }

    explicit operator string() const {
        std::string str = std::to_string ((float)*this);
        auto erase = str.find_last_not_of('0');
        return str.erase (str[erase] == '.' ? erase : (erase + 1), std::string::npos );
    }
  };

    template<class T>
  struct plain : base<T> {
  protected:
    T value;
  public:
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

    template<class T> T
  parse(const string& str) {
    return parse<T>(str.data(), str.size());
  }

    template<class Type, class Return> std::shared_ptr<Type>
  parse_plain(const tstring& value) {
      return std::make_shared<Type>(parse<Return>(value.begin(), value.size()));
  }

  struct parse_context {
    bool parent_based_ref{false};

    string raw;
    wrapper_s parent, current;
    base_s* place{nullptr};

    wrapper_s get_current();
    wrapper_s get_parent();
    base_s& get_place();
    friend struct parse_context_base;
  };

  struct parse_preprocessed {
    std::array<tstring, 7> tokens;
    unsigned char token_count{0};
    base_s fallback;
    tstring process(tstring&);
  };
}
