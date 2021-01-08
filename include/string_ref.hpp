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
    virtual void set(string) {}
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
    void set(string value) { val = value; }
  };

  struct local_ref : public string_ref {
    string_ref_p& ref;

    local_ref(string_ref_p& ref) : ref(ref) {}
    string get() const { return ref->get(); }
    bool readonly() const { return ref->readonly(); }
    void set(string value) { ref->set(value); }
  };

  struct color_ref : public string_ref {
    string_ref_p ref;
    cspace::processor processor;

    string get() const;
  };

  struct fallback_ref : public string_ref {
    string_ref_p fallback;

    string use_fallback(const string& error_message) const;
  };

  struct env_ref : public fallback_ref {
    string name;

    string get() const;
    bool readonly() const;
    void set(string value);
  };

  struct cmd_ref : public fallback_ref {
    string name;

    string get() const;
  };

  struct file_ref : public fallback_ref {
    string name;

    string get() const;
    bool readonly() const;
    void set(string value);
  };
}
