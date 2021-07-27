#include <filesystem>

#include <dablib/dab.hpp>

using namespace dab;
namespace fs = std::filesystem;

struct dab_file_paths {
    std::vector<fs::path> files;
    std::vector<char> rem_types;
};

// NOTE : by value here, most likely you want to move into it
void create_dab(std::string_view filename, const dab_file_paths& assets, bool pedantic, bool overwite);
void update_dab(std::string_view filename, const dab_file_paths& assets, bool pedantic);
void add_dab(std::string_view filename, const dab_file_paths& assets, bool pedantic);
void remove_dab(std::string_view filename, const dab_file_paths& assets);
