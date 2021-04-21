#pragma once

#include <iostream>

#include "node/wrapper.hpp"
#include "node/parse.hpp"

void parse_ini(std::istream&, node::errorlist&, node::wrapper_s& output);
void write_ini(std::ostream&, const node::wrapper_s&, const string& prefix = "");

void parse_yml(std::istream&, node::errorlist&, node::wrapper_s& output);
void write_yml(std::ostream&, const node::wrapper_s&, int indent = 0);

void replace_text(std::istream&, std::ostream&, node::wrapper_s& replacements);

inline node::wrapper_s parse_ini (std::istream& is, node::errorlist& err) {
  auto output = std::make_shared<node::wrapper>();
  parse_ini(is, err, output);
  return output;
}

inline node::wrapper_s parse_yml (std::istream& is, node::errorlist& err) {
  auto output = std::make_shared<node::wrapper>();
  parse_yml(is, err, output);
  return output;
}

