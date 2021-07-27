#include <shaderc/shaderc.hpp>

#include "importers.hpp"

struct pragma_info {
    u64_t offset;
    std::string string;
};

// TODO : rewrite to not span multiple asset files, pack in one

// naive preprocessor
static std::pair<pragma_info, bool> find_pragma(std::ifstream& file) {
    std::string line_buffer;
    u64_t offset = 0;

    while (std::getline(file, line_buffer)) {
        std::stringstream line_ss{ line_buffer };
        std::string word_buffer;

        line_ss >> word_buffer; // 1 word, rest ignored
        if (word_buffer == "#pragma") {
            line_ss >> word_buffer;
            return { pragma_info{ offset, std::move(word_buffer) }, true };
        }
        else {
            offset += line_buffer.size() + 1; // 1 for a linebreak
        }
    }
    return { pragma_info{ offset }, false };
}

parsed_file parse_shader(const fs::path& path) {
    constexpr std::array pragma_shader_type{
        "vertex",
        "fragment"
    }; // NOTE : be aware of the order, should be compatible with shaderc_shader_kind

    std::ifstream glsl_src{ path, std::ios_base::ate | std::ios_base::in };
    const size_t end_pos = glsl_src.tellg();
    glsl_src.seekg(0, std::ios_base::beg);

    struct shader_unit {
        size_t offset;
        size_t length;
        shaderc_shader_kind kind;
    };
    std::vector<shader_unit> shader_units;

    size_t current_length = 0; // need for not breaking on unknown pragmas
    while (true) {
        const std::pair<pragma_info, bool> current_pragma = find_pragma(glsl_src);
        current_length += current_pragma.first.offset;
        if (!current_pragma.second) {
            break;
        }

        const auto pragma_it = std::find(pragma_shader_type.begin(), pragma_shader_type.end(), current_pragma.first.string);
        if (pragma_it == pragma_shader_type.end()) {
            continue; // don't error on pragmas
        }

        const shaderc_shader_kind shader_type = static_cast<shaderc_shader_kind>(pragma_it - pragma_shader_type.begin());
        shader_units.emplace_back(glsl_src.tellg(), 0, shader_type);

        if (shader_units.size() != 1) {
            // set length of previous unit
            auto& prev_unit = *(shader_units.end() - 2);
            prev_unit.length = current_length;
        }
        current_length = 0;
    }

    if (shader_units.size() != 0) {
        shader_units.back().length = current_length; // set last length
    }
    glsl_src.clear();

    std::vector<char> unit_src;
    std::array<u32_t, pragma_shader_type.size()> shader_type_counts{ 0 };
    const auto name = path.stem().string();

    parsed_file ret_file;
    for (const auto& unit : shader_units) {
        std::string unit_name = 
            name + '.' + std::to_string(shader_type_counts[unit.kind]) +
            '.' + pragma_shader_type[unit.kind];

        shader_type_counts[unit.kind] += 1;

        unit_src.resize(unit.length);
        glsl_src.seekg(unit.offset, std::ios_base::beg);
        glsl_src.read(unit_src.data(), unit_src.size());
        unit_src.back() = '\n'; // on LF writes LF, on CRLF do an additional linebreak

        shaderc::Compiler compiler;
        const auto unit_result = compiler.CompileGlslToSpv(unit_src.data(), unit_src.size(), unit.kind, unit_name.data());
        if (unit_result.GetCompilationStatus() != shaderc_compilation_status_success) {
            throw std::runtime_error{
                std::string{ "Failed to compile " } + pragma_shader_type[unit.kind] + " shader unit in "
                + path.string() + ", error:" + unit_result.GetErrorMessage()
            };
        }

        byte_vector unit_bin{ reinterpret_cast<const std::byte*>(unit_result.begin()), reinterpret_cast<const std::byte*>(unit_result.end()) };
        ret_file.emplace_back(std::move(unit_bin), std::move(unit_name));
    }
    ret_file.emplace_back(byte_vector{}, name);

    return ret_file;
}