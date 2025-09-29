#include "pch.h"
#include "Mesh.h"

namespace PAIN {
	Mesh::Mesh(const std::vector<Vertex> vertices, std::vector<unsigned int> indices)
	{
		index_count = static_cast<size_t>(indices.size());

		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate and bind VBO
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		// Generate and bind EBO (index buffer)
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);


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
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

#ifdef PN_PLATFORM_WINDOWS
		// Get current working directory and build paths from there
		std::filesystem::path current_path = std::filesystem::current_path();
		std::filesystem::path project_root = current_path / "PAIN"; // Adjust as needed

		// Or try to find the project root by looking for a marker file
		std::filesystem::path search_path = current_path;
		while (search_path.has_parent_path()) {
			if (std::filesystem::exists(search_path / "PAIN" / "assets")) {
				project_root = search_path / "PAIN";
				break;
			}
			search_path = search_path.parent_path();
		}

		std::filesystem::path mesh_full = project_root / "assets" / "Meshes" / mesh_file;

		if (!std::filesystem::exists(mesh_full) || mesh_file == "")
		{
#endif
			PN_CORE_ERROR("Mesh file not found: {}, loading default mesh", mesh_file == "" ? "No mesh file given" : mesh_file);
			vertices = {
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

			indices = {
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
			return std::make_unique<Mesh>(vertices, indices);
#ifdef PN_PLATFORM_WINDOWS
		}

		struct TempVertex {
			int pIdx = -1, nIdx = -1;
			TempVertex() = default;
			TempVertex(const std::string& token) {
				// Parse formats: v//n or v/n
				if (token.find("//") != std::string::npos) {
					sscanf(token.c_str(), "%d//%d", &pIdx, &nIdx);
				}
				else {
					sscanf(token.c_str(), "%d/%d", &pIdx, &nIdx);
				}
			}
		};

		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;

		std::ifstream objStream(mesh_full);
		if (!objStream) {
			PN_CORE_ERROR("Could not open {}", mesh_full.string());
			assert(false);
		}

		std::string line;
		while (std::getline(objStream, line)) {
			if (line.empty() || line[0] == '#') continue;

			std::istringstream ls(line);
			std::string token;
			ls >> token;

			if (token == "v") {
				glm::vec3 p;
				ls >> p.x >> p.y >> p.z;
				positions.push_back(p);
			}
			//else if (token == "vt") {
			//	// Process texture coordinate
			//	float s, t;
			//	ls >> s >> t;
			//	texCoords.push_back(glm::vec2(s, t));
			//}
			else if (token == "vn") {
				glm::vec3 n;
				ls >> n.x >> n.y >> n.z;
				normals.push_back(n);
			}
			else if (token == "f") {
				std::vector<TempVertex> faceVerts;
				std::string vStr;
				while (ls >> vStr) faceVerts.emplace_back(vStr);

				// Fan triangulation
				for (size_t i = 1; i + 1 < faceVerts.size(); i++) {
					TempVertex tv[3] = { faceVerts[0], faceVerts[i], faceVerts[i + 1] };
					for (int j = 0; j < 3; j++) {
						Vertex v{};
						if (tv[j].pIdx > 0) v.pos = positions[tv[j].pIdx - 1];
						if (tv[j].nIdx > 0) v.normal = normals[tv[j].nIdx - 1];

						vertices.push_back(v);
						indices.push_back((unsigned int)vertices.size() - 1);
					}
				}
			}
		}

		// can add deduplication
		// can add normal fallback
		// can add tangents (optional)
		// can add generalization
		// must add texcoords

		return std::make_unique<Mesh>(vertices, indices);
#endif
	}
}