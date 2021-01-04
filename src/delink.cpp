#include "parse_delink.hpp"
#include "logger.hpp"

#include <stdexcept>
#include <cspace/colorspace.hpp>

#include "tstring.hpp"
#include "error.hpp"

GLOBAL_NAMESPACE

inline void delink(document& doc, str_errlist& err, const string& sec, const string& key, string_ref_p& value) {
  auto report_err = [&](const string& msg) {
    err.emplace_back(sec + "." + key, msg);
  };

  if (!value) {
    report_err("Null value detected, possibly due to cyclical referencing: ");
    value = std::make_unique<const_string>("");
    return;
  }
  tstring mod(value->get());
  if (!mod.cut_front_back("${", "}")) return;

  auto take_fallback = [&](opt_str& fallback) {
    auto sep = mod.rfind(':');
    if (sep != tstring::npos) {
      fallback = mod.substr(sep + 1).trim_quotes().to_string();
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
      auto sep = mod.find(';');
      if (sep != tstring::npos) {
        auto sep2 = mod.find(':');
        if (sep2 != tstring::npos)
          newval->processor.inter = cspace::stospace(mod.substr(0,sep2++).trim());
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
      opt_str fallback;
      take_fallback(fallback);
      string new_sec, new_key;
      if (auto sep = mod.find('.'); sep != tstring::npos) {
        new_sec = mod.substr(0, sep).trim().to_string();
        new_key = mod.substr(sep + 1).trim().to_string();
      } else {
        new_key = mod.trim().to_string();
        new_sec = sec;
      }
      if (auto index = doc.find(new_sec, new_key); index) {
        auto& ref_val = doc.values[*index];
        // Clear to avoid cyclical linking
        value.reset();
        delink(doc, err, new_sec, new_key, ref_val);
        value = std::make_unique<local_string>(ref_val);
      } else if (fallback)
        value = std::make_unique<const_string>(move(*fallback));
      else
        return report_err("Referenced key doesn't exist: " + new_sec + "." + new_key);
    }
  };
  subroutine();
}

void delink(document& doc, str_errlist& err) {
  for(auto& seckey : doc.map) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink(doc, err, seckey.first, keyval.first, doc.values[keyval.second]);
    }
  }
}

GLOBAL_NAMESPACE_END
