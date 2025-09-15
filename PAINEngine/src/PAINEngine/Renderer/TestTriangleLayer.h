#pragma once

#include "Applications/AppSystem.h"

namespace PAIN {
	class TestTriangleLayer : public AppSystem {
	public:
		TestTriangleLayer();
		~TestTriangleLayer();
			
		void onUpdate() override;

		void onEvent([[maybe_unused]] PAIN::Event::Event& e) override {
			// do nothing for now
		}

	private: 
		unsigned int m_VAO = 0;
		unsigned int m_VBO = 0;
		unsigned int m_ShaderProgram = 0;
	};
}