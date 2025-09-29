#pragma once

namespace PAIN {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		//tex coords
	};


	class Mesh {

	public:
		Mesh(const std::vector<Vertex> vertices, std::vector<unsigned int> indices);
		~Mesh();
		void Draw() const;

		static std::unique_ptr<Mesh> LoadObj(const std::string& mesh_file = "");

	private:

		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;
		size_t index_count;
	};
}