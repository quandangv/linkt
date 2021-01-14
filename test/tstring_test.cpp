#include "tstring.hpp"
#include "test.h"

#include <vector>
#include <tuple>

#include "logger.hpp"

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
  EXPECT_EQ(expect, trim(reality));
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
  EXPECT_EQ(cut_front_back(src, test_set.front, test_set.back), test_set.match)
      << "Source: " << test_set.src;
  EXPECT_EQ(src, test_set.result);
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
  trim_quotes(src);
  EXPECT_EQ(src, test_set.result)
      << "Source: " << test_set.src;
}

constexpr char test_source[] = "123456";

struct substr_test { size_t pos, end_pos, index, length; string result; };
class SubstrTest : public ::testing::Test, public ::testing::WithParamInterface<substr_test> {};

vector<substr_test> substr_tests = {
  {2, 6, 1, 2, "45"},
  {0, 6, 9, 1, ""},
  {1, 5, 1, 9, "345"},
};
INSTANTIATE_TEST_SUITE_P(TString, SubstrTest, ::testing::ValuesIn(substr_tests));

TEST_P(SubstrTest, substr) {
  auto test_set = GetParam();
  tstring str;
  {
    tstring tmp(test_source, test_set.pos, test_set.end_pos);
    str = substr(tmp, test_set.index, test_set.length);
  }
  EXPECT_EQ((string)str, test_set.result);
}

struct erase_test { int num; string result; };
class EraseTest : public ::testing::Test, public ::testing::WithParamInterface<erase_test> {};

vector<erase_test> erase_tests = {
  {2, "3456"},
  {-3, "123"},
  {1, "23456"},
  {0, "123456"},
  {6, ""},
  {9, ""},
  {-9, ""},
};
INSTANTIATE_TEST_SUITE_P(TString, EraseTest, ::testing::ValuesIn(erase_tests));
TEST_P(EraseTest, erase_front_back) {
  auto testset = GetParam();
  auto reality = tstring(test_source);
  if (testset.num < 0)
    reality.erase_back(-testset.num);
  else
    reality.erase_front(testset.num);
  EXPECT_EQ(testset.result, reality)
      << "Num: " << testset.num;
}

struct erase_mid_test { size_t off, len; string result; };
class erase_mid_test_intf : public ::testing::Test, public ::testing::WithParamInterface<erase_mid_test> {};

vector<erase_mid_test> erase_mid_tests = {
  {0, 0, "123456"},
  {0, 1, "23456"},
  {4, 9, "1234"},
  {1, 2, "1456"},
};
INSTANTIATE_TEST_SUITE_P(TString, erase_mid_test_intf, ::testing::ValuesIn(erase_mid_tests));
TEST_P(erase_mid_test_intf, erase_mid) {
  auto src = string(test_source);
  auto testset = GetParam();
  tstring reality(src);
  reality.erase(src, testset.off, testset.len);
  EXPECT_EQ(testset.result, reality)
      << "Start: " << testset.off << ", Length: " << testset.len;
}

struct find_test { string source; char c; int result, r_result; };
class find_test_intf : public ::testing::Test, public ::testing::WithParamInterface<find_test> {};

vector<find_test> find_tests = {
  {"123454321", '2', 1, 7},
  {"123454301", '2', 1, 1},
  {"123404321", '5', -1, -1},
  {"123454321", 0, -1, -1},
};
INSTANTIATE_TEST_SUITE_P(TString, find_test_intf, ::testing::ValuesIn(find_tests));
TEST_P(find_test_intf, find) {
  auto testset = GetParam();
  tstring src(testset.source);
  EXPECT_EQ(find(src, testset.c), testset.result)
      << "Source: " << src<< ", character: " << testset.c;
  EXPECT_EQ(rfind(src, testset.c), testset.r_result)
      << "Source: " << src<< ", character: " << testset.c;
}

TEST(TString, other) {
  string str;

  str = "123456789";
  EXPECT_EQ(string("123456789").length(), tstring(move(str)).length());
  EXPECT_EQ(string("123456789").length(), tstring("123456789").length());
  EXPECT_EQ(string("123456789"), tstring("123456789"));

  str = "";
  EXPECT_EQ(string("").length(), tstring(move(str)).length());
  EXPECT_EQ(string("").length(), tstring("").length());
  EXPECT_EQ(string(""), tstring(""));

  str = "123456";
  EXPECT_EQ(string("").length(), tstring(move(str)).erase_front(9).length());
  EXPECT_EQ(string("").length(), tstring("123456").erase_front(9).length());
  EXPECT_EQ(string(""), tstring("123456").erase_front(9));
}

