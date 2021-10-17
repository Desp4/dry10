#include "geometry.hpp"

#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "util/num.hpp"

namespace dry::math {

constexpr bool is_within_sphere(glm::vec3 point, sphere sphere) {
	const f32_t x = point.x - sphere.pos.x;
	const f32_t y = point.y - sphere.pos.y;
	const f32_t z = point.z - sphere.pos.z;
	return (x * x + y * y + z * z) <= (sphere.radius * sphere.radius);
}

constexpr f32_t vec3_transpose_mul(glm::vec3 vec_t, glm::vec3 vec) {
	return vec_t.x * vec.x + vec_t.y * vec.y + vec_t.z * vec.z;
}

sphere mb_dash(const std::vector<glm::vec3>& b) {
	assert(b.size() <= 4);

	if (b.size() == 0) {
		return { { 0, 0, 0 }, 0 };
	}

	// TODO : trivial optimization - Qs can be computed only once
	glm::mat3 a_b{ 1.0f };

	// TODO : trivial optimization - only need to calculate 1 diagonal half
	for (auto column = 1u; column < b.size(); ++column) {
		const auto q1 = b[column] - b[0];

		for (auto row = 1u; row < b.size(); ++row) {
			const auto q2 = b[row] - b[0];
			a_b[column - 1][row - 1] = 2.0f * vec3_transpose_mul(q1, q2);
		}
	}

	a_b = glm::inverse(a_b);
	glm::vec3 v_b{ 0 };
	
	for (auto i = 1u; i < b.size(); ++i) {
		const auto q = b[i] - b[0];
		v_b[i - 1] = vec3_transpose_mul(q, q);
	}

	const glm::vec3 lambdas = a_b * v_b;

	glm::vec3 c{ 0 };
	for (auto i = 1u; i < b.size(); ++i) {
		c += lambdas[i - 1] * (b[i] - b[0]);
	}

	return { .pos = c + b[0], .radius = std::sqrtf(vec3_transpose_mul(c, c)) };
}

/*
    Implementation of algorithms described in:
    [1] Bernd Gartner, "Fast and Robust Smallest Enclosing Balls"
    [2] Emo Welzl, "Smallest enclosing disks (balls and ellipsoids)"
*/
sphere mtf_mb(std::span<glm::vec3> l, std::vector<glm::vec3>& b) {
	auto mb = mb_dash(b);

	// size = d + 1, d(dimensions) = 3
	if (b.size() == 4) {
		return mb;
	}

	for (auto i = 0u; i < l.size(); ++i) {
		if (!is_within_sphere(l[i], mb)) {
			std::vector<glm::vec3> b_pi = b;
			b_pi.push_back(l[i]);
			mb = mtf_mb({ l.begin(), l.begin() + i }, b_pi);

			auto p_tmp = std::move(l[i]);
			std::copy_backward(l.begin(), l.begin() + i, l.begin() + i + 1);
			l[0] = std::move(p_tmp);
		}
	}
	return mb;
}

sphere minimal_bounding_sphere(std::vector<glm::vec3> points) {
	std::vector<glm::vec3> b;
	return mtf_mb(points, b);
}

}
