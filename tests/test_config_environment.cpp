#include "TestFramework.hpp"

#include "cfg/ConfigEnvironment.hpp"

#include <cstdlib>
#include <string>
#include <vector>

namespace {

struct EnvGuard {
    std::vector<std::string> names;

    ~EnvGuard() {
        for (const std::string& name : names) {
            ::unsetenv(name.c_str());
        }
    }
};

void set_var(EnvGuard& guard, const char* name, const char* value) {
    guard.names.emplace_back(name);
    ::setenv(name, value, 1);
}

} // namespace

TEST(config_environment_typed_overrides) {
    EnvGuard guard;
    set_var(guard, "EMERALD_CFG_TEST_NAME", "  Emerald Env  ");
    set_var(guard, "EMERALD_CFG_TEST_PORT", " 4711 ");
    set_var(guard, "EMERALD_CFG_TEST_OPACITY", "0.75");
    set_var(guard, "EMERALD_CFG_TEST_VISIBLE", "YES");
    set_var(guard, "EMERALD_CFG_TEST_THEMES", " dark , light , high ");

    cfg::ConfigEnvironment env;
    env.bind("EMERALD_CFG_TEST_NAME", "app.name");
    env.bind(
        "EMERALD_CFG_TEST_PORT",
        "server.port",
        cfg::ConfigEnvironment::Type::Integer
    );
    env.bind(
        "EMERALD_CFG_TEST_OPACITY",
        "ui.opacity",
        cfg::ConfigEnvironment::Type::Real
    );
    env.bind(
        "EMERALD_CFG_TEST_VISIBLE",
        "ui.visible",
        cfg::ConfigEnvironment::Type::Boolean
    );
    env.bind(
        "EMERALD_CFG_TEST_THEMES",
        "ui.themes",
        cfg::ConfigEnvironment::Type::StringList
    );

    CHECK(env.size() == 5U);
    CHECK(env.contains("EMERALD_CFG_TEST_NAME"));
    CHECK(!env.contains("EMERALD_CFG_TEST_MISSING"));
    CHECK(env.variables()[1] == "EMERALD_CFG_TEST_PORT");
    CHECK(env.bindings()[1].key == "server.port");
    CHECK(env.find("EMERALD_CFG_TEST_THEMES") != nullptr);

    cfg::Config config;
    config.set_default("app.name", std::string("Default"));
    config.set_default("server.port", 80);
    CHECK(env.apply(config));

    CHECK(config.get_string("app.name") == "Emerald Env");
    CHECK(config.get_int("server.port") == 4711);
    CHECK(config.get_double("ui.opacity") > 0.74);
    CHECK(config.get_double("ui.opacity") < 0.76);
    CHECK(config.get_bool("ui.visible"));
    CHECK(config.get_string_list("ui.themes").size() == 3U);
    CHECK(config.get_string_list("ui.themes")[0] == "dark");
    CHECK(config.get_string_list("ui.themes")[1] == "light");
    CHECK(config.get_string_list("ui.themes")[2] == "high");

    env.bind("EMERALD_CFG_TEST_NAME", "renamed.key");
    CHECK(env.size() == 5U);
    CHECK(env.find("EMERALD_CFG_TEST_NAME")->key == "renamed.key");
    CHECK(env.apply(config));
    CHECK(config.get_string("renamed.key") == "Emerald Env");

    CHECK(env.remove("EMERALD_CFG_TEST_NAME"));
    CHECK(!env.remove("EMERALD_CFG_TEST_NAME"));
    CHECK(env.size() == 4U);
}

TEST(config_environment_missing_invalid_and_bool_parser) {
    EnvGuard guard;

    bool value = false;
    CHECK(cfg::ConfigEnvironment::parse_bool("true", value));
    CHECK(value);
    CHECK(cfg::ConfigEnvironment::parse_bool("OFF", value));
    CHECK(!value);
    CHECK(cfg::ConfigEnvironment::parse_bool("  yes ", value));
    CHECK(value);
    CHECK(!cfg::ConfigEnvironment::parse_bool("maybe", value));
    CHECK(value);

    cfg::ConfigEnvironment env;
    env.bind("EMERALD_CFG_TEST_MISSING", "missing", cfg::ConfigEnvironment::Type::Integer);
    cfg::Config config;
    config.set_default("missing", 7);
    CHECK(env.apply(config));
    CHECK(config.get_int("missing") == 7);

    set_var(guard, "EMERALD_CFG_TEST_BAD_INT", "12x");
    env.bind(
        "EMERALD_CFG_TEST_BAD_INT",
        "bad_int",
        cfg::ConfigEnvironment::Type::Integer
    );
    config.set_default("bad_int", 5);
    CHECK(!env.apply(config));
    CHECK(config.get_int("bad_int") == 5);

    set_var(guard, "EMERALD_CFG_TEST_BAD_REAL", "oops");
    env.bind(
        "EMERALD_CFG_TEST_BAD_REAL",
        "bad_real",
        cfg::ConfigEnvironment::Type::Real
    );
    config.set_default("bad_real", 1.5);
    CHECK(!env.apply(config));
    CHECK(config.get_double("bad_real") > 1.49);

    set_var(guard, "EMERALD_CFG_TEST_BAD_BOOL", "maybe");
    env.bind(
        "EMERALD_CFG_TEST_BAD_BOOL",
        "bad_bool",
        cfg::ConfigEnvironment::Type::Boolean
    );
    config.set_default("bad_bool", true);
    CHECK(!env.apply(config));
    CHECK(config.get_bool("bad_bool"));

    env.clear();
    CHECK(env.size() == 0U);
    CHECK(env.bindings().empty());
    CHECK(env.variables().empty());
}
