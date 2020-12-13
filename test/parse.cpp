#include "parse.hpp"
#include "test.h"

#include <algorithm>

using namespace std;
struct parse_result {
  vector<tuple<string, string, string>> keys;
  vector<int> err;
};
using TestSet = pair<string, parse_result>;

class GetTest : public ::testing::Test, public ::testing::WithParamInterface<TestSet> {};

vector<TestSet> parse_tests = {
  {"key-rogue=rogue \n\
[test-excess];excess\n\
[test.forbidden];excess\n\
[test]\n\
key-a = a \n\
  key b = b\n\
key-cmt = ;cmt\n\
key#forbidden = a\n\
; comment = abc\n\
 key-c=c\n\
key-empty=   \n\
[test-missing\n\
key-test= test \n\
[test2]\n\
key-test2 = test2\n\
key-b = 'b  '\n\
key-c = \"  c  \"   \n\
key-a = '    a\"",
    {
      { // keys
        {"", "key-rogue", "rogue"},
        {"test", "key-a", "a"},
        {"test", "key-cmt", ";cmt"},
        {"test", "key-c", "c"},
        {"test", "key-empty", ""},
        {"test", "key-test", "test"},
        {"test2", "key-test2", "test2"},
        {"test2", "key-b", "b  "},
        {"test2", "key-c", "  c  "},
        {"test2", "key-a", "'    a\""},
      },
      { // err
        2, 3, 8, 12
      }
    }
  }
};
INSTANTIATE_TEST_SUITE_P(Parse, GetTest, ::testing::ValuesIn(parse_tests));

TEST_P(GetTest, parse_string) {
  stringstream ss{GetParam().first};
  document doc;
  errorlist err;
  parse(ss, doc, err);
  auto expected = GetParam().second;
  for(auto& key : expected.keys) {
    auto& section = doc[get<0>(key)];
    EXPECT_TRUE(section.find(get<1>(key)) != section.end())
        << "Key: " << get<0>(key) << "." << get<1>(key);
    EXPECT_EQ(section[get<1>(key)], get<2>(key))
        << "Key: " << get<0>(key) << "." << get<1>(key);
    section.erase(get<1>(key));
  }
  for(auto& key : doc) {
    EXPECT_TRUE(key.second.empty())
        << "Section name: " << key.first << endl
        << "First element: " << key.second.begin()->first;
  }
  for(auto& line : expected.err) {
    auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == line; });
    EXPECT_NE(pos, err.end()) << "line = " << line;
    err.erase(pos);
  }
}
