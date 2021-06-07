#include <iostream>
#include <array>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <span>

#define ZIP_STATIC
#include <zip.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#include <shaderc/shaderc.hpp>

namespace fs = std::filesystem;

static std::vector<std::vector<std::byte>> zip_buffers;

static void add_buffer_to_archive(zip_t* archive, std::string_view name, void* data, std::uint64_t size) {
    auto src_buffer = zip_source_buffer(archive, data, size, 0);
    if (src_buffer == nullptr) {
        throw std::runtime_error{ std::string{ "Failed to add " } + name.data() + " to archive: " + zip_strerror(archive) };
    }

    if (zip_file_add(archive, name.data(), src_buffer, ZIP_FL_ENC_GUESS) < 0) {
        zip_source_free(src_buffer);
        throw std::runtime_error{ std::string{ "Failed to add " } + name.data() + " to archive: " + zip_strerror(archive) };
    }
    std::cout << name << " added to archive\n";
}

static void add_file_to_archive(zip_t* archive, const fs::path& path, const char* extension) {
    auto src_file = zip_source_file(archive, path.string().c_str(), 0, 0);
    if (src_file == nullptr) {
        throw std::runtime_error{ "Failed to add " + path.string() + " to archive: " + zip_strerror(archive) };
    }

    if (zip_file_add(archive, std::string{ path.stem().string() + '.' + extension }.c_str(), src_file, ZIP_FL_ENC_GUESS) < 0) {
        zip_source_free(src_file);
        throw std::runtime_error{ "Failed to add " + path.string() + " to archive: " + zip_strerror(archive) };
    }

    std::cout << path.filename().string() << " added to archive: " << extension << '\n';
}

static bool probe_image(zip_t* archive, const fs::path& path) {
    // NOTE : not looking for extensions, let stbi figure it out
    int w = 0, h = 0, ch = 0;
    // check if it loads
    if (!stbi_info(path.string().c_str(), &w, &h, &ch)) {
        return false;
    }

    add_file_to_archive(archive, path, "texture");
    return true;
}

static bool probe_shader(zip_t* archive, const fs::path& path) {
    constexpr std::array<const char*, 2> pragma_shader_type{
        "vertex",
        "fragment"
    }; // NOTE : be aware of the order, should be compatible with shaderc_shader_kind

    if (path.extension() != ".glsl") {
        return false;
    }

    // naive preprocessor looking for pragmas
    struct shader_unit {
        std::size_t offset;
        std::size_t length;
        shaderc_shader_kind kind;
    };

    std::ifstream glsl_src{ path, std::ios_base::ate | std::ios_base::in };
    const std::size_t end_pos = glsl_src.tellg();
    glsl_src.seekg(0, std::ios_base::beg);

    std::string line_buffer;
    std::vector<shader_unit> shader_units;
    std::size_t unit_len = 0;

    while (std::getline(glsl_src, line_buffer)) {
        std::stringstream line_ss{ line_buffer };
        std::string word_buffer;

        line_ss >> word_buffer;
        if (word_buffer == "#pragma") {
            line_ss >> word_buffer;

            for (auto i = 0u; i < pragma_shader_type.size(); ++i) {
                if (word_buffer == pragma_shader_type[i]) {
                    // abuse int to enum cast
                    shader_units.emplace_back(
                        glsl_src.tellg(),
                        0,
                        static_cast<shaderc_shader_kind>(i)
                    );
                    if (shader_units.size() != 1) {
                        auto& prev_unit = *(shader_units.begin() + (shader_units.size() - 2));
                        prev_unit.length = unit_len;
                        unit_len = 0;
                    }
                }
            }
        }
        else {
            unit_len += line_buffer.size() + 1; // one for a linebreak(whatever type it is)
        }
    }

    if (shader_units.size() != 0) {
        shader_units.back().length = unit_len; // set last length
    }
    glsl_src.clear();

    std::vector<char> unit_src;
    std::array<std::uint32_t, pragma_shader_type.size()> shader_type_counts{ 0 };

    for (auto& unit : shader_units) {
        const std::string unit_name = path.stem().string() + '.' + 
            std::to_string(shader_type_counts[unit.kind]) + '.' + pragma_shader_type[unit.kind] + ".shader";

        shader_type_counts[unit.kind] += 1;

        unit_src.resize(unit.length);
        glsl_src.seekg(unit.offset, std::ios_base::beg);
        glsl_src.read(unit_src.data(), unit_src.size());
        unit_src.back() = '\n'; // if no LF at eof add one or else might get garbage character at the end

        shaderc::Compiler compiler;
        const auto unit_result = compiler.CompileGlslToSpv(unit_src.data(), unit_src.size(), unit.kind, unit_name.data());
        if (unit_result.GetCompilationStatus() != shaderc_compilation_status_success) {
            if (unit_result.GetNumErrors() != 0) {
                std::cerr << "shaderc compilation error: " << unit_result.GetErrorMessage() << '\n';
            }
            throw std::runtime_error{ std::string{ "Failed to compile " } + pragma_shader_type[unit.kind] + " shader unit in " + path.string() };
        }

        zip_buffers.resize(zip_buffers.size() + 1);
        auto& buffer = zip_buffers.back();

        buffer.resize((unit_result.end() - unit_result.begin()) * sizeof(std::uint32_t));
        std::copy(unit_result.begin(), unit_result.end(), reinterpret_cast<uint32_t*>(buffer.data()));

        // write to zip
        add_buffer_to_archive(archive, unit_name, buffer.data(), buffer.size());
    }
    add_buffer_to_archive(archive, path.stem().string() + ".shader", nullptr, 0);
    return true;
}

