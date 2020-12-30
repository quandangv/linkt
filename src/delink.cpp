#include "delink.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <functional>
#include <cspace/interface.hpp>

#include "tstring.hpp"
#include "logger.hpp"
#include "error.hpp"

inline void delink(document& doc, str_errlist& err, const string& sec, const string& key, string& src) {
  auto report_err = [&](const string& msg) {
    err[sec + "." + key] = msg;
  };
  cspace::interface intf;

  tstring mod(src);
  if (!mod.cut_front_back("${", "}")) return;

  tstring fallback;
  auto take_fallback = [&] {
    auto sep = mod.rfind(':');
    if (sep != tstring::npos) {
      fallback = mod.substr(sep + 1).trim_quotes();
      mod.set_length(sep);
    }
  };
  auto use_fallback = [&](const string& failmsg) {
    if (fallback.untouched()) {
      report_err("De-linking failed and no fallback was found. Reason: " + failmsg);
    } else src = fallback.to_string();
  };
  std::function<bool()> subroutine = [&]() {
    if (mod.cut_front_back("file:", "")) {
      // Delink file
      take_fallback();
      // Copy the string because the original string have to be returned if the operation fail and no fallback provided
      std::ifstream ifs(mod.trim_quotes().to_string().data());
      if (!ifs.fail()) {
        src.assign(std::istreambuf_iterator<char>{ifs}, {});
        ifs.close();
        auto last_line = src.find_last_not_of("\r\n");
        src.erase(last_line + 1);
        return true;
      } else return use_fallback("Can't read file: " + mod.to_string()), false;
    } else if (mod.cut_front_back("env:", "")) {
      // Delink environment variable
      take_fallback();
      if (auto new_value = std::getenv(mod.trim_quotes().to_string().data()); new_value != nullptr) {
        src = string(new_value);
        return true;
      } else return use_fallback("Environment variable doesn't exist: " + mod.to_string()), false;
    } else if (mod.cut_front_back("color:", "")) {
      // Delink color
      auto sep = mod.find(';');
      intf.modifications.clear();
      if (sep != tstring::npos) {
        auto sep2 = mod.find(':');
        if (sep2 != tstring::npos) {
          intf.inter = cspace::stospace(mod.substr(0,sep2).trim());
        } else
          intf.inter = cspace::colorspaces::cielab;
        intf.add_modification(mod.substr(sep2 + 1, sep - sep2 - 1).to_string());
        mod.erase_front(sep + 1);
      }
      take_fallback();
      mod.trim();
      if (mod[0] == '$') {
        mod.erase_front();
        subroutine();
        mod = tstring(src);
      }
      if (mod[0] == '#') {
        mod.erase_front();
        src = intf.operate_hex(mod.to_string());
        logger::debug(src);
        return true;
      } else {
        auto pos = mod.find('(');
        if (pos != tstring::npos && mod.back() == ')') {
          auto colorspace = mod.substr(0, pos);
          intf.add_term(colorspace + ":");
          mod.erase_front(pos + 1);
          mod.erase_back();
          std::stringstream ss(mod.to_string());
          for(double d; ss >> d;) {
            if (ss.peek() == ',')
              ss.ignore();
            if ((src = intf.add_data(d)) != "") {
              if (ss >> d)
                report_err("Excess color component: " + ss.str());
              break;
            }
          }
          return true;
      } else return use_fallback("Color string is invalid: " + mod.to_string()), false;
      }
    } else {
      // Delink local value
      take_fallback();
      string new_sec, new_key;
      if (auto sep = mod.find('.'); sep != tstring::npos) {
        new_sec = mod.substr(0, sep).trim().to_string();
        new_key = mod.substr(sep + 1).trim().to_string();
      } else {
        new_key = mod.trim().to_string();
        new_sec = sec;
      }
      if (auto new_value = find(doc, new_sec, new_key); new_value != nullptr) {
        // Clear to avoid cyclical linking
        src.clear();
        delink(doc, err, new_sec, new_key, *new_value);
        src = *new_value;
        return true;
      } else return use_fallback("Referenced key doesn't exist: " + new_sec + "." + new_key), false;
    }
  };
  subroutine();
}

void delink(document& doc, str_errlist& err) {
  for(auto& seckey : doc) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink(doc, err, seckey.first, keyval.first, keyval.second);
    }
  }
}
