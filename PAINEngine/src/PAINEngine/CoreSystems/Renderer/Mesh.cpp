#include "pch.h"
#include "Mesh.h"

namespace PAIN {

	static void ComputeLocalAABB(const std::vector<Vertex>& verts,
		glm::vec3& outMin, glm::vec3& outMax)
	{
		if (verts.empty()) { outMin = outMax = glm::vec3(0); return; }
		glm::vec3 mn(FLT_MAX), mx(-FLT_MAX);
#ifdef PN_PLATFORM_WINDOWS
		for (const auto& v : verts) {
			mn.x = std::min(mn.x, v.pos.x); mn.y = std::min(mn.y, v.pos.y); mn.z = std::min(mn.z, v.pos.z);
			mx.x = std::max(mx.x, v.pos.x); mx.y = std::max(mx.y, v.pos.y); mx.z = std::max(mx.z, v.pos.z);
		}
#else
        for (const auto& v : verts) {
            mn.x = fmin(mn.x, v.pos.x); mn.y = fmin(mn.y, v.pos.y); mn.z = fmin(mn.z, v.pos.z);
            mx.x = fmax(mx.x, v.pos.x); mx.y = fmax(mx.y, v.pos.y); mx.z = fmax(mx.z, v.pos.z);
        }
#endif
		outMin = mn; outMax = mx;
	}

	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::string& ref)
		: vertices(vertices), indices(indices), ref(ref)
	{
		ComputeLocalAABB(vertices, aabb_min, aabb_max);
	}
	Mesh::~Mesh()
	{
	}

}