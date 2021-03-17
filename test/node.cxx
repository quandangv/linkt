#include "test.hxx"

#include <fstream>

struct parse_test_single {
  string path, value, parsed;
  bool is_fixed{true}, fail{false}, exception{false}, clone_fail{false};
};
using parse_test = vector<parse_test_single>;

void test_nodes(parse_test testset, int repeat = base_repeat) {
  auto doc = std::make_shared<node::wrapper>();

  node::parse_context context;
  context.parent = doc;
  context.parent_based_ref = true;
  // Add keys to doc
  for(auto test : testset) {
    auto last_count = get_test_part_count();
    context.raw = test.value;
    tstring ts(context.raw);
    try {
      doc->add(test.path, context, ts);
    } catch (const std::exception& e) {
      EXPECT_TRUE(test.fail);
    }
    if (last_count != get_test_part_count())
      cerr << "Key: " << test.path << endl;
  }
  auto test_doc = [&](node::base_s node, node::errorlist& errs) {
    auto doc = std::dynamic_pointer_cast<node::wrapper>(node);
    for (auto test : testset) {
      // Skip key if it is expected to fail in the previous step
      if (test.fail || test.clone_fail) {
        std::erase_if(errs, [&](auto pair) { return pair.first == test.path; });
      } else {
        check_key(*doc, test.path, test.parsed, test.exception, test.is_fixed);
      }
    }
  };
  triple_node_test(doc, test_doc, repeat);
}

TEST(Node, Simple) {
  test_nodes({
    {"key", "foo", "foo"},
    {"a.ref", "${key}", "foo"},
    {"a.ref-space", "${ key }", "foo"},
    {"newline", "hello\\nworld", "hello\nworld"},
  });
}

TEST(Node, Cmd) {
  test_nodes({
    {"msg", "1.000", "1.000"},
    {"cmd", "${cmd echo ${msg}}", "1.000", false},
    {"cmd-ref", "${map 1 2 ${cmd}}", "2", false},
    {"cmd-msg", "result is ${cmd-ref}", "result is 2", false},
  }, base_repeat / 100);
  test_nodes({{"cmd1", "${cmd echo 'hello  world'}", "hello  world", false}}, base_repeat / 100);
  test_nodes({{"cmd2", "${cmd echo hello world}", "hello world", false}}, base_repeat / 100);
  test_nodes({{"cmd3", "${cmd nexist}", "", false, false, true}}, base_repeat / 100);
  test_nodes({{"cmd4", "${cmd nexist ? fail}", "fail", false}}, base_repeat / 100);
  test_nodes({{"cmd4", "${cmd nexist ?}", "", false}}, base_repeat / 100);
  test_nodes({
    {"greeting", "${cmd echo Hello ${rel name}}", "Hello quan", false},
    {"greeting.name", "quan", "quan"},
  }, base_repeat / 100);
}

TEST(Node, Ref) {
  test_nodes({
    {"test2.ref-fake", "{test.key-a}", "{test.key-a}"},
    {"test2.ref-file-default-before", "${file nexist.txt ? ${test3.ref-ref-a}}", "a", false},
    {"test2.ref-before", "${test2.ref-a}", "a"},
    {"test.key-a", "a", "a"},
    {"test2.ref-a", "${test.key-a}", "a"},
    {"test3.ref-ref-a", "${test2.ref-a?failed}", "a"},
    {"test.ref-ref-a", "${test2.ref-a?failed}", "a"},
    {"test2.ref-default-a", "${test.key-nexist?${test.key-a}}", "a", false},
    {"test2.ref-file-default", "${file nexist.txt ? ${test.key-a}}", "a", false},
    {"test2.ref-nexist", "${test.key-nexist? \" f a i l ' }", "\" f a i l '", false},
    {"test2.ref-fail", "${test.key-fail}", "${test.key-fail}", true, false, true, true},
    {"test2.interpolation", "This is ${test.key-a} test", "This is a test"},
    {"test2.interpolation2", "$ ${test.key-a}", "$ a"},
    {"test2.interpolation3", "} ${test.key-a}", "} a"},
    {"test2.escape", "\\${test.key-a}", "${test.key-a}"},
    {"test2.not-escape", "\\$${test.key-a}", "\\$a"},
  });
  test_nodes({
    {"ref-cyclic-1", "${.ref-cyclic-2}", "${ref-cyclic-1}", true, false, true, true},
    {"ref-cyclic-2", "${.ref-cyclic-1}", "${ref-cyclic-1}", true, false, true, true},
  });
  test_nodes({
    {"ref-cyclic-1", "${.ref-cyclic-2}", "${ref-cyclic-1}", true, false, true, true},
    {"ref-cyclic-2", "${.ref-cyclic-3}", "${ref-cyclic-1}", true, false, true, true},
    {"ref-cyclic-3", "${.ref-cyclic-1}", "${ref-cyclic-1}", true, false, true, true},
    {"ref-not-cyclic-1", "${ref-not-cyclic-2}", ""},
    {"ref-not-cyclic-2", "", ""}
  });
  test_nodes({{"dep", "${dep fail fail2}", "", false, true}});
  test_nodes({{"dep", "${rel fail fail2}", "", false, true}});
}
TEST(Node, File) {
  test_nodes({
    {"ext", "txt", "txt"},
    {"file", "${file key_file.${ext} ? fail}", "content", false},
    {"file-fail", "${file nexist.${ext} ? Can't find ${ext} file}", "Can't find txt file", false},
  });
  test_nodes({{"file1", "${file key_file.txt }", "content", false}});
  test_nodes({{"file2", "${file key_file.txt?fail}", "content", false}});
  test_nodes({{"file3", "${file nexist.txt ?   ${file key_file.txt}}", "content", false}});
  test_nodes({{"file4", "${file nexist.txt ? \" f a i l ' }", "\" f a i l '", false}});
  test_nodes({{"file5", "${file nexist.txt}", "${file nexist.txt}", false, false, true}});
}

TEST(Node, Color) {
  test_nodes({
    {"color", "${color #123456 }", "#123456"},
    {"color-fallback", "${color nexist(1) ? #ffffff }", "#ffffff"},
    {"color-hsv", "${color hsv(180, 1, 0.75)}", "#00BFBF"},
    {"color-ref", "${color ${color}}", "#123456"},
    {"color-mod", "${color cielch 'lum * 1.5, hue + 60' ${color}}", "#633E5C"},
  });
  test_nodes({
    {"clone5.stat", "60", "60"},
    {"color-fail", "${color ${clone clone5}}", "", false, true},
  });
}

TEST(Node, Clone) {
  test_nodes({
    {"clone_source", "${color #123456 }", "#123456"},
    {"clone_source.lv1", "abc", "abc"},
    {"clone_source.lv1.lv2", "abc", "abc"},
    {"clone_source.dumb", "${nexist}", "", true, false, true, true},
    {"clone", "${clone clone_source }", "#123456"},
    {"clone.lv1", "def", "def", false, true},
    {"clone.lv1.lv2", "def", "def", false, true},
    {"clone.lv1.dumb", "abc", "abc"},
    {"clone_merge", "${clone clone_source clone}", "#123456"},
    {"clone_merge.lv1.dumb", "def", "def", false, true},
  });
  test_nodes({{"clone2", "${clone nexist nexist2 }", "", false, true}});
  test_nodes({{"clone3", "${clone nexist}", "", false, true}});
  test_nodes({
    {"base", "${map 100 1 ${rel stat}}", "", true, false, true, true},
    {"clone4.stat", "60", "60"},
    {"clone4", "${clone base}", "0.6"},
    {"clone4.stat", "60", "60", false, true},
  });
  test_nodes({
    {"src1.key1", "a", "a"},
    {"src2.key2", "b", "b"},
    {"src3", "c", "c"},
    {"merge", "${clone src1 src2 src3}", "c"},
    {"merge-fail", "${clone src3 src2 src1}", "", false, true},
  });
}

TEST(Node, Other) {
  setenv("test_env", "test_env", true);
  unsetenv("nexist");
  test_nodes({{"interpolate", "%{${color hsv(0, 1, 0.5)}}", "%{#800000}"}});
  test_nodes({{"dumb1", "${dumb nexist.txt}", "${dumb nexist.txt}", false, true}});
  test_nodes({{"dumb2", "", ""}});
  test_nodes({{"dumb3", "${}", "", false, true}});
  test_nodes({{"env0", "${env 'test_env' ? fail}", "test_env", false}});
  test_nodes({{"env1", "${env ${nexist ? test_env}}", "test_env", false}});
  test_nodes({{"env2", "${env nexist? \" f a i l \" }", " f a i l ", false}});
  test_nodes({{"env3", "${env nexist test_env }", "", false, true}});
  test_nodes({{"map", "${map 5:10 0:2 7.5}", "1"}});
  test_nodes({{"map", "${map 5:10 2 7.5 ? -1}", "1"}});
  test_nodes({{"map", "${map 5:10 7.5}", "1", false, true}});
  test_nodes({
    {"source", "60", "60"},
    {"cache", "${cache ${source} hello}", "hello", false},
  });
}
