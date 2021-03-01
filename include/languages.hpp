#pragma once

#include <istream>

#include "node/wrapper.hpp"

void parse_ini  (std::istream&, node::wrapper&, node::errorlist&);
void write_ini  (std::ostream&, const node::wrapper&, const string& prefix = "");

void parse_yml  (std::istream&, node::wrapper&, node::errorlist&);
void write_yml  (std::ostream&, const node::wrapper&, int indent = 0);

