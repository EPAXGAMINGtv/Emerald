#include "TestFramework.hpp"
#include "TestPaths.hpp"

#include "cfg/ConfigSchema.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace {

cfg::ConfigSchema make_schema() {
    cfg::ConfigSchema schema;
    schema.string("app.name", "Emerald", "Application name");
    schema.integer(
        "server.port",
        8080,
        "Listening port",
        cfg::ConfigSchema::typed<int>(cfg::range<int>(1, 65535))
    );
    schema.real("ui.opacity", 0.9, "Window opacity");
    schema.boolean("ui.visible", true, "Start visible");
    schema.string_list("ui.themes", {"dark", "light"}, "Theme fallback order");
    return schema;
}

} // namespace

TEST(config_schema_definition_and_documentation) {
    cfg::ConfigSchema schema = make_schema();

    CHECK(schema.fields().size() == 5U);
    CHECK(schema.contains("app.name"));
    CHECK(schema.contains("server.port"));
    CHECK(!schema.contains("missing"));

    const cfg::ConfigSchema::Field* port = schema.find("server.port");
    CHECK(port != nullptr);
    CHECK(port != nullptr && port->type == cfg::ConfigSchema::Type::Integer);
    CHECK(port != nullptr && port->description == "Listening port");
    CHECK(port != nullptr && static_cast<bool>(port->validator));

    const std::vector<std::string> keys = schema.keys();
    CHECK(keys.size() == 5U);
    CHECK(keys[0] == "app.name");
    CHECK(keys[1] == "server.port");
    CHECK(keys[4] == "ui.themes");

    const std::string docs = schema.documentation();
    CHECK(docs.find("# Config schema") == 0U);
    CHECK(docs.find("`app.name` (string)") != std::string::npos);
    CHECK(docs.find("`server.port` (integer)") != std::string::npos);
    CHECK(docs.find("`ui.themes` (string-list)") != std::string::npos);
    CHECK(docs.find("default: `8080`") != std::string::npos);
    CHECK(docs.find("Listening port") != std::string::npos);

    cfg::ConfigSchema::Field duplicate;
    duplicate.key = "app.name";
    YAML::Node value;
    value = "other";
    duplicate.default_value = value;
    CHECK(!schema.add(std::move(duplicate)));

    cfg::ConfigSchema::Field empty;
    empty.key = "new.key";
    CHECK(!schema.add(std::move(empty)));
    CHECK(!schema.contains("new.key"));
}

TEST(config_schema_applies_defaults_and_validates) {
    const cfg::ConfigSchema schema = make_schema();
    cfg::Config config;
    schema.apply_to(config);

    CHECK(config.get_string("app.name") == "Emerald");
    CHECK(config.get_int("server.port") == 8080);
    CHECK(config.get_double("ui.opacity") > 0.89);
    CHECK(config.get_bool("ui.visible"));
    CHECK(config.get_string_list("ui.themes").size() == 2U);
    CHECK(schema.validate(config).empty());
    CHECK(config.validate().empty());

    config.set("server.port", 0);
    std::vector<cfg::Issue> issues = schema.validate(config);
    CHECK(issues.size() == 1U);
    CHECK(issues[0].key == "server.port");
    CHECK(!issues[0].message.empty());
    CHECK(!config.validate().empty());

    config.set("ui.visible", std::vector<std::string>{"yes"});
    issues = schema.validate(config);
    CHECK(issues.size() == 2U);
    CHECK(issues[1].key == "ui.visible");
    CHECK(issues[1].message.find("expected boolean") != std::string::npos);

    config.set("server.port", 1234);
    config.set("ui.visible", false);
    CHECK(schema.validate(config).empty());
    CHECK(config.validate().empty());
}

TEST(config_schema_roundtrip_with_overrides_only) {
    test::TempDir dir("config_schema");

    const cfg::ConfigSchema schema = make_schema();
    cfg::Config config(dir.path() / "settings.yaml");
    schema.apply_to(config);
    CHECK(config.save(cfg::SaveMode::Overrides));

    cfg::Config loaded(dir.path() / "settings.yaml");
    schema.apply_to(loaded);
    CHECK(loaded.load());
    CHECK(loaded.get_string("app.name") == "Emerald");
    CHECK(loaded.get_int("server.port") == 8080);
    CHECK(loaded.get_bool("ui.visible"));
    CHECK(schema.validate(loaded).empty());

    loaded.set("server.port", 9000);
    CHECK(loaded.save());
    CHECK(config.load());
    CHECK(config.get_int("server.port") == 9000);
    CHECK(schema.validate(config).empty());
}
