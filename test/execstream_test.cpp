#include "execstream.hpp"
#include "test.h"

#include <string>
#include <array>

using namespace std;

class Execstream : public ::testing::TestWithParam<int> {};
INSTANTIATE_TEST_CASE_P(Instantiation, Execstream, ::testing::Range(0, 1));

string writeread(execstream&& stream, string input) {
  stream.write(input);
  return stream.readall();
}

int exitcode(execstream&& stream) {
  return WEXITSTATUS(stream.close());
}

TEST_P(Execstream, out) {
  EXPECT_EQ("foo\nbar\n", execstream("echo foo; echo bar", execstream::type_out).readall());
}

TEST_P(Execstream, filtered_out) {
  EXPECT_EQ("foo\n", execstream("echo foo; >&2 echo bar", execstream::type_out).readall());
}

TEST_P(Execstream, filtered_err) {
  EXPECT_EQ("bar\n", execstream("echo foo; >&2 echo bar", execstream::type_err).readall());
}

TEST_P(Execstream, out_err) {
  EXPECT_EQ("foo\nbar\n", execstream("echo foo; echo bar >&2", execstream::type_out_err).readall());
}

TEST_P(Execstream, excess_out_err) {
  EXPECT_EQ("foo\nbar\n", execstream("echo foo; echo bar", execstream::type_out_err).readall());
}

TEST_P(Execstream, exitcode) {
  EXPECT_EQ(127, exitcode(execstream("nexist", execstream::type_out)));
}

TEST_P(Execstream, full) {
  EXPECT_EQ("hello beautiful world\n", writeread(execstream("echo hello $(head -1) world", execstream::type_full), "beautiful\n"));
}

