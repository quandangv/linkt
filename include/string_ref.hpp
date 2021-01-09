#pragma once

#include <string>
#include <optional>
#include <memory>
#include <cspace/processor.hpp>

namespace lini {
  using std::string;
  using opt_str = std::optional<string>;

  struct string_ref {
    virtual string get() const = 0;
    virtual bool readonly() const { return true; }
    virtual void set(const string&) {}
    virtual ~string_ref() {}
  };

  using string_ref_p = std::unique_ptr<string_ref>;

  struct onetime_ref : public string_ref {
    mutable string val;

    onetime_ref(string&& val) : val(val) {}
    string get() const { return val; }
    string get_onetime() const { return move(val); }
  };
    
  struct const_ref : public string_ref {
    string val;

    const_ref(string&& val) : val(val) {}
    string get() const { return val; }
    bool readonly() const { return false; }
    void set(const string& value) { val = value; }
  };

  struct local_ref : public string_ref {
    string_ref_p& ref;

    local_ref(string_ref_p& ref) : ref(ref) {}
    string get() const { return ref->get(); }
    bool readonly() const { return ref->readonly(); }
    void set(const string& value) { ref->set(value); }
  };

  struct fallback_ref : public string_ref {
    string_ref_p fallback;

    string use_fallback(const string& error_message) const;
  };

  struct meta_ref : public fallback_ref {
    string_ref_p value;
  };

  struct color_ref : public meta_ref {
    cspace::processor processor;

    string get() const;
  };

  struct env_ref : public meta_ref {
    string get() const;
    bool readonly() const;
    void set(const string& value);
  };

  struct cmd_ref : public meta_ref {
    string get() const;
  };

  struct file_ref : public meta_ref {
    string get() const;
    bool readonly() const;
    void set(const string& value);
  };
}
