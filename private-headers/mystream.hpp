#pragma once

#include <cstdio>
#include <sys/types.h>

struct mystream {
  FILE* fp;
  pid_t pid;

  mystream(const char* cmd, int type);
  mystream(mystream&&);
  ~mystream();

  static constexpr int type_out = 0b001;
  static constexpr int type_in = 0b010;
  static constexpr int type_err = 0b100;
  static constexpr int type_out_err = 0b101;
  static constexpr int type_full = 0b111;

  int close();
};

