#pragma once

#include "PAINEngine/Applications/AppSystem.h"
#include "PAINEngine/CoreSystems/Renderer/Shader.h"
#include "glm/glm.hpp"

namespace PAIN {
	class TestTriangleLayer : public AppSystem {
	public:
		TestTriangleLayer();
		~TestTriangleLayer();

		void onAttach() override;
		void onUpdate() override;

		void onEvent([[maybe_unused]] PAIN::Event::Event& e) override {
			// do nothing for now
		}

	private:
		unsigned int m_VAO = 0;
		unsigned int m_VBO = 0;

		std::unique_ptr<Shader> m_Shader;

		// 3D Audio Demo Variables
		glm::vec3 m_cubePosition;
		glm::vec3 m_cameraPosition;
	};
}