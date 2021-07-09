#pragma once

#ifndef DRY_ASSET_SRC_H
#define DRY_ASSET_SRC_H

#include <string_view>
#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "util/num.hpp"

#include "graphics/material_base.hpp"
#include "vkw/pipeline_g.hpp"

namespace dry::asset {

enum class shader_stage{
    vertex,
    fragment
};

// hashed type
using hash_t = u32_t;
constexpr hash_t null_hash_v = (std::numeric_limits<hash_t>::max)();

// source types
// TODO : more vertex types
struct mesh_source {
    struct vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 tex;
    };

    std::vector<vertex> vertices;
    std::vector<u32_t> indices;
};

struct texture_source {
    byte_vec pixel_data;
    u32_t width;
    u32_t height;
    u8_t channels;
};

struct shader_source {
    struct shader_unit {
        byte_vec spirv;
        shader_stage stage;
    };

    shader_unit vert_stage;
    std::vector<shader_unit> oth_stages;
    vkw::g_pipeline_create_ctx create_ctx; // defaulted
};

struct material_source {
    hash_t shader;
    std::shared_ptr<material_base> material; // TODO : write a wrapper, std::any in assetreg needs copy

    material_source() = default;
    template<typename Material>
    material_source(hash_t sh, Material&& mat) :
        shader{ sh },
        material{ std::make_shared<std::decay_t<Material>>(std::forward<Material>(mat)) }
    {}
};

// extension lookup
template<typename>
struct asset_source_ext {
    static constexpr std::string_view ext = ".NULL";
};

template<>
struct asset_source_ext<mesh_source> {
    static constexpr std::string_view ext = ".mesh";
};
template<>
struct asset_source_ext<texture_source> {
    static constexpr std::string_view ext = ".texture";
};
template<>
struct asset_source_ext<shader_source> {
    static constexpr std::string_view vert_ext = ".vertex.shader";
    static constexpr std::string_view frag_ext = ".fragment.shader";

    static constexpr std::string_view ext = ".shader";
};

template<typename Asset>
constexpr std::string_view asset_source_ext_v = asset_source_ext<Asset>::ext;

template<typename T>
struct hashed_asset : public T {
    using T::T;
    hashed_asset(const T& base, hash_t hash_val) : T{ base }, hash{ hash_val }{}
    hashed_asset(T&& base, hash_t hash_val) : T{ std::move(base) }, hash{ hash_val }{}

    hash_t hash;
};

// typedefs for common types
using mesh_asset = hashed_asset<mesh_source>;
using texture_asset = hashed_asset<texture_source>;
using shader_asset = hashed_asset<shader_source>;
using material_asset = hashed_asset<material_source>;

}

#endif
