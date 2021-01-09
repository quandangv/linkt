#include "string_inter.hpp"
#include "test.h"

TEST(Stringinter, basic) {
  string_inter si;
  si.base = "Hello  world, !";
  si.positions = {6, 14};
  vector<string> rep{"awful", "human"};
  EXPECT_EQ("Hello awful world, human!", si.get(rep));
}
