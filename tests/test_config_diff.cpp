#include "TestFramework.hpp"

#include "cfg/ConfigDiff.hpp"

#include <string>
#include <vector>

TEST(config_diff_added_removed_and_changed) {
    cfg::Config from;
    from.set("app.name", std::string("Old"));
    from.set("server.host", std::string("localhost"));
    from.set("server.port", 80);
    from.set("server.tls", false);
    from.set("removed", 1);

    cfg::Config to;
    to.set("app.name", std::string("New"));
    to.set("server.host", std::string("localhost"));
    to.set("server.port", 443);
    to.set("server.tls", false);
    to.set("server.added", true);

    const std::vector<cfg::ConfigDiff::Change> changes =
        cfg::ConfigDiff::compare(from, to);
    CHECK(changes.size() == 4U);
    CHECK(changes[0].kind == cfg::ConfigDiff::Change::Kind::Changed);
    CHECK(changes[0].key == "app.name");
    CHECK(changes[1].kind == cfg::ConfigDiff::Change::Kind::Removed);
    CHECK(changes[1].key == "removed");
    CHECK(changes[2].kind == cfg::ConfigDiff::Change::Kind::Added);
    CHECK(changes[2].key == "server.added");
    CHECK(changes[3].kind == cfg::ConfigDiff::Change::Kind::Changed);
    CHECK(changes[3].key == "server.port");

    const std::string text = cfg::ConfigDiff::to_string(changes);
    CHECK(text.find("~ app.name") != std::string::npos);
    CHECK(text.find("- removed") != std::string::npos);
    CHECK(text.find("+ server.added") != std::string::npos);
    CHECK(text.find("~ server.port") != std::string::npos);
    CHECK(text.find('\n') != std::string::npos);

    CHECK(cfg::ConfigDiff::equal(from, from));
    CHECK(!cfg::ConfigDiff::equal(from, to));
    CHECK(cfg::ConfigDiff::compare(from, from).empty());
}

TEST(config_diff_yaml_nodes_and_prefix) {
    const YAML::Node before = YAML::Load(
        "host: localhost\nport: 1\nold: yes\n"
    );
    const YAML::Node after = YAML::Load(
        "host: localhost\nport: 2\nnew: true\n"
    );

    const std::vector<cfg::ConfigDiff::Change> changes =
        cfg::ConfigDiff::compare(before, after, "server");
    CHECK(changes.size() == 3U);
    CHECK(changes[0].key == "server.new");
    CHECK(changes[0].kind == cfg::ConfigDiff::Change::Kind::Added);
    CHECK(changes[1].key == "server.old");
    CHECK(changes[1].kind == cfg::ConfigDiff::Change::Kind::Removed);
    CHECK(changes[2].key == "server.port");
    CHECK(changes[2].kind == cfg::ConfigDiff::Change::Kind::Changed);
}

TEST(config_diff_sequences_are_whole_values) {
    cfg::Config from;
    from.set("themes", std::vector<std::string>{"dark", "light"});
    cfg::Config to;
    to.set("themes", std::vector<std::string>{"dark", "solar"});

    const std::vector<cfg::ConfigDiff::Change> changes =
        cfg::ConfigDiff::compare(from, to);
    CHECK(changes.size() == 1U);
    CHECK(changes[0].key == "themes");
    CHECK(changes[0].kind == cfg::ConfigDiff::Change::Kind::Changed);
    CHECK(changes[0].before.IsSequence());
    CHECK(changes[0].after.IsSequence());

    cfg::Config same;
    same.set("themes", std::vector<std::string>{"dark", "light"});
    CHECK(cfg::ConfigDiff::equal(from, same));
}
