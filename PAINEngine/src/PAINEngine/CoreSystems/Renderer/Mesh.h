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
		Mesh(const std::vector<Vertex> vertices, std::vector<unsigned int> indices);
		~Mesh();
		void Draw() const;

		static std::shared_ptr<Mesh> LoadObj(const std::string& mesh_file = "");

		unsigned int texture_id = 0;
		Material material{};

	private:

		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;
		size_t index_count;
	};
}