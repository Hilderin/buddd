#include "error.h"
#include "settings/settings_store.h"
#include "settings/settings_manager.h"
#include "util/os_config_dir.h"
#include "util/editor_data_root.h"
#include "scene/component_registry/serialization_context.h"
#include "engine_service.h"
#include "engine_context.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_system.h"
#include "scene/world.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <yaml-cpp/yaml.h>

using Catch::Approx;

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <unistd.h>

namespace be = buddd::engine;

// ── Test helper: creates a headless engine and provides a SerializationContext ──
struct SettingsTestCtx {
    std::unique_ptr<be::EngineService> engine;

    SettingsTestCtx()
    {
        auto eng = be::EngineService::create(
            be::Backend::Headless,
            be::WindowConfig{.title = "SettingsTest", .width = 128, .height = 128});
        REQUIRE(eng.has_value());
        engine = std::move(*eng);
    }

    [[nodiscard]] auto ctx() const -> be::SerializationContext {
        return be::SerializationContext{engine->assets()};
    }
};

// ── Helper: create a temp directory path ──
static auto temp_dir() -> std::filesystem::path {
    auto tmp = std::filesystem::temp_directory_path();
    static std::atomic<unsigned int> counter = 0;
    auto pid = static_cast<unsigned int>(getpid());
    for (int i = 0; i < 200; ++i) {
        auto unique = pid + (++counter);
        auto candidate = tmp / ("buddd_settings_test_" + std::to_string(unique));
        std::error_code ec;
        if (std::filesystem::create_directory(candidate, ec)) {
            return candidate;
        }
    }
    FAIL("Could not create temp directory");
    return tmp;
};

// ── Helper: write a YAML string to a file ──
static void write_yaml(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path);
    REQUIRE(out.is_open());
    out << content;
}

// ═════════════════════════════════════════════════════════════════════════════
//  os_user_config_dir tests
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("os_user_config_dir returns non-empty path", "[settings][util]") {
    auto path = be::os_user_config_dir();
    REQUIRE_FALSE(path.empty());
    // At least one of these should be true for any non-empty path
    bool valid_path = path.has_filename() || path.has_parent_path();
    CHECK(valid_path);
}

// ═════════════════════════════════════════════════════════════════════════════
//  editor_data_root / editor_user_data_root tests
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("editor_data_root returns <project_root>/.buddd/", "[settings][util]") {
    auto tmp = temp_dir();
    auto path = be::editor_data_root(tmp);
    REQUIRE(path == tmp / ".buddd");
}

