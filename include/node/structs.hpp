#pragma once

#include "tstring.hpp"

#include <vector>
#include <memory>

namespace node {
  struct wrapper;
  template<class T> struct base;
  using std::string;
  using base_s = std::shared_ptr<base<string>>;
  using wrapper_s = std::shared_ptr<wrapper>;
  using const_wrapper_s = std::shared_ptr<const wrapper>;

  struct errorlist : std::vector<std::pair<string, string>> {
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

  struct parse_context {
    bool parent_based_ref{false};

    string raw, current_path;
    wrapper_s parent, current, root;
    base_s* place{nullptr};

    wrapper_s get_current();
    wrapper_s get_parent();
    base_s& get_place();
    friend struct parse_context_base;
  };

  class parse_preprocessed {
    base_s fallback;
  public:
    std::array<tstring, 7> tokens;
    unsigned char token_count{0};
    tstring process(tstring&);
    void set_fallback(base_s fb) { fallback = fb; }
    bool has_fallback() { return fallback.get(); }
    base_s pop_fallback() { auto result = std::move(fallback); return result; }
  };
}
