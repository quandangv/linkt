#include "execstream.hpp"
#include "test.h"

#include <string>
#include <array>

string writeread(execstream&& stream, string input) {
  stream.write(input);
  return stream.readall();
}

int exitcode(execstream&& stream) {
  return WEXITSTATUS(stream.close());
}

TEST(Execstream, out) {
  EXPECT_EQ("foo\nbar\n", execstream("echo foo; echo bar", execstream::type_out).readall());
}
TEST(Execstream, filtered_out) {
  EXPECT_EQ("foo\n", execstream("echo foo; >&2 echo bar", execstream::type_out).readall());
}
TEST(Execstream, filtered_err) {
  EXPECT_EQ("bar\n", execstream("echo foo; >&2 echo bar", execstream::type_err).readall());
}
TEST(Execstream, out_err) {
  EXPECT_EQ("foo\nbar\n", execstream("echo foo; echo bar >&2", execstream::type_out_err).readall());
}
TEST(Execstream, excess_out_err) {
  EXPECT_EQ("foo\nbar\n", execstream("echo foo; echo bar", execstream::type_out_err).readall());
}
TEST(Execstream, exitcode) {
  EXPECT_EQ(127, exitcode(execstream("nexist", execstream::type_out)));
}
TEST(Execstream, full) {
  EXPECT_EQ("hello beautiful world\n", writeread(execstream("echo hello $(head -1) world", execstream::type_full), "beautiful\n"));
}
