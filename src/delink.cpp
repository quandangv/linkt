#include "delink.hpp"

void delink(document& doc, str_errorlist& err) {
  auto delink = [&](const string& sec, const string& key, fixed_string& src) {
    auto report_err = [&](const string& msg) {
      err[sec + "." + key] = msg;
    };

    fixed_string backup(move(src));
    if (src.cut_front_back("${", "}")) {
      tmp_fixed_string mod(backup);
      auto sep = mod.find('.');
      if (sep == tmp_fixed_string::npos) {
        report_err("Unparsed link: " + backup.to_string());
        return;
      }
      auto new_sec = mod.sustr(0, sep).to_string();
      mod.erase_front(sep + 1);

      fixed_string* fallback;
      string new_key;
//      sep = mod.find(':');
      if (sep == tmp_fixed_string::npos) {
        fallback = &original;
//        new_key = mod.to_string();
      } else {
        new_key = mod.substr(0, sep).to_string();
//        mod.erase_front(sep + 1);
        fallback = &value;
      }
//
      if (auto new_value = find(doc, new_sec, new_key); new_value != nullptr) {
        src = move(*new_value);
//      } else src = 
    }
  };
//  for(auto& sec_key : doc) {
    auto& section = sec_key.second;
    for(auto& keyval : section) {
//    }
  }
//}
