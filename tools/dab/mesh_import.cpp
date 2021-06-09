#include <span>
#include <concepts>

#include <tiny_gltf.h>

#include "dab.hpp"

template<typename Container>
static byte_vector& operator<<(byte_vector& dst_bytes, const Container& container) {
    using value_type = typename Container::value_type;

    const auto dst_ind = dst_bytes.size();
    dst_bytes.resize(dst_bytes.size() + container.size() * sizeof(value_type));

    std::copy(container.begin(), container.end(), reinterpret_cast<value_type*>(dst_bytes.data() + dst_ind));
    return dst_bytes;
}

template<std::integral T>
static byte_vector& operator<<(byte_vector& dst_bytes, T value) {
    const auto dst_ind = dst_bytes.size();
    dst_bytes.resize(dst_bytes.size() + sizeof(T));

    *reinterpret_cast<T*>(dst_bytes.data() + dst_ind) = value;
    return dst_bytes;
}

template<typename T>
static std::span<const T> get_accessor_buffer(const tinygltf::Model& model, std::uint32_t accessor_ind) {
    const auto& accessor = model.accessors[accessor_ind];
    const auto& buffer_view = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[buffer_view.buffer];

    const T* ptr = reinterpret_cast<const T*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
    return { ptr, (accessor.ByteStride(buffer_view) / sizeof(T)) * accessor.count };
}

template<typename T>
static std::vector<std::uint32_t> cast_indices_range(const tinygltf::Model& model, std::uint32_t accessor) {
    const auto span = get_accessor_buffer<T>(model, accessor);
    std::vector<std::uint32_t> ret;
    ret.reserve(span.size());

    for (auto el : span) {
        ret.push_back(static_cast<std::uint32_t>(el));
    }
    return ret;   
}

template<>
static std::vector<std::uint32_t> cast_indices_range<std::uint32_t>(const tinygltf::Model& model, std::uint32_t accessor) {
    const auto span = get_accessor_buffer<std::uint32_t>(model, accessor);
    return { span.begin(), span.end() };
}

parsed_file parse_mesh(const fs::path& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    const auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    if (!ret) {
        throw std::runtime_error{ "Error parsing GLTF file " + path.string() + ", error:" + err };
    }

    // structure
    // u64 index count
    // u64 vertex count
    // index buffer
    // interleaved vertices {v3 pos, v3 normal, v2 uv}, if either is not present fill with 0s
    parsed_file ret_file;

    for (const auto& mesh : model.meshes) {
        struct vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };
        struct vec2 { float x = 0.0f, y = 0.0f; };
        struct vertex { vec3 pos; vec3 normal; vec2 uv; };

        std::vector<std::uint32_t> mesh_indices;
        std::vector<vertex> mesh_vertices;
        // TODO : dig through docs or try mesh with multiple primitives, need to have proper indices
        for (const auto& primitive : mesh.primitives) {
            if (primitive.indices == -1) {
                // TODO right here
                throw std::runtime_error{ "GLTF file " + path.string() + " doesn't contain indices, no auto generation yet" };
            }

            const auto ind_type = model.accessors[primitive.indices].componentType;

            std::vector<std::uint32_t> indices;
            switch (ind_type) {
            case 5120: indices = cast_indices_range<std::int8_t>(model, primitive.indices); break;
            case 5121: indices = cast_indices_range<std::uint8_t>(model, primitive.indices); break;
            case 5122: indices = cast_indices_range<std::int16_t>(model, primitive.indices); break;
            case 5123: indices = cast_indices_range<std::uint16_t>(model, primitive.indices); break;
            case 5125: indices = cast_indices_range<std::uint32_t>(model, primitive.indices); break;
            default: throw std::runtime_error{ "Unexpected index format in GLTF file " + path.string() + " this shouldn't happen" };
            }

            // POSITION always present
            const std::span<const vec3> pos_buffer = get_accessor_buffer<vec3>(model, primitive.attributes.at("POSITION"));
            std::vector<vertex> vertices(pos_buffer.size());
            // NOTE : not ideal iterating 3 times instead of one
            for (auto i = 0u; i < vertices.size(); ++i) {
                vertices[i].pos = pos_buffer[i];
            }

            if (primitive.attributes.contains("NORMAL")) {
                const std::span<const vec3> normal_buffer = get_accessor_buffer<vec3>(model, primitive.attributes.at("NORMAL"));
                for (auto i = 0u; i < vertices.size(); ++i) {
                    vertices[i].normal = normal_buffer[i];
                }
            }
            
            if (primitive.attributes.contains("TEXCOORD_0")) {
                const std::span<const vec2> tex_buffer = get_accessor_buffer<vec2>(model, primitive.attributes.at("TEXCOORD_0"));
                for (auto i = 0u; i < vertices.size(); ++i) {
                    vertices[i].uv = tex_buffer[i];
                }
            }

            mesh_indices.insert(mesh_indices.end(), indices.begin(), indices.end());
            mesh_vertices.insert(mesh_vertices.end(), vertices.begin(), vertices.end());
        }

        const std::uint64_t index_count = mesh_indices.size();
        const std::uint64_t vertex_count = mesh_vertices.size();

        byte_vector mesh_data;
        mesh_data.reserve(sizeof(std::uint64_t) * 2 + index_count * sizeof(std::uint32_t) + vertex_count * sizeof(vertex));
        mesh_data << index_count << vertex_count << mesh_indices << mesh_vertices;
       
        ret_file.emplace_back(std::move(mesh_data), mesh.name + ".mesh");
    }
    return ret_file;
}