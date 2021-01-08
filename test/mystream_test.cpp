#include "mystream.hpp"
#include "test.h"

#include <string>
#include <array>

using namespace std;

class Myopen : public ::testing::TestWithParam<int> {};
INSTANTIATE_TEST_CASE_P(Instantiation, Myopen, ::testing::Range(0, 10));

string readall(mystream&& stream) {
  string result;
  array<char, 128> buffer;
  while (fgets(buffer.data(), buffer.size(), stream.fp) != nullptr) {
    cout << result;
    result += buffer.data();
  }
  return result;
}

string writeread(mystream&& stream, string input) {
  fputs(input.data(), stream.fp);
  return readall(move(stream));
}

int exitcode(mystream&& stream) {
  return WEXITSTATUS(stream.close());
}

TEST_P(Myopen, out) {
  EXPECT_EQ("foo bar\n", readall(mystream("echo foo  bar", mystream::type_out)));
}

TEST_P(Myopen, filtered_out) {
  EXPECT_EQ("foo\n", readall(mystream("echo foo; >&2 echo bar", mystream::type_out)));
}

TEST_P(Myopen, filtered_err) {
  EXPECT_EQ("bar\n", readall(mystream("echo foo; >&2 echo bar", mystream::type_err)));
}

TEST_P(Myopen, out_err) {
  EXPECT_EQ("foo\nbar\n", readall(mystream("echo foo; >&2 echo bar", mystream::type_out_err)));
}

TEST_P(Myopen, exitcode) {
  EXPECT_EQ(127, exitcode(mystream("nexist", mystream::type_out)));
}

TEST_P(Myopen, full) {
  EXPECT_EQ("hello beautiful world\n", writeread(mystream("echo hello $(head -1) world", mystream::type_full), "beautiful\n"));
}