template<typename T>
static std::span<const T> gltf_buffer(const tinygltf::Model& model, std::uint32_t accessor_ind) {
    const auto& accessor = model.accessors[accessor_ind];
    const auto& buffer_view = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[buffer_view.buffer];

    const T* ptr = reinterpret_cast<const T*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
    return { ptr, (accessor.ByteStride(buffer_view) / sizeof(T)) * accessor.count };
}

template<typename T>
std::vector<std::uint32_t> cast_range(const tinygltf::Model& model, std::uint32_t accessor) {
    const auto span = gltf_buffer<T>(model, accessor);
    if constexpr (std::is_same_v<T, std::uint32_t>) {
        return { span.begin(), span.end() };
    }
    else {
        std::vector<std::uint32_t> ret;
        ret.reserve(span.size());
        for (auto el : span) {
            ret.push_back(static_cast<std::uint32_t>(el));
        }
        return ret;
    }
}

// NOTE : only look for meshes, if vertices consider an error
static bool probe_mesh(zip_t* archive, const fs::path& path) {
    using namespace tinygltf;
    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    const auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    if (!ret) {
        return false;
    }

    // Structure:
    // u64 - vertex count
    // u8  - uv count(0 if none)
    // vertex buffer of float3
    // uv buffers of float2, with leading

    for (const auto& mesh : model.meshes) {
        struct vec3 { float x, y, z; };
        struct vec2 { float x, y; };
        std::vector<vec3> mesh_pos;
        std::vector<vec2> mesh_tex;

        for (const auto& primitive : mesh.primitives) {
            const auto ind_type = model.accessors[primitive.indices].componentType;

            std::vector<std::uint32_t> indices;
            switch (ind_type) {
            case 5120: indices = cast_range<std::int8_t>(model, primitive.indices); break;
            case 5121: indices = cast_range<std::uint8_t>(model, primitive.indices); break;
            case 5122: indices = cast_range<std::int16_t>(model, primitive.indices); break;
            case 5123: indices = cast_range<std::uint16_t>(model, primitive.indices); break;
            case 5125: indices = cast_range<std::uint32_t>(model, primitive.indices); break;
            default: throw std::runtime_error{ "TODO : gltf parsing, you got error in indices, pls check it out" };
            }

            const auto pos_buffer = gltf_buffer<vec3>(model, primitive.attributes.at("POSITION"));
            const auto tex_buffer = gltf_buffer<vec2>(model, primitive.attributes.at("TEXCOORD_0"));

            for (auto ind : indices) {
                mesh_pos.push_back(pos_buffer[ind]);
                mesh_tex.push_back(tex_buffer[ind]);
            }
        }

        const std::uint64_t vert_count = mesh_pos.size();
        const std::uint8_t tex_count = 1;

        zip_buffers.resize(zip_buffers.size() + 1);
        auto& buffer = zip_buffers.back();
        buffer.resize(sizeof vert_count + sizeof tex_count + vert_count * sizeof(vec3) + vert_count * tex_count * sizeof(vec2));

        *reinterpret_cast<std::uint64_t*>(buffer.data()) = vert_count;
        *reinterpret_cast<std::uint8_t*>(buffer.data() + sizeof vert_count) = tex_count;

        auto* dst_ptr = std::copy(mesh_pos.begin(), mesh_pos.end(), reinterpret_cast<vec3*>(buffer.data() + sizeof vert_count + sizeof tex_count));
        // 1 tex, no iteration TODO
        std::copy(mesh_tex.begin(), mesh_tex.end(), reinterpret_cast<vec2*>(dst_ptr));

        add_buffer_to_archive(archive, mesh.name + ".mesh", buffer.data(), buffer.size());
    }
    return true;
}

int main(int argc, char** argv) {
    // command line parsing
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        std::cout << "Usage: dab <dirs...> <output>\n";
        return 0;
    }
    if (argc < 3) {
        std::cerr << "Invalid usage, type --help or -h for commands\n";
        return -1;
    }

    const char* const out_name = argv[argc - 1];
    if (fs::exists(out_name)) {
        std::cout << "File " << out_name << " already exists\n";
        return 0;
    }

    // open zip
    int errnum{};
    auto archive_handle = zip_open(out_name, ZIP_CREATE, &errnum);
    if (archive_handle == nullptr) {
        zip_error_t zip_err{};
        zip_error_init_with_code(&zip_err, errnum);
        std::cerr << "Zip error for output file " << argv[argc - 1] << ": " << zip_error_strerror(&zip_err) << '\n';
        return -1;
    }  

    argc -= 2;
    argv += 1;

    // process files
    int total_files = 0;
    for (int i = 0; i < argc; ++i) {
        if (!fs::is_directory(argv[i])) {
            std::cerr << argv[i] << " is not a directory\n";
            return -1;
        }

        for (const auto& entry : fs::directory_iterator{ argv[i] }) {
            if (!entry.is_regular_file()) {
                continue;
            }

            using prober_writer = bool(*)(zip_t*, const fs::path&);
            constexpr std::array<prober_writer, 3> asset_writers{ probe_image, probe_shader, probe_mesh };

            for (auto writer : asset_writers) {
                try {
                    if (writer(archive_handle, entry.path())) {
                        total_files += 1;
                        break;
                    }
                }
                catch (const std::runtime_error& ex) {
                    std::cerr << ex.what() << '\n';
                    zip_discard(archive_handle);
                    return -1;
                }
            }
        }
    }

    if (zip_close(archive_handle) < 0) {
        std::cout << "Error creating a zip archive: " << zip_strerror(archive_handle) << '\n';
        return -1;
    }

    std::cout << "Added " << total_files << " assets to archive\n";
    return 0;
}