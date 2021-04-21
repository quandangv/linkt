#pragma once

#include <iostream>

#include "node/wrapper.hpp"
#include "node/parse.hpp"

void parse_ini(std::istream&, node::errorlist&, node::wrapper_s& output);
void write_ini(std::ostream&, const node::wrapper_s&, const string& prefix = "");

void parse_yml(std::istream&, node::errorlist&, node::wrapper_s& output);
void write_yml(std::ostream&, const node::wrapper_s&, int indent = 0);

void replace_text(std::istream&, std::ostream&, node::wrapper_s& replacements);
