#pragma once

#include "error.hpp"

#include <cstdio>
#include <sys/types.h>
#include <string>

struct execstream {
  struct error : lini::error_base { using error_base::error_base; };

  FILE* fp;
  pid_t pid;

  execstream(const char* cmd, int type);
  execstream(execstream&&);
  ~execstream();

  static constexpr int type_out = 0b001;
  static constexpr int type_in = 0b010;
  static constexpr int type_err = 0b100;
  static constexpr int type_out_err = 0b101;
  static constexpr int type_full = 0b111;

  std::string readall();
  int write(const std::string&);
  int close();
};

