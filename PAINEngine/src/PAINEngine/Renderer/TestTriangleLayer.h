#pragma once

#include "Applications/AppLayer.h"

namespace PAIN {
	class TestTriangleLayer : public AppLayer {
	public:
		TestTriangleLayer();
		~TestTriangleLayer();
			
		void onUpdate() override;

		void onEvent(PAIN::Event::Event& e) override {
			// do nothing for now
		}

	private: 
		unsigned int m_VAO = 0;
		unsigned int m_VBO = 0;
		unsigned int m_ShaderProgram = 0;
	};
}