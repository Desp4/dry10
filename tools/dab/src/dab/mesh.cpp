#include <iostream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "readers.hpp"

bool write_mesh(asset_block& block, const fs::path& path) {
    // NOTE : only adding meshes for now
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate);

    if (!scene) {
        return false;
    }
    // write meshes
    for (int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh& mesh = *scene->mMeshes[i];
        const uint8_t uv_set_count = mesh.GetNumUVChannels();
        const size_t offset = block.file.tellp();
        
        // header - 1 byte for uv set count
        block.file.write(reinterpret_cast<const char*>(&mesh.mNumVertices), sizeof mesh.mNumVertices);
        block.file.write(reinterpret_cast<const char*>(&uv_set_count), sizeof uv_set_count);
        // write vertex data
        block.file.write(reinterpret_cast<const char*>(mesh.mVertices), mesh.mNumVertices * sizeof(aiVector3D));

        for (int j = 0; j < uv_set_count; ++j) {
            const uint8_t component_count = mesh.mNumUVComponents[j];
            block.file.write(reinterpret_cast<const char*>(&component_count), sizeof component_count);

            if (component_count == 3) {
                block.file.write(reinterpret_cast<const char*>(mesh.mTextureCoords[j]),
                    mesh.mNumVertices * sizeof(aiVector3D));
            } else {
                for (int k = 0; k < mesh.mNumVertices; ++k) {
                    block.file.write(reinterpret_cast<const char*>(mesh.mTextureCoords[j] + k),
                        component_count * sizeof(ai_real));
                };
            }
        }

        if (mesh.mName.length > 0xFF) {
            std::cout << path << ": mesh name" << mesh.mName.C_Str() << " is greater than 255 bytes, truncating to 255\n";
            block.header.push_back(dab::asset_decl{
                .name{ mesh.mName.C_Str(), mesh.mName.C_Str() + 0xFF },
                .offset = offset,
                .type = dab::asset_type::mesh
            });
        } else {
            block.header.push_back(dab::asset_decl{
                .name = mesh.mName.C_Str(),
                .offset = offset,
                .type = dab::asset_type::mesh
            });
        }
    }
    return true;
}