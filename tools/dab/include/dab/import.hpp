#pragma once

#include <fstream>

#include "dabasset.hpp"

namespace dab {

class dab_importer {
public:
    bool open(const std::filesystem::path& filepath);
    void close();

    std::vector<asset_decl> declarations();

    template<class T>
    T read(size_t offset);

private:
    std::ifstream _file;
};

template<>
mesh dab_importer::read(size_t);
template<>
texture dab_importer::read(size_t);
template<>
shader dab_importer::read(size_t);

template<class T>
T dab_importer::read(size_t offset) {
    static_assert(false, "unspecialized use is forbindden");
}

}