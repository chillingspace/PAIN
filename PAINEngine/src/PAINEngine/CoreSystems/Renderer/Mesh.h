#pragma once

namespace PAIN {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		//tex coords
	};

	class Mesh {

	public:
		Mesh(const Vertex* vertices, size_t vertexCount,
			const unsigned int* indices, size_t indexCount);
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