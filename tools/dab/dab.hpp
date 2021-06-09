#include <vector>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

using byte_vector = std::vector<std::byte>;
using parsed_file = std::vector<std::pair<byte_vector, std::string>>;

constexpr std::array<std::string_view, 1> shader_extensions{ ".glsl" };
constexpr std::array<std::string_view, 2> texture_extensions{ ".png", ".jpg" };
constexpr std::array<std::string_view, 1> mesh_extensions{ ".gltf" };

parsed_file parse_shader(const fs::path& path);
parsed_file parse_texture(const fs::path& path);
parsed_file parse_mesh(const fs::path& path);