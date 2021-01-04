#pragma once

#include <string>
#include <optional>
#include <cspace/processor.hpp>

namespace lini {
  using std::string;
  using opt_str = std::optional<string>;
  class string_ref {
  public:
    virtual string get() const = 0;
    virtual bool readonly() const { return true; }
    virtual void set(string) {}
    virtual ~string_ref() {}
  };

  using string_ref_p = std::unique_ptr<string_ref>;

  class const_string : public string_ref {
  public:
    string val;

    const_string(string&& val) : val(val) {}
    string get() const { return val; }
    bool readonly() const { return false; }
    void set(string value) { val = value; }
  };

  class local_string : public string_ref {
    string_ref_p& ref;
  public:
    local_string(string_ref_p& ref) : ref(ref) {}
    string get() const { return ref->get(); }
    bool readonly() const { return ref->readonly(); }
    void set(string value) { ref->set(value); }
  };

  class fallback_string : public string_ref {
  public:
    opt_str fallback;
    string use_fallback(const string& error_message) const;
  };

  class env_string : public fallback_string {
  public:
    string name;
    string get() const;
    bool readonly() const;
    void set(string value);
  };

  class file_string : public fallback_string {
  public:
    string path;
    string get() const;
    bool readonly() const;
    void set(string value);
  };

  class color_string : public string_ref {
  public:
    string_ref_p ref;
    cspace::processor processor;
    string get() const;
  };
}