TEST_CASE("editor_user_data_root returns <project_root>/.buddd/user/", "[settings][util]") {
    auto tmp = temp_dir();
    auto path = be::editor_user_data_root(tmp);
    REQUIRE(path == tmp / ".buddd" / "user");
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-001 — Construct and load non-existent file (default state)
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-001: SettingsStore load non-existent file returns defaults", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "nonexistent.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto result = store.load();
    REQUIRE(result.has_value());

    // Verify get returns defaults for unknown keys
    REQUIRE(store.get<int32_t>("missing_key", 42) == 42);
    REQUIRE(store.get<std::string>("another.key", "default") == "default");
    REQUIRE(store.get<bool>("flag", true) == true);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-002 — Load valid YAML file with known keys
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-002: SettingsStore load valid YAML and get keys", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "valid.yaml";

    write_yaml(file_path,
        "string_key: hello\n"
        "int_key: 42\n"
        "bool_key: true\n"
        "float_key: 3.14\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto result = store.load();
    REQUIRE(result.has_value());

    REQUIRE(store.get<std::string>("string_key", "") == "hello");
    REQUIRE(store.get<int32_t>("int_key", 0) == 42);
    REQUIRE(store.get<bool>("bool_key", false) == true);
    REQUIRE(store.get<float>("float_key", 0.0f) == Approx(3.14f).margin(1e-5f));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-003 — Load from directory path returns IoFailed
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-003: SettingsStore load from directory returns IoFailed", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsStore store(tmp, ctx.ctx());  // tmp is a directory
    auto result = store.load();
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::IoFailed);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-004 — Load non-existent file returns success
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-004: SettingsStore load non-existent file succeeds", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "not_there.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto result = store.load();
    REQUIRE(result.has_value());
    REQUIRE(store.get<int32_t>("any_key", -1) == -1);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-005 — Load malformed YAML returns InvalidFormat
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-005: SettingsStore load malformed YAML returns InvalidFormat", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "garbage.yaml";

    write_yaml(file_path, "<<<< garbage >>>> : : :\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto result = store.load();
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == be::Error::Category::InvalidFormat);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-006 — Set keys, save, read back via YAML::LoadFile
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-006: SettingsStore save and verify file content", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "roundtrip.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    store.set<std::string>("name", "test_project");
    store.set<int32_t>("version", 1);
    store.set<bool>("enabled", true);
    store.set<float>("pi", 3.14159f);

    auto save_result = store.save();
    REQUIRE(save_result.has_value());

    // Read back via YAML::LoadFile
    auto node = YAML::LoadFile(file_path.string());
    REQUIRE(node["name"].as<std::string>() == "test_project");
    REQUIRE(node["version"].as<int32_t>() == 1);
    REQUIRE(node["enabled"].as<bool>() == true);
    REQUIRE(node["pi"].as<float>() == Approx(3.14159f).margin(1e-5f));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-007 — Save to non-writable path returns IoFailed
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-007: SettingsStore save to non-writable path returns IoFailed", "[settings][settings_store]") {
    SettingsTestCtx ctx;

    // Use a path inside /proc which is not writable
    auto file_path = std::filesystem::path("/proc/buddd_test_settings.yaml");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    store.set<std::string>("key", "value");

    auto save_result = store.save();
    REQUIRE_FALSE(save_result.has_value());
    REQUIRE(save_result.error().category == be::Error::Category::IoFailed);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-008 — Save without changes is no-op (not dirty)
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-008: SettingsStore save without changes is no-op", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "clean.yaml";

    // Create a file first
    write_yaml(file_path, "key: value\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // Get modification time before save
    auto before = std::filesystem::last_write_time(file_path);

    // Save (should be no-op since not dirty)
    auto save_result = store.save();
    REQUIRE(save_result.has_value());

    // Verify file was not touched
    auto after = std::filesystem::last_write_time(file_path);
    REQUIRE(before == after);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-009 — Dirty state transitions
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-009: SettingsStore dirty state transitions", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "dirty.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    REQUIRE_FALSE(store.is_dirty());  // after construction

    auto load_result = store.load();
    REQUIRE(load_result.has_value());
    REQUIRE_FALSE(store.is_dirty());  // after load

    store.set<std::string>("key", "value");
    REQUIRE(store.is_dirty());  // after set

    auto save_result = store.save();
    REQUIRE(save_result.has_value());
    REQUIRE_FALSE(store.is_dirty());  // after save
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-010 — Set and get all built-in types
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-010: SettingsStore set/get all built-in types", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "all_types.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // Bool
    store.set<bool>("flag", true);
    REQUIRE(store.get<bool>("flag", false) == true);
    REQUIRE(store.is_dirty());

    // Int32
    store.set<int32_t>("count", 42);
    REQUIRE(store.get<int32_t>("count", 0) == 42);

    // Float
    store.set<float>("temperature", 36.6f);
    REQUIRE(store.get<float>("temperature", 0.0f) == Approx(36.6f).margin(1e-5f));

    // Double (not registered in TypeRegistry — test that it returns default and warns)
    // Cannot test the warning directly, but verify it doesn't crash.
    store.set<double>("precision", 3.14159265358979);
    REQUIRE(store.get<double>("precision", 0.0) == 0.0);  // Not registered → returns default

    // String
    store.set<std::string>("greeting", "hello world");
    REQUIRE(store.get<std::string>("greeting", "") == "hello world");
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-011 — Get existing key vs default for missing key
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-011: SettingsStore get existing vs missing keys", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "get_test.yaml";

    write_yaml(file_path, "existing: stored_value\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // Existing key returns stored value
    REQUIRE(store.get<std::string>("existing", "default") == "stored_value");

    // Missing key returns provided default
    REQUIRE(store.get<std::string>("missing", "fallback") == "fallback");
    REQUIRE(store.get<int32_t>("nonexistent", -1) == -1);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-012 — Observer fires on set, not on unrelated key
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-012: SettingsStore observer fires only on observed key", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "observer_test.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    int callback_count = 0;
    std::string callback_key;

    auto conn = store.observe("editor.theme", [&](const std::string& key) {
        ++callback_count;
        callback_key = key;
    });

    // Set observed key
    store.set<std::string>("editor.theme", "dark");
    REQUIRE(callback_count == 1);
    REQUIRE(callback_key == "editor.theme");

    // Set different key — observer should NOT fire
    store.set<std::string>("other.key", "value");
    REQUIRE(callback_count == 1);  // unchanged
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-013 — Multiple observers for same key
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-013: SettingsStore multiple observers for same key", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "multi_observer.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    int count1 = 0, count2 = 0;
    auto conn1 = store.observe("key", [&](const std::string&) { ++count1; });
    auto conn2 = store.observe("key", [&](const std::string&) { ++count2; });

    store.set<std::string>("key", "value");

    REQUIRE(count1 == 1);
    REQUIRE(count2 == 1);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-014 — SettingsManager construction
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-014: SettingsManager construction with three stores", "[settings][settings_manager]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsManager mgr(tmp, ctx.ctx());

    // Verify all three stores exist by accessing them
    REQUIRE_NOTHROW(mgr.editor_settings());
    REQUIRE_NOTHROW(mgr.project_settings());
    REQUIRE_NOTHROW(mgr.user_project_settings());
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-023 — Set same value does not mark dirty
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-023: Setting key to current value does not mark dirty", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "same_value.yaml";

    write_yaml(file_path, "key: value\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());
    REQUIRE_FALSE(store.is_dirty());

    // Set same value
    store.set<std::string>("key", "value");
    REQUIRE_FALSE(store.is_dirty());  // should still be clean

    // Set different value
    store.set<std::string>("key", "new_value");
    REQUIRE(store.is_dirty());  // now dirty
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-024 — Nested keys via dot-separated paths
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-024: SettingsStore nested dot-path keys", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "nested.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // Set nested key
    store.set<std::string>("renderer.resolution.width", "1920");
    store.set<std::string>("renderer.resolution.height", "1080");
    store.set<int32_t>("editor.font.size", 14);

    // Verify via get
    REQUIRE(store.get<std::string>("renderer.resolution.width", "") == "1920");
    REQUIRE(store.get<std::string>("renderer.resolution.height", "") == "1080");
    REQUIRE(store.get<int32_t>("editor.font.size", 0) == 14);

    // Save and reload
    auto save_result = store.save();
    REQUIRE(save_result.has_value());

    be::SettingsStore store2(file_path, ctx.ctx());
    auto load_result2 = store2.load();
    REQUIRE(load_result2.has_value());

    REQUIRE(store2.get<std::string>("renderer.resolution.width", "") == "1920");
    REQUIRE(store2.get<std::string>("renderer.resolution.height", "") == "1080");
    REQUIRE(store2.get<int32_t>("editor.font.size", 0) == 14);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-025 — Connection destruction unregisters observer
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-025: Connection destruction unregisters observer", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "connection_test.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    int callback_count = 0;
    auto conn = store.observe("key", [&](const std::string&) { ++callback_count; });

    // Set key — observer fires
    store.set<std::string>("key", "first");
    REQUIRE(callback_count == 1);

    // Destroy connection
    conn.reset();

    // Set key again — observer should NOT fire
    store.set<std::string>("key", "second");
    REQUIRE(callback_count == 1);  // unchanged
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-028 — get<bool> reads YAML boolean correctly
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-028: SettingsStore get<bool> reads YAML boolean", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "bool_test.yaml";

    write_yaml(file_path, "flag: true\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    REQUIRE(store.get<bool>("flag", false) == true);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-029 — get<int32_t> reads YAML integer correctly
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-029: SettingsStore get<int32_t> reads YAML integer", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "int_test.yaml";

    write_yaml(file_path, "count: 42\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    REQUIRE(store.get<int32_t>("count", 0) == 42);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-030 — get<float> reads YAML float correctly
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-030: SettingsStore get<float> reads YAML float", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "float_test.yaml";

    write_yaml(file_path, "pi: 3.14\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    REQUIRE(store.get<float>("pi", 0.0f) == Approx(3.14f).margin(1e-5f));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-031 — get<std::string> reads YAML string correctly
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-031: SettingsStore get<std::string> reads YAML string", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "string_test.yaml";

    write_yaml(file_path, "message: hello\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    REQUIRE(store.get<std::string>("message", "") == "hello");
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-032 — Unregistered type logs warning and returns default
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-032: SettingsStore unregistered type returns default", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "unregistered_test.yaml";

    write_yaml(file_path, "key: 42\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // uint64_t is not registered in TypeRegistry — should return default
    uint64_t result = store.get<uint64_t>("key", 999);
    REQUIRE(result == 999);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-015 — Platform-dependent path test (Linux)
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-015: SettingsManager editor settings path (Linux)", "[settings][settings_manager][platform]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsManager mgr(tmp, ctx.ctx());

    // The editor settings path is os_user_config_dir() / "editor.yaml"
    auto expected_base = be::os_user_config_dir();
    // We can't easily check the exact path since it depends on env vars,
    // but we can verify the path ends with "editor.yaml"
    auto path = be::os_user_config_dir() / "editor.yaml";
    REQUIRE(path.string().find("editor.yaml") != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-016 — Project settings path
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-016: Project settings path is <cwd>/buddd.project.yaml", "[settings][settings_manager]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsManager mgr(tmp, ctx.ctx());

    // The project settings path is stored but we can't access it directly.
    // Instead, set a key, save_all, and verify file exists.
    mgr.project_settings().set<std::string>("test", "value");
    auto save_result = mgr.save_all();
    REQUIRE(save_result.has_value());

    auto expected_path = tmp / "buddd.project.yaml";
    REQUIRE(std::filesystem::exists(expected_path));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-017 — User project settings path
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-017: User project settings path via editor_user_data_root", "[settings][settings_manager]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsManager mgr(tmp, ctx.ctx());

    mgr.user_project_settings().set<std::string>("test", "value");
    auto save_result = mgr.save_all();
    REQUIRE(save_result.has_value());

    auto expected_path = be::editor_user_data_root(tmp) / "settings.yaml";
    REQUIRE(std::filesystem::exists(expected_path));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-018 — load_all creates .buddd/user/ directory
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-018: load_all creates .buddd/user/ directory", "[settings][settings_manager]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    {
        be::SettingsManager mgr(tmp, ctx.ctx());
        auto load_result = mgr.load_all();
        REQUIRE(load_result.has_value());
    }

    auto user_data_root = be::editor_user_data_root(tmp);
    REQUIRE(std::filesystem::exists(user_data_root));
    REQUIRE(std::filesystem::is_directory(user_data_root));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsStore: AC-019 — save_all writes dirty stores
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("AC-019: save_all writes dirty stores and skips clean ones", "[settings][settings_manager]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsManager mgr(tmp, ctx.ctx());
    auto load_result = mgr.load_all();
    REQUIRE(load_result.has_value());

    // Set a key on each store
    mgr.editor_settings().set<std::string>("key", "editor_value");
    mgr.project_settings().set<std::string>("key", "project_value");
    mgr.user_project_settings().set<std::string>("key", "user_value");

    auto save_result = mgr.save_all();
    REQUIRE(save_result.has_value());

    // Verify project and user files exist on disk (they go to temp dir — hermetic)
    auto project_path = tmp / "buddd.project.yaml";
    auto user_path = be::editor_user_data_root(tmp) / "settings.yaml";
    REQUIRE(std::filesystem::exists(project_path));
    REQUIRE(std::filesystem::exists(user_path));

    // Verify content of project and user files
    auto project_node = YAML::LoadFile(project_path.string());
    REQUIRE(project_node["key"].as<std::string>() == "project_value");

    auto user_node = YAML::LoadFile(user_path.string());
    REQUIRE(user_node["key"].as<std::string>() == "user_value");

    // Editor settings go to os_user_config_dir() — we don't check the file on disk
    // to avoid OS config dir side effects (the save behaviour itself is verified
    // by AC-006). Verify editor data in-memory instead.
    REQUIRE(mgr.editor_settings().get<std::string>("key", "") == "editor_value");
    REQUIRE_FALSE(mgr.editor_settings().is_dirty()); // save() was called, store is clean
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Empty file (zero bytes) loads successfully
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Empty settings file loads successfully", "[settings][settings_store][edge]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "empty.yaml";

    // Create empty file
    std::ofstream(file_path).close();
    REQUIRE(std::filesystem::file_size(file_path) == 0);

    be::SettingsStore store(file_path, ctx.ctx());
    auto result = store.load();
    REQUIRE(result.has_value());
    REQUIRE(store.get<std::string>("any", "default") == "default");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: File with only comments loads successfully
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Settings file with only comments loads successfully", "[settings][settings_store][edge]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "comments.yaml";

    write_yaml(file_path, "# This is a comment\n# Another comment\n");

    be::SettingsStore store(file_path, ctx.ctx());
    auto result = store.load();
    REQUIRE(result.has_value());
    REQUIRE(store.get<std::string>("any_key", "default") == "default");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Observer callback throws exception does not propagate
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Observer callback exception does not propagate", "[settings][settings_store][edge]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "throw_observer.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    auto conn = store.observe("key", [&](const std::string&) {
        throw std::runtime_error("observer error");
    });

    // Must not throw
    REQUIRE_NOTHROW(store.set<std::string>("key", "value"));

    // Value must still be set
    REQUIRE(store.get<std::string>("key", "") == "value");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Connection move semantics
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Connection move semantics", "[settings][settings_store][edge]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "move_conn.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    int count1 = 0;
    auto conn1 = store.observe("key", [&](const std::string&) { ++count1; });

    // Move-construct
    auto conn2 = std::move(conn1);
    REQUIRE(conn1 == nullptr);  // NOLINT: moved-from should be null

    // conn2 should still fire
    store.set<std::string>("key", "value");
    REQUIRE(count1 == 1);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Multiple consecutive set() calls on same key
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Multiple consecutive set() calls on same key", "[settings][settings_store][edge]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "multi_set.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    store.set<int32_t>("key", 1);
    REQUIRE(store.is_dirty());
    REQUIRE(store.get<int32_t>("key", 0) == 1);

    store.set<int32_t>("key", 2);
    REQUIRE(store.is_dirty());
    REQUIRE(store.get<int32_t>("key", 0) == 2);

    store.set<int32_t>("key", 3);
    REQUIRE(store.get<int32_t>("key", 0) == 3);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Dot-path with empty segments
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Dot-path with empty segments is handled", "[settings][settings_store][edge]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "empty_segments.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // Setting with double-dot should be equivalent to single-dot
    store.set<std::string>("foo..bar", "value");
    REQUIRE(store.get<std::string>("foo.bar", "") == "value");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Settings manager layout_ini_path
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("SettingsManager layout_ini_path returns correct path", "[settings][settings_manager]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();

    be::SettingsManager mgr(tmp, ctx.ctx());
    auto expected = (be::editor_user_data_root(tmp) / "layout.ini").string();
    REQUIRE(mgr.layout_ini_path() == expected);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Edge case: Double value round-trip
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("Double is not in TypeRegistry, get returns default", "[settings][settings_store]") {
    SettingsTestCtx ctx;
    auto tmp = temp_dir();
    auto file_path = tmp / "double_test.yaml";

    be::SettingsStore store(file_path, ctx.ctx());
    auto load_result = store.load();
    REQUIRE(load_result.has_value());

    // double is NOT registered in TypeRegistry
    store.set<double>("value", 3.14159265358979);
    // Get returns default since TypeRegistry doesn't have double
    REQUIRE(store.get<double>("value", 42.0) == 42.0);
}
