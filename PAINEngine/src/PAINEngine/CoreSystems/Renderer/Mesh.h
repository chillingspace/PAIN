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
		Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::string& ref);
		~Mesh();
		void Draw(unsigned int vao, unsigned int vbo, unsigned int ebo) const;

		unsigned int texture_id = 0;
		Material material{};

		std::string getRef() const {
			return ref;
		}

	private:
		std::string ref;
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
	};
}