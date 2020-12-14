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
[test.forbidden]\n\
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
key-b = 'dup'\n\
key-c = \"  c  \"   \n\
key-c = \"dup\"   \n\
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
        2, 3, 6, 8, 12, 17, 19
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
  // cout << to_string(doc);
  auto expected = GetParam().second;
  for(auto& line : expected.err) {
    auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == line; });
    EXPECT_NE(pos, err.end()) << "line = " << line;
    if (pos != err.end())
      err.erase(pos);
  }
  for(auto& e : err) {
    EXPECT_FALSE(true)
      << "Line num: " << e.first << endl
      << "Message: " << e.second;
  }
  for(auto& key : expected.keys) {
    auto sec_fix = fixed_string(string(get<0>(key)));
    auto key_fix = fixed_string(string(get<1>(key)));
    auto val_fix = fixed_string(string(get<2>(key)));
    auto& section = doc[sec_fix];
    EXPECT_TRUE(section.find(key_fix) != section.end())
        << "Key: " << get<0>(key) << "." << get<1>(key);
    EXPECT_EQ(section[key_fix].to_string(), get<2>(key))
        << "Key: " << get<0>(key) << "." << get<1>(key);
    section.erase(fixed_string(key_fix));
  }
  for(auto& section : doc) {
    for(auto& keyval : section.second) {
      EXPECT_FALSE(true)
          << "Key: " << section.first.to_string() << "." << keyval.first.to_string();
    }
  }
}
