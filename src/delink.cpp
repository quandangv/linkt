#include "parse_delink.hpp"
#include "logger.hpp"
#include "tstring.hpp"
#include "error.hpp"

#include <stdexcept>
#include <functional>
#include <cspace/colorspace.hpp>

GLOBAL_NAMESPACE

using namespace std;
constexpr char scope[] = "delink";

void delink(document& doc, str_errlist& err) {
  std::function<void(const string&, const string&, string_ref_p&)> delink_key = [&](const string& sec, const string& key, string_ref_p& value) {
    auto report_err = [&](const string& msg) { err.emplace_back(sec + "." + key, msg); };
    if (!value)
      return report_err("Null value detected, possibly due to cyclical referencing");
    
    string src;
    if (auto onetime = dynamic_cast<onetime_string*>(value.get()); onetime != nullptr) {
      src = onetime->get_onetime();
    } else return;
    tstring mod(src);
    if (!mod.cut_front_back("${", "}")) {
      value = make_unique<const_string>(move(src));
      return;
    }

    auto take_fallback = [&](string_ref_p& fallback) {
      if (auto sep = mod.rfind('?'); sep != tstring::npos) {
        fallback = make_unique<onetime_string>(mod.substr(sep + 1).trim_quotes().to_string());
        logger::debug(fallback->get());
        delink_key(sec, key, fallback);
        logger::debug(fallback->get());
        mod.set_length(sep);
      }
    };
    std::function<void()> subroutine = [&] {
      if (mod.cut_front_back("file:", "")) {
        // Delink file
        auto newval = std::make_unique<file_string>();
        take_fallback(newval->fallback);
        newval->path = mod.trim_quotes().to_string();
        value = move(newval);
      } else if (mod.cut_front_back("env:", "")) {
        // Delink environment variable
        auto newval = std::make_unique<env_string>();
        take_fallback(newval->fallback);
        newval->name = mod.trim_quotes().to_string();
        value = move(newval);
      } else if (mod.cut_front_back("color:", "")) {
        // Delink color
        auto newval = std::make_unique<color_string>();
        if (auto sep = mod.find(';'); sep != tstring::npos) {
          auto sep2 = mod.find(':');
          if (sep2 != tstring::npos)
            newval->processor.inter = cspace::stospace(mod.substr(0,sep2++).trim());
          else sep2 = 0;
          newval->processor.add_modification(mod.substr(sep2, sep - sep2).to_string());
          mod.erase_front(sep + 1);
        }
        mod.trim();
        if (mod[0] == '$') {
          mod.erase_front();
          subroutine();
          newval->ref = move(value);
        } else newval->ref = std::make_unique<const_string>(mod.to_string());
        value = move(newval);
      } else {
        // Delink local value
        string_ref_p fallback;
        take_fallback(fallback);

        string new_sec, new_key;
        if (auto sep = mod.find('.'); sep != tstring::npos) {
          new_sec = mod.substr(0, sep).trim().to_string();
          mod.erase_front(sep + 1);
        } else new_sec = sec;
        new_key = mod.trim().to_string();

        if (auto index = doc.find(new_sec, new_key); index) {
          // Clear to avoid cyclical linking
          value.reset();
          auto& ref_val = doc.values[*index];
          if (!ref_val) {
            report_err("Cyclical referencing detected");
            value = make_unique<const_string>(move(src));
          } else {
            delink_key(new_sec, new_key, ref_val);
            value = std::make_unique<local_string>(ref_val);
          }
        } else if (fallback)
          value = move(fallback);
        else {
          value = make_unique<const_string>(move(src));
          return report_err("Referenced key doesn't exist: " + new_sec + "." + new_key);
        }
      }
    };
    subroutine();
  };
  for(auto& seckey : doc.map) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink_key(seckey.first, keyval.first, doc.values[keyval.second]);
    }
  }
}

GLOBAL_NAMESPACE_END
