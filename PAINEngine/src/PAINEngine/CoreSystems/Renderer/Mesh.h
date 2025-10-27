#pragma once

#include "Material.h"

namespace PAIN {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		//tex coords
		glm::vec2 uv;
	};

	class Mesh {

	public:
		Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();
		void Draw(unsigned int vao, unsigned int vbo, unsigned int ebo) const;

		const glm::vec3& getAABBMin() const { return aabb_min; }
		const glm::vec3& getAABBMax() const { return aabb_max; }

		const std::vector<Vertex>& getVertices() const { return vertices; }
		const std::vector<unsigned int>& getIndices() const { return indices; }

		unsigned int texture_id = 0;
		Material material{};

	private:
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		glm::vec3 aabb_min{ 0 }, aabb_max{ 0 };

		static constexpr int MAX_VERTICES = 1000000;
		static constexpr int MAX_INDICES = 1000000;
	};
}