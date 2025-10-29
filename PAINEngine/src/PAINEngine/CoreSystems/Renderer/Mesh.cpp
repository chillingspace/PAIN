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
			mn.x = min(mn.x, v.pos.x); mn.y = min(mn.y, v.pos.y); mn.z = min(mn.z, v.pos.z);
			mx.x = max(mx.x, v.pos.x); mx.y = max(mx.y, v.pos.y); mx.z = max(mx.z, v.pos.z);
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



	void Mesh::Draw(unsigned int vao, unsigned int vbo, unsigned int ebo) const
	{
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(unsigned int), indices.data());

		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error in Mesh::Draw: {} on mesh {}", err, ref);;
		}
	}

	
}