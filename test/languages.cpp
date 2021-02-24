#include "test.h"

#include <fstream>
#include <unistd.h>

struct file_test_param {
  struct expectation { string path, value; };

  string path, language;
  vector<expectation> expectations;
  vector<string> err;
};

void test_language(file_test_param testset) {
  std::ifstream ifs{testset.path + ".txt"};
  ASSERT_FALSE(ifs.fail());

  node::wrapper doc;
  errorlist err;
  if (testset.language == "ini")
    parse_ini(ifs, doc, err);
  else if (testset.language == "yml")
    parse_yml(ifs, doc, err);

  // Check for unexpected errors
  for(auto& e : err) {
    EXPECT_NE(find(testset.err.begin(), testset.err.end(), e.first), testset.err.end())
      << "Unexpected parsing error, at: " << e.first << endl << "Message: " << e.second;
  }
  // Check the keys
  for(auto& pair : testset.expectations)
    check_key(doc, pair.path, pair.value, false);

  // Check for expected errors
  for(auto& e : testset.err) {
    auto pos = find_if(err.begin(), err.end(), [&](auto it) { return it.first == e; });
    EXPECT_NE(pos, err.end()) << "Expected parsing error at: " << e;
  }

  // Check wrapper export
  std::ofstream ofs{testset.path + "_export.txt"};

  if (testset.language == "ini")
    write_ini(ofs, doc);
  else if (testset.language == "yml")
    write_yml(ofs, doc);

  auto command = "diff -Z '" + testset.path + "_output.txt' '" + testset.path + "_export.txt' 2>&1";
  auto file = popen(command.data(), "r");
  ASSERT_TRUE(file);
  std::array<char, 2> buf;
  fgets(buf.data(), 2, file);
  EXPECT_TRUE(feof(file))
      << "Output of command not empty: " << command;
  pclose(file);
}

TEST(Language, Plain_ini) {
  test_language({"ini_test", "ini",
    {
      {"key-rogue", "rogue"},
      {"test.key-a", "a"},
      {"test.key-cmt", ";cmt"},
      {"test.key-c", "c"},
      {"test.key-empty", ""},
      {"test.key-test", "test"},
      {"test.key", "key"},
      {"test2.key-test2", "test2"},
      {"test2.key-b", "b  "},
      {"test2.key-c", "  c  "},
      {"test.ref-ref-a", "${test2.ref-a ? failed}"},
      {"test2.ref-a", "${test.key-a}"},
      {"test2.ref-rogue", "${.key-rogue}"},
      {"test2.ref-nexist", "${test.key-nexist ? \" f a i l ' }"},
      {"test2.key-a", "'    a\""},
    },
    { "line 2", "line 7", "line 9", "line 13", "line 19", "line 21", }
  });
}

TEST(Language, Functional_ini) {
  test_language({"lemonbar_test", "ini",
    {
      {"msg", "hello"},
      {"simple", " 69 96 0 "},
      {"compact", " %{F#f00}CPU 69% %{F#ff0}RAM 96% %{F#0f0}TEMP 99*C %{F#0ff}BAT 0% "},
      {"mod.cpu", "%{F#f00}CPU 69%"},
      {"mod.ram", "%{F#ff0}RAM 96%"},
      {"mod.temp", "%{F#0f0}TEMP 99*C"},
      {"mod.bat", "%{F#0ff}BAT 0%"},
      {"stat.cpu", "69"},
      {"stat.ram", "96"},
      {"stat.temp-c", "99"},
      {"stat.bat", "0"},
    },
    {}
  });
}

TEST(Language, Yml) {
  test_language({"yml_test", "yml",
    {
      {"bar.base.B.dumb", "hello"},
      {"bar.bat.B.L.value", "60"},
      {"bar.bat.B.L", "0.800000"},
      {"bar.bat.stat", "60"},
      {"bar.bat.unit", "%"},
      {"bar.bat.B.dumb", "hello"},
      {"bar.bat.B", "#FF54CB"},
      {"bar.bat", "%{B#FF54CB} BAT 60%"},
      {"bar.F", "#fff"},
      {"bar", "%{F#fff}%{B#FF54CB} BAT 60% "},
    },
    {}
  });
}

node::wrapper doc;
void set_key(const string& key, const string& newval) {
  auto last_count = get_current_test_part_count();
  EXPECT_TRUE(doc.set(key, newval));
  ASSERT_EQ(newval, doc.get_child(key)) << "Unexpected value after assignment";
  if (last_count != get_current_test_part_count())
    cerr << "Key: " << key << endl;
}

TEST(Assign, Load) {
  std::ifstream ifs{"assign_test.txt"};
  ASSERT_FALSE(ifs.fail());
  errorlist err;
  parse_ini(ifs, doc, err);
  ASSERT_TRUE(err.empty());
  ifs.close();

  // Test wrapper functionalities
  EXPECT_FALSE(doc.get_child("nexist"_ts));
  EXPECT_EQ(doc.get_child("nexist"_ts, "fallback"), "fallback");
  EXPECT_THROW(doc.get_child_ref("nexist"_ts), node::base::error);
  EXPECT_EQ(doc.get_child("key-a"_ts, "fallback"), "a");
  EXPECT_EQ(doc.get_child_ref("key-a"_ts).get(), "a");
  EXPECT_EQ(doc.get(), "");
  EXPECT_ANY_THROW(doc.get_child("ref-fail"_ts));
}

TEST(Assign, Ref) {
  // Test local_ref assignments
  set_key("key-a", "a");
  set_key("ref-a", "foo");
  set_key("ref-ref-a", "bar");
  EXPECT_EQ("bar", *doc.get_child("key-a"_ts));

  // Test fallback assignments
  set_key("ref-default-a", "foobar");
  EXPECT_EQ("foobar", *doc.get_child("key-a"_ts));
}

TEST(Assign, File_Env) {
  // Test file_ref assignments
  set_key("ref-nexist", "barfoo");
  set_key("env-nexist", "barbar");
  set_key("file-parse", "foo");
  std::ifstream ifs("key_file.txt");
  string content;
  getline(ifs, content);
  ifs.close();
  EXPECT_EQ("foo", content);

  // Test env_ref assignments
  set_key("env", "foo");
  set_key("file-parse", "content");
}
