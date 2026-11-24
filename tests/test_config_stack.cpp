#include "TestFramework.hpp"
#include "TestPaths.hpp"

#include "cfg/ConfigStack.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

TEST(config_stack_precedence_and_merge) {
    cfg::ConfigStack stack;
    CHECK(stack.size() == 0U);
    CHECK(stack.names().empty());
    CHECK(!stack.contains("missing"));
    CHECK(stack.find("missing") == nullptr);

    cfg::Config& base = stack.add("base");
    base.set("app.name", std::string("Base"));
    base.set("server.host", std::string("localhost"));
    base.set("server.port", 1000);
    base.set("features", std::vector<std::string>{"alpha", "beta"});

    cfg::Config& user = stack.add("user");
    user.set("app.name", std::string("User"));
    user.set("server.port", 2000);

    cfg::Config& cli = stack.add("cli");
    cli.set("server.port", 3000);

    CHECK(stack.size() == 3U);
    CHECK(stack.contains("user"));
    CHECK(stack.find("base") == &base);
    CHECK(stack.get("user").get_string("app.name") == "User");
    CHECK(stack.names()[0] == "base");
    CHECK(stack.names()[2] == "cli");

    cfg::Config& same = stack.add("user");
    CHECK(&same == &user);
    CHECK(stack.size() == 3U);

    const cfg::Config effective = stack.effective();
    CHECK(effective.get_string("app.name") == "User");
    CHECK(effective.get_string("server.host") == "localhost");
    CHECK(effective.get_int("server.port") == 3000);
    CHECK(effective.get_string_list("features").size() == 2U);
    CHECK(effective.get_string_list("features")[0] == "alpha");
    CHECK(effective.get_string_list("features")[1] == "beta");

    CHECK(base.get_string("app.name") == "Base");
    CHECK(user.get_int("server.port") == 2000);
    CHECK(cli.get_int("server.port") == 3000);

    CHECK(stack.remove("cli"));
    CHECK(!stack.contains("cli"));
    CHECK(stack.effective().get_int("server.port") == 2000);
    CHECK(!stack.remove("cli"));

    bool threw = false;
    try {
        (void)stack.get("missing");
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    stack.clear();
    CHECK(stack.size() == 0U);
    CHECK(stack.names().empty());
    CHECK(stack.effective().root().IsMap());
}

TEST(config_stack_file_layers) {
    test::TempDir dir("config_stack");

    cfg::Config base(dir.path() / "base.yaml");
    base.set("app.name", std::string("Base"));
    base.set("server.port", 8080);
    CHECK(base.save(cfg::SaveMode::Overrides));

    cfg::Config user(dir.path() / "user.yaml");
    user.set("server.port", 9090);
    CHECK(user.save(cfg::SaveMode::Overrides));

    cfg::ConfigStack stack;
    stack.add("base", dir.path() / "base.yaml");
    stack.add("user", dir.path() / "user.yaml");
    CHECK(stack.load_all());

    cfg::Config effective = stack.effective();
    CHECK(effective.get_string("app.name") == "Base");
    CHECK(effective.get_int("server.port") == 9090);
    CHECK(stack.save_all());

    effective.set("server.port", 1);
    CHECK(stack.get("user").get_int("server.port") == 9090);

    stack.get("user").set("server.port", 9999);
    CHECK(stack.save_layer("user", cfg::SaveMode::Overrides));

    cfg::Config reloaded(dir.path() / "user.yaml");
    CHECK(reloaded.load());
    CHECK(reloaded.get_int("server.port") == 9999);
    CHECK(!stack.save_layer("missing"));

    cfg::ConfigStack broken;
    broken.add("broken", dir.path() / "does-not-exist.yaml");
    CHECK(!broken.load_all());
}
