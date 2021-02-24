#pragma once

#include <istream>

#include "node/wrapper.hpp"

namespace lini {
  struct errorlist : std::vector<std::pair<std::string, std::string>> {
    void report_error  (int line, const std::string& msg);
    void report_error  (int line, const std::string& key, const std::string& msg);
    bool extract_key  (tstring& line, int linecount, char separator, tstring& key);
  };
  void parse_ini  (std::istream&, node::wrapper&, errorlist&);
  void write_ini  (std::ostream&, const node::wrapper&, const string& prefix = "");
  void parse_yml  (std::istream&, node::wrapper&, errorlist&);
  void write_yml  (std::ostream&, const node::wrapper&, int indent = 0);

  void write_key  (std::ostream&, const std::string& prefix, std::string&& value);
}
