#include "tstring.hpp"
#include "test.h"

#include <vector>
#include <tuple>

#include "logger.hpp"

using namespace std;

struct substr_test {
  string src;
  size_t pos, end_pos, index, length;
  string result;
};
class SubstrTest : public ::testing::Test, public ::testing::WithParamInterface<substr_test> {};

vector<substr_test> substr_tests = {
  {"123456789", 2, 9, 3, 3, "678"},
  {"123456789", 0, 9, 10, 1, ""},
  {"123456789", 5, 8, 1, 5, "78"},
};
INSTANTIATE_TEST_SUITE_P(TString, SubstrTest, ::testing::ValuesIn(substr_tests));

TEST_P(SubstrTest, substr) {
  auto test_set = GetParam();
  tstring str(test_set.src.data(), test_set.pos, test_set.end_pos);
  str = str.substr(test_set.index, test_set.length);
  EXPECT_EQ(str.to_string(), test_set.result);
}

using comp_test = pair<string, string>;

vector<comp_test> comp_tests = {
  {"", ""},
  {"123456789", "123456789"},
  {"123456789", "1234567"},
  {"123456789", "123456789abcdef"},
  {"123456789", "213456789"},
  {"423456789", "123456789"},
};
class CompTest : public ::testing::Test, public ::testing::WithParamInterface<comp_test> {};
INSTANTIATE_TEST_SUITE_P(TString, CompTest, ::testing::ValuesIn(comp_tests));
TEST_P(CompTest, comp_equality) {
  auto a = tstring(GetParam().second);
  auto b = tstring(GetParam().first);
  EXPECT_EQ(GetParam().first < GetParam().second, a < b)
      << "First: " << GetParam().first << endl
      << "Second: " << GetParam().second;
  EXPECT_EQ(GetParam().first == GetParam().second, a == b)
      << "First: " << GetParam().first << endl
      << "Second: " << GetParam().second;
  EXPECT_EQ(GetParam().first > GetParam().second, a > b)
      << "First: " << GetParam().first << endl
      << "Second: " << GetParam().second;
}

using trim_test = pair<string, string>;
class TrimTest : public ::testing::Test, public ::testing::WithParamInterface<trim_test> {};

vector<trim_test> trim_tests = {
  { "   abcdef ", "abcdef" },
  { "   abc def ", "abc def" },
  { "abc def ", "abc def" },
  { "abc   def", "abc   def" },
  { "abc def\t   ", "abc def" },
  { "     \t   \t   ", "" },
  { "abc def_   ", "abc def_" },
};
INSTANTIATE_TEST_SUITE_P(TString, TrimTest, ::testing::ValuesIn(trim_tests));
TEST_P(TrimTest, trim_equality) {
  auto expect = tstring(GetParam().second);
  auto reality = tstring(GetParam().first);
  reality.trim();
  EXPECT_EQ(expect.to_string(), reality.to_string());
  EXPECT_EQ(expect, reality);
}

struct cut_test {
  const char *src, *front, *back, *result;
  bool match;
};
class CutTest : public ::testing::Test, public ::testing::WithParamInterface<cut_test> {};

vector<cut_test> cut_tests = {
  {"${abcedg}", "${", "}", "abcedg", true},
  {"${abcedg]", "${", "}", "${abcedg]", false},
  {"/*abcdef*/", "/*", "*/", "abcdef", true},
  {"/*abcdef", "/*", "*/", "/*abcdef", false},
  {"rgb:FF0000", "rgb:", "", "FF0000", true},
  {"hsl:", "hsl:", "", "", true},
  {".cpp", "", ".cpp", "", true},
  {"hsl:", "hsl!", "", "hsl:", false},
  {".cpp", "", ".cpP", ".cpp", false},
  {"rgb::hsl", "rgb:", ":hsl", "", true},
  {"", "", "", "", true},
  {"abcdef", "", "", "abcdef", true},
  {"' f a i l '", "'", "'", " f a i l ", true},
  {"\" f a i l '", "'", "'", "\" f a i l '", false},
};
INSTANTIATE_TEST_SUITE_P(TString, CutTest, ::testing::ValuesIn(cut_tests));
TEST_P(CutTest, cut) {
  auto& test_set = GetParam();
  tstring src(test_set.src);
  EXPECT_EQ(src.cut_front_back(test_set.front, test_set.back), test_set.match)
      << "Source: " << test_set.src;
  EXPECT_EQ(src.to_string(), test_set.result);
}

struct trim_quotes_test {
  const char *src, *result;
};
class TrimQuotesTest : public ::testing::Test, public ::testing::WithParamInterface<trim_quotes_test> {};

vector<trim_quotes_test> trim_quotes_tests = {
  {"123456", "123456"},
  {"", ""},
  {"  123456  ", "123456"},
  {"  ' 1 2 3' ", " 1 2 3"},
  {" ' 12345 6 \"", "' 12345 6 \""},
};
INSTANTIATE_TEST_SUITE_P(TString, TrimQuotesTest, ::testing::ValuesIn(trim_quotes_tests));
TEST_P(TrimQuotesTest, trim) {
  auto& test_set = GetParam();
  tstring src(test_set.src);
  EXPECT_EQ(src.trim_quotes().to_string(), test_set.result)
      << "Source: " << test_set.src;
}

using erase_test = tuple<string, int, string>;
class EraseTest : public ::testing::Test, public ::testing::WithParamInterface<erase_test> {};

vector<erase_test> erase_tests = {
  { "123456", 2, "3456" },
  { "123456", -3, "123" },
  { "123456", 1, "23456" },
  { "123456", 0, "123456" },
  { "123456", 6, "" },
  { "123456", 9, "" },
  { "123456", -9, "" },
};
INSTANTIATE_TEST_SUITE_P(TString, EraseTest, ::testing::ValuesIn(erase_tests));
TEST_P(EraseTest, trim_equality) {
  auto expect = tstring(get<2>(GetParam()));
  auto reality = tstring(get<0>(GetParam()));
  auto num = get<1>(GetParam());
  if (num < 0) reality.erase_back(-num);
  else reality.erase_front(num);
  EXPECT_EQ(expect.to_string(), reality.to_string())
      << "Input: " << get<0>(GetParam()) << endl
      << "Num: " << get<1>(GetParam());
  EXPECT_EQ(expect, reality);
}

TEST(TString, other) {
  string str;

  str = "123456789";
  EXPECT_EQ(string("123456789").length(), tstring(move(str)).length());
  EXPECT_EQ(string("123456789").length(), tstring("123456789").length());
  EXPECT_EQ(string("123456789"), tstring("123456789").to_string());

  str = "";
  EXPECT_EQ(string("").length(), tstring(move(str)).length());
  EXPECT_EQ(string("").length(), tstring("").length());
  EXPECT_EQ(string(""), tstring("").to_string());

  str = "123456";
  EXPECT_EQ(string("").length(), tstring(move(str)).erase_front(9).length());
  EXPECT_EQ(string("").length(), tstring("123456").erase_front(9).length());
  EXPECT_EQ(string(""), tstring("123456").erase_front(9).to_string());
}

