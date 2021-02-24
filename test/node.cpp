#include "test.h"

#include <fstream>

struct parse_test_single {
  string path, value, parsed;
  bool fail, exception;
};
using parse_test = vector<parse_test_single>;

void test_nodes(parse_test testset) {
  auto fail_count = get_current_test_part_count();
  node::base_p base_doc = std::make_unique<node::wrapper>();

  // Add keys
  for(auto test : testset) {
    auto& doc = dynamic_cast<node::wrapper&>(*base_doc);
    auto last_count = get_current_test_part_count();
    if (test.fail)
      EXPECT_ANY_THROW(doc.add(test.path, move(test.value))) << "Expected error";
    else
      EXPECT_NO_THROW(doc.add(test.path, move(test.value))) << "Unexpected error";
    if (last_count != get_current_test_part_count())
      cerr << "Key: " << test.path << endl;
  }
  auto test_doc = [&] {
    auto& doc = dynamic_cast<const node::wrapper&>(*base_doc);
    for(auto test : testset) {
      // Skip key if it is expected to fail in the previous step
      if (test.fail) continue;
      check_key(doc, test.path, test.parsed, test.exception);
    }
  };
  test_doc();

  if (fail_count != get_current_test_part_count())
    GTEST_SKIP() << "Skipping clone test";
  base_doc = clone(base_doc);
  test_doc();
}

TEST(Node, Simple) {
  test_nodes({
    {".key", "foo", "foo"},
    {"a.ref", "${.key}", "foo"},
    {"a.ref-space", "${ .key }", "foo"},
  });
}

TEST(Node, Ref) {
  test_nodes({
    {"test2.ref-file-default-before", "${file nexist.txt ? ${test.ref-ref-a}}", "a"},
    {"test2.ref-before", "${test2.ref-a}", "a"},
    {"test.key-a", "a", "a"},
    {"test2.ref-a", "${test.key-a}", "a"},
    {"test2.ref-default-a", "${test.key-nexist?${test.key-a}}", "a"},
    {"test2.ref-file-default", "${file nexist.txt ? ${test.key-a}}", "a"},
    {"test.ref-ref-a", "${test2.ref-a?failed}", "a"},
    {"test2.ref-fallback-a", "${ test.key-a ? fail }", "a"},
    {"test2.ref-nexist", "${test.key-nexist? \" f a i l ' }", "\" f a i l '"},
    {"test2.ref-fail", "${test.key-fail}", "${test.key-fail}", false, true},
    {"test2.ref-fake", "{test.key-a}", "{test.key-a}"},
    {"test2.interpolation", "This is ${test.key-a} test", "This is a test"},
    {"test2.interpolation-trick", "$ ${test.key-a}", "$ a"},
    {"test2.interpolation-trick-2", "} ${test.key-a}", "} a"},
    {"test2.escape", "\\${test.key-a}", "${test.key-a}"},
    {"test2.not-escape", "\\$${test.key-a}", "\\$a"},
  });
  //  {
  //    {"", "ref-cyclic-1", "${.ref-cyclic-2}", "${ref-cyclic-1}"},
  //    {"", "ref-cyclic-2", "${.ref-cyclic-1}", "${ref-cyclic-1}", true},
  //  },
  //  {
  //    {"", "ref-cyclic-1", "${.ref-cyclic-2}", "${ref-cyclic-1}"},
  //    {"", "ref-cyclic-2", "${.ref-cyclic-3}", "${ref-cyclic-1}"},
  //    {"", "ref-cyclic-3", "${.ref-cyclic-1}", "${ref-cyclic-1}", true},
  //    {"", "ref-not-cyclic-1", "${.ref-not-cyclic-2}", ""},
  //    {"", "ref-not-cyclic-2", "", ""}
  //  },
}
TEST(Node, File) {
  test_nodes({
    {".ext", "txt", "txt"},
    {".file", "${file key_file.${.ext} ? fail}", "content"},
    {".file-fail", "${file nexist.${.ext} ? Can't find ${.ext} file}", "Can't find txt file"},
  });
  test_nodes({{".file1", "${file key_file.txt }", "content"}});
  test_nodes({{".file2", "${file key_file.txt?fail}", "content"}});
  test_nodes({{".file3", "${file nexist.txt ? ${file key_file.txt}}", "content"}});
  test_nodes({{".file4", "${file nexist.txt ? \" f a i l ' }", "\" f a i l '"}});
  test_nodes({{".file5", "${file nexist.txt}", "${file nexist.txt}", false, true}});
}

TEST(Node, Cmd) {
  test_nodes({
    {".msg", "foo bar", "foo bar"},
    {".cmd", "${cmd echo ${.msg}}", "foo bar"},
  });
  test_nodes({{".cmd", "${cmd echo hello world}", "hello world"}});
  test_nodes({{".cmd", "${cmd nexist}", "", false, true}});
}

TEST(Node, Color) {
  test_nodes({
    {".color", "${color #123456 }", "#123456"},
    {".color-fallback", "${color nexist(1) ? #ffffff }", "#ffffff"},
    {".color-hsv", "${color hsv(180, 1, 0.75)}", "#00BFBF"},
    {".color-ref", "${color ${.color}}", "#123456"},
    {".color-mod", "${color cielch 'lum * 1.5, hue + 60' ${.color}}", "#633E5C"},
  });
}

TEST(Node, Other) {
  setenv("test_env", "test_env", true);
  unsetenv("nexist");

  test_nodes({{".interpolate", "%{${color hsv(0, 1, 0.5)}}", "%{#800000}"}});
  test_nodes({{".dumb", "${dumb nexist.txt}", "${dumb nexist.txt}", true}});
  test_nodes({{".dumb", "", ""}});
  test_nodes({{".env", "${env test_env? fail}", "test_env"}});
  test_nodes({{".env", "${env nexist? \" f a i l \" }", " f a i l "}});
  test_nodes({{".map", "${map 5:10 0:2 7.5}", "1.000000"}});
  test_nodes({{".map", "${map 5:10 2 7.5}", "1.000000"}});
  test_nodes({
    {".clone_source", "${color #123456 }", "#123456"},
    {".clone_source.lv1", "", ""},
    {".clone_source.lv1.lv2", "", ""},
    {".clone", "${clone .clone_source }", "#123456"},
    {".clone.lv1", "", ""},
    {".clone.lv1.lv2", "", "", true},
    {".clone-fail", "${clone .nexist }", "", true},
  });
}
