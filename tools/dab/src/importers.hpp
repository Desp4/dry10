#pragma once

#ifndef DAB_IMPORTERS_H
#define DAB_IMPORTERS_H

#include "dab.hpp"
#include "io_op.hpp"

// NOTE : TODO : every parser just returns 1 byte vector, name string is just the filename
//   EXCEPT the mesh parser(and shader too for now)
using parsed_file = std::vector<std::pair<byte_vector, std::string>>;

parsed_file parse_shader(const fs::path& path);
parsed_file parse_texture(const fs::path& path);
parsed_file parse_mesh(const fs::path& path);

#endif