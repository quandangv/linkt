#pragma once

#include <istream>

#include "node/wrapper.hpp"

namespace linked_nodes {
  using node::errorlist;

  void parse_ini  (std::istream&, node::wrapper&, errorlist&);
  void write_ini  (std::ostream&, const node::wrapper&, const string& prefix = "");

  void parse_yml  (std::istream&, node::wrapper&, errorlist&);
  void write_yml  (std::ostream&, const node::wrapper&, int indent = 0);

  void write_key  (std::ostream&, const std::string& prefix, std::string&& value);
}
