#include "render/shader_program_headless.h"
#include "render/shader_headless.h"

#include <cctype>
#include <vector>

#include "log/log.h"

BUDDD_LOG_TAG("Render:Headless");

namespace buddd::engine {

namespace {

    auto next_generation() -> uint64_t {
        static uint64_t gen = 0;
        return ++gen;
    }

    /// Simulates vertex/fragment shader linking by checking that vertex
    /// outputs match fragment inputs.
    auto simulate_link(const std::string& vs_source, const std::string& fs_source) -> bool {
        auto get_variable_names = [](const std::string& source, bool is_output) -> std::vector<std::string> {
            std::vector<std::string> names;
            std::string keyword = is_output ? "out" : "in";

            size_t pos = 0;
            while (pos < source.size()) {
                auto kw_pos = source.find(keyword, pos);
                if (kw_pos == std::string::npos) break;

                if (kw_pos > 0 && (std::isalnum(static_cast<unsigned char>(source[kw_pos - 1])) || source[kw_pos - 1] == '_')) {
                    pos = kw_pos + keyword.size();
                    continue;
                }

                auto after_kw = kw_pos + keyword.size();
                if (after_kw >= source.size() || (!std::isspace(static_cast<unsigned char>(source[after_kw])) && source[after_kw] != '(')) {
                    pos = after_kw;
                    continue;
                }

                auto scan_pos = after_kw;
                while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) ++scan_pos;
                while (scan_pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[scan_pos])) || source[scan_pos] == '_')) ++scan_pos;
                while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) ++scan_pos;

                std::string name;
                while (scan_pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[scan_pos])) || source[scan_pos] == '_')) {
                    name += source[scan_pos];
                    ++scan_pos;
                }

                if (scan_pos < source.size() && source[scan_pos] == '[') {
                    while (scan_pos < source.size() && source[scan_pos] != ';' && source[scan_pos] != ']') ++scan_pos;
                    if (scan_pos < source.size() && source[scan_pos] == ']') ++scan_pos;
                }

                while (scan_pos < source.size() && std::isspace(static_cast<unsigned char>(source[scan_pos]))) ++scan_pos;

                if (scan_pos < source.size() && (source[scan_pos] == ';' || source[scan_pos] == ',') && !name.empty()) {
                    names.push_back(name);
                }
                pos = scan_pos;
            }
            return names;
        };

        auto vs_outputs = get_variable_names(vs_source, true);
        auto fs_inputs = get_variable_names(fs_source, false);

        if (fs_inputs.empty()) return true;

        for (const auto& vs_out : vs_outputs) {
            for (const auto& fs_in : fs_inputs) {
                if (vs_out == fs_in) return true;
            }
        }

        return false;
    }

} // anonymous namespace

ShaderProgramHeadless::ShaderProgramHeadless(uint64_t generation,
                                             std::string vs_source,
                                             std::string fs_source)
    : handle_(static_cast<uint32_t>(generation) & 0xFFFFFFFFu)
    , generation_(generation)
    , vs_source_(std::move(vs_source))
    , fs_source_(std::move(fs_source)) {}

auto ShaderProgramHeadless::create(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader
) -> Result<std::unique_ptr<ShaderProgramHeadless>>
{
    if (!vertex_shader || !fragment_shader) {
        return make_error(Error::Category::InvalidArgument,
            "Null shader passed to ShaderProgramHeadless::create");
    }

    auto* vs = static_cast<ShaderHeadless*>(vertex_shader.get());
    auto* fs = static_cast<ShaderHeadless*>(fragment_shader.get());

    if (!simulate_link(vs->source(), fs->source())) {
        return make_error(Error::Category::LinkingFailed,
            "Simulated linking error: vertex shader outputs do not match "
            "fragment shader inputs");
    }

    auto gen = next_generation();

    BUDDD_LOG_DEBUG("Shader program created (Headless, gen={})", gen);

    return std::unique_ptr<ShaderProgramHeadless>(
        new ShaderProgramHeadless(gen, vs->source(), fs->source()));
}

auto ShaderProgramHeadless::replace_handle(uint32_t /*new_handle*/) -> void {
    // Increment generation counter (handle_ is derived from generation)
    ++generation_;
    handle_ = static_cast<uint32_t>(generation_);
}

auto ShaderProgramHeadless::release_handle() noexcept -> uint32_t {
    uint32_t old = handle_;
    handle_ = 0;
    generation_ = 0;
    vs_source_.clear();
    fs_source_.clear();
    return old;
}

} // namespace buddd::engine
