#include "pch.h"
#include "Mesh.h"

namespace PAIN {
	Mesh::Mesh(const Vertex* vertices, size_t vertexCount,
		const unsigned int* indices, size_t indexCount)
	{
		index_count = static_cast<size_t>(indexCount);

		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate and bind VBO
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

		// Generate and bind EBO (index buffer)
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);


		// Position attribute, layout(location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		// color attribute, layout(location = 1)
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
		//glEnableVertexAttribArray(1);

		// Normal attribute, layout(location = 1)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(1);

		// Unbind VAO
		glBindVertexArray(0);

	}
	Mesh::~Mesh()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
	void Mesh::Draw() const
	{
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	std::unique_ptr<Mesh> Mesh::LoadObj(const std::string& mesh_file)
	{

		static constexpr Vertex vertices[] = {
			// Front (+Z)
			{{-0.5f, -0.5f,  0.5f}, {0,0,1}},
			{{ 0.5f, -0.5f,  0.5f}, {0,0,1}},
			{{ 0.5f,  0.5f,  0.5f}, {0,0,1}},
			{{-0.5f,  0.5f,  0.5f}, {0,0,1}},

			// Back (-Z)
			{{ 0.5f, -0.5f, -0.5f}, {0,0,-1}},
			{{-0.5f, -0.5f, -0.5f}, {0,0,-1}},
			{{-0.5f,  0.5f, -0.5f}, {0,0,-1}},
			{{ 0.5f,  0.5f, -0.5f}, {0,0,-1}},

			// Left (-X)
			{{-0.5f, -0.5f, -0.5f}, {-1,0,0}},
			{{-0.5f, -0.5f,  0.5f}, {-1,0,0}},
			{{-0.5f,  0.5f,  0.5f}, {-1,0,0}},
			{{-0.5f,  0.5f, -0.5f}, {-1,0,0}},

			// Right (+X)
			{{ 0.5f, -0.5f,  0.5f}, {1,0,0}},
			{{ 0.5f, -0.5f, -0.5f}, {1,0,0}},
			{{ 0.5f,  0.5f, -0.5f}, {1,0,0}},
			{{ 0.5f,  0.5f,  0.5f}, {1,0,0}},

			// Top (+Y)
			{{-0.5f,  0.5f,  0.5f}, {0,1,0}},
			{{ 0.5f,  0.5f,  0.5f}, {0,1,0}},
			{{ 0.5f,  0.5f, -0.5f}, {0,1,0}},
			{{-0.5f,  0.5f, -0.5f}, {0,1,0}},

			// Bottom (-Y)
			{{-0.5f, -0.5f, -0.5f}, {0,-1,0}},
			{{ 0.5f, -0.5f, -0.5f}, {0,-1,0}},
			{{ 0.5f, -0.5f,  0.5f}, {0,-1,0}},
			{{-0.5f, -0.5f,  0.5f}, {0,-1,0}}
		};

		static constexpr unsigned int indices[] = {
			// Front (+Z)
			0,1,2, 0,2,3,
			// Back (-Z)
			4,5,6, 4,6,7,
			// Left (-X)
			8,9,10, 8,10,11,
			// Right (+X)
			12,13,14, 12,14,15,
			// Top (+Y)
			16,17,18, 16,18,19,
			// Bottom (-Y)
			20,21,22, 20,22,23
		};

		return std::make_unique<Mesh>(vertices, std::size(vertices), indices, std::size(indices));
	}
}