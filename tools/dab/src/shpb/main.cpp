#include <iostream>
#include <fstream>
#include <filesystem>

#include "share/shpb_type.hpp"

shpb::ShaderType argType(const char* str)
{
    if (!strcmp(str, "-v"))
        return shpb::Shader_Vertex;
    if (!strcmp(str, "-f"))
        return shpb::Shader_Fragment;
    if (!strcmp(str, "-g"))
        return shpb::Shader_Geometry;
    if (!strcmp(str, "-c"))
        return shpb::Shader_Compute;
    return shpb::Shader_None;
}

int main(int argc, char** argv)
{
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        std::cout <<
            "usage: shpb <files...> <out_file>\n" \
            "files\t{ -v | -f | -g | -c } <file>\n" \
            "-v\tvertex shader\n" \
            "-f\tfragment shader\n" \
            "-g\tgeometry shader\n" \
            "-c\tcompute shader\n";
        return 0;
    }

    if (argc < 4)
    {
        std::cerr << "invalid usage, type --help for details\n";
        return -1;
    }

    std::ofstream outFile(argv[--argc], std::ios_base::binary);
    if (!outFile.is_open())
    {
        std::cerr << "could not open file " << argv[argc] << '\n';
        return -1;
    }

    outFile.write(shpb::FILE_MAGIC, sizeof shpb::FILE_MAGIC);
    outFile.seekp(sizeof(uint32_t), std::ios_base::cur);

    uint32_t i = 1;
    while (i < argc)
    {
        const shpb::ShaderType type = argType(argv[i]);
        ++i;
        if (type == shpb::Shader_None || i == argc)
        {
            std::cerr << "invalid usage, type --help for details\n";
            outFile.close();
            std::filesystem::remove(argv[argc]);
            return -1;
        }

        std::ifstream shaderFile(argv[i], std::ios_base::binary | std::ios_base::ate);
        if (!shaderFile.is_open())
        {
            std::cerr << "could not open file " << argv[i] << '\n';
            outFile.close();
            std::filesystem::remove(argv[argc]);
            return -1;
        }

        const size_t size = shaderFile.tellg();
        shaderFile.seekg(0, std::ios_base::beg);

        outFile.write(reinterpret_cast<const char*>(&type), sizeof type);
        outFile.write(reinterpret_cast<const char*>(&size), sizeof size);
        outFile << shaderFile.rdbuf();
        ++i;
    }

    i /= 2; // rounds down, alright

    outFile.seekp(sizeof shpb::FILE_MAGIC, std::ios_base::beg);
    outFile.write(reinterpret_cast<const char*>(&i), sizeof i);

    return 0;
}