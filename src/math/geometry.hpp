#pragma once

#ifndef DRY_MATH_GEOMETRY_H
#define DRY_MATH_GEOMETRY_H

#include <vector>

#include <glm/vec3.hpp>

#include "util/num.hpp"

namespace dry::math {

struct sphere {
	glm::vec3 pos;
	f32_t radius;
};

sphere minimal_bounding_sphere(std::vector<glm::vec3> points);

}

#endif