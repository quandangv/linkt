#pragma once

#include "tstring.hpp"

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace node {
  template<class T> struct base;
  template<> struct base<string>;
  struct wrapper;
  using std::string;
  using base_s = std::shared_ptr<base<string>>;
  using base_w = std::weak_ptr<base<string>>;
  using wrapper_s = std::shared_ptr<wrapper>;
  using const_wrapper_s = std::shared_ptr<const wrapper>;
  using wrapper_w = std::weak_ptr<wrapper>;

  struct node_error : std::logic_error { using logic_error::logic_error; };
  struct required_field_null_error : std::logic_error { using logic_error::logic_error; };
  struct clone_error : std::logic_error { using logic_error::logic_error; };

  struct errorlist : std::vector<std::pair<std::string, std::string>> {
    void report_error  (int line, const std::string& msg)
    { emplace_back("line " +std::to_string(line), msg); }

    void report_error  (int line, const std::string& key, const std::string& msg)
    { emplace_back("line " +std::to_string(line) + ", key " + key, msg); }

    void report_error  (const std::string& key, const std::string& msg)
    { emplace_back(key, msg); }

    bool extract_key  (tstring& line, int linecount, char separator, tstring& key);
  };

  struct clone_context {
    std::string current_path;
    std::vector<std::pair<const_wrapper_s, wrapper_s>> ancestors;
    bool optimize{false}, no_dependency{false};
    errorlist errors;

    void report_error(const string& msg)
    { errors.report_error(current_path, msg); }
  };

  struct throwing_clone_context : public clone_context {
    ~throwing_clone_context() noexcept(false);
  };

  template<>
  struct base<string> {
    virtual ~base() {}

    virtual string get  () const = 0;
    virtual base_s clone  (clone_context&) const = 0;

    base_s checked_clone  (clone_context& context, const string& msg) const {
      auto result = clone(context);
      return result ?: throw clone_error("clone_error: Empty clone result in: " + msg);
    }
  };

  struct settable {
    virtual bool set  (const string&) = 0;
  };

  template<class T>
  struct base : base<string> {
    virtual explicit operator T() const = 0;
  };

  template<>
  struct base<int> : base<string> {
    virtual explicit operator int() const = 0;
    string get  () const { return std::to_string((int)*this); }
  };

  template<>
  struct base<float> : base<string> {
    virtual explicit operator float() const = 0;
    string get  () const {
      std::string str = std::to_string ((float)*this);
      auto erase = str.find_last_not_of('0');
      return str.erase (str[erase] == '.' ? erase : (erase + 1), std::string::npos );
    }
  };

  struct fixed {};

  struct plain : base<string>, settable, fixed {
    string val;

    explicit plain  (string&& val) : val(val) {}
    string get  () const { return val; }
    bool set  (const string& value) { val = value; return true; }
    base_s clone  (clone_context&) const { return std::make_shared<plain>(string(val)); }
  };

  //template<class T>
  //struct custom_plain : base<T>, fixed {
  //  T value;
  //  explicit operator T  () const { return value; }
  //};

  bool is_fixed(base_s node);
}
