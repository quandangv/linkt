#include "fixed_string.hpp"
#include "test.h"

#include <vector>
#include <tuple>

using namespace std;

using comp_test = pair<string, string>;
class CompTest : public ::testing::Test, public ::testing::WithParamInterface<comp_test> {};

vector<comp_test> comp_tests = {
  {"", ""},
  {"123456789", "123456789"},
  {"123456789", "1234567"},
  {"123456789", "123456789abcdef"},
  {"123456789", "213456789"},
  {"423456789", "123456789"},
};
INSTANTIATE_TEST_SUITE_P(FixedString, CompTest, ::testing::ValuesIn(comp_tests));
TEST_P(CompTest, comp_equality) {
  auto a = tmp_fixed_string(GetParam().second);
  auto b = tmp_fixed_string(GetParam().first);
  EXPECT_EQ(GetParam().first <=> GetParam().second, a <=> b)
      << "First: " << GetParam().first << endl
      << "Second: " << GetParam().second;
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
  { "abc def_   ", "abc def_" },
};
INSTANTIATE_TEST_SUITE_P(FixedString, TrimTest, ::testing::ValuesIn(trim_tests));
TEST_P(TrimTest, trim_equality) {
  auto expect = tmp_fixed_string(GetParam().second);
  auto reality = tmp_fixed_string(GetParam().first);
  reality.trim();
  EXPECT_EQ(expect.to_string(), reality.to_string());
  EXPECT_EQ(expect, reality);
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
INSTANTIATE_TEST_SUITE_P(FixedString, EraseTest, ::testing::ValuesIn(erase_tests));
TEST_P(EraseTest, trim_equality) {
  auto expect = fixed_string(get<2>(GetParam()));
  auto reality = fixed_string(get<0>(GetParam()));
  auto num = get<1>(GetParam());
  if (num < 0) reality.erase_back(-num);
  else reality.erase_front(num);
  EXPECT_EQ(expect.to_string(), reality.to_string())
      << "Input: " << get<0>(GetParam()) << endl
      << "Num: " << get<1>(GetParam());
  EXPECT_EQ(expect, reality);
}

TEST(FixedString, other) {
  string str;

  str = "123456789";
  EXPECT_EQ(string("123456789").length(), tmp_fixed_string(move(str)).length());
  EXPECT_EQ(string("123456789").length(), tmp_fixed_string("123456789").length());
  EXPECT_EQ(string("123456789"), fixed_string("123456789").to_string());

  str = "";
  EXPECT_EQ(string("").length(), tmp_fixed_string(move(str)).length());
  EXPECT_EQ(string("").length(), tmp_fixed_string("").length());
  EXPECT_EQ(string(""), tmp_fixed_string("").to_string());

  str = "123456";
  EXPECT_EQ(string("").length(), tmp_fixed_string(move(str)).erase_front(9).length());
  EXPECT_EQ(string("").length(), tmp_fixed_string("123456").erase_front(9).length());
  EXPECT_EQ(string(""), tmp_fixed_string("123456").erase_front(9).to_string());
}

TEST(FixedString, scope) {
  fixed_string fstr;
  {
    string s = "hello";
    fstr = fixed_string(s);
  }
  string g = "lolol";
  EXPECT_EQ(fstr.to_string(), "hello");
  {
    string s = "";
    fstr = fixed_string(s);
  }
  g = "lolol";
  EXPECT_EQ(fstr.to_string(), "");
}

TEST(FixedString, move) {
  {
    fixed_string tmpstr("123456789");
    fixed_string fstr;
    tmpstr.erase_front(3);
    EXPECT_EQ(tmpstr.to_string(), "456789");
    fstr = move(tmpstr);
    EXPECT_EQ(tmpstr.to_string(), "");
    EXPECT_EQ(fstr.to_string(), "456789");
  }
}
