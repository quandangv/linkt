#pragma once

#include <istream>

#include "node/wrapper.hpp"

node::wrapper_s parse_ini  (std::istream&, node::errorlist&);
void write_ini  (std::ostream&, const node::wrapper_s&, const string& prefix = "");

node::wrapper_s parse_yml  (std::istream&, node::errorlist&);
void write_yml  (std::ostream&, const node::wrapper_s&, int indent = 0);

