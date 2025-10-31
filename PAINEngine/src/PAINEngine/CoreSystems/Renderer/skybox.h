/*****************************************************************//**
 * \file   skybox.h
 * \brief  
 * 
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "pch.h"
#include "CoreSystems/Path/Path.h"
#include "Applications/AppSystem.h"

#include "CoreSystems/Assets/sAssets.h"


namespace PAIN {
	class Skybox {
	private:
		Skybox();
		~Skybox();

		unsigned int skybox_tex;
		unsigned int cubemap_tex;

		void loadHdr(const std::string& path);
		void convertEquirectangularToCubemap();
		void renderCube();

		std::shared_ptr<Assets::Shader> conversionShader;
		std::shared_ptr<Assets::Shader> shader;

		std::shared_ptr<Services> services;

		int winWidth = 0;
		int winHeight = 0;
	public:
		static Skybox& get() {
			static Skybox instance;
			return instance;
		}

		void init(const std::shared_ptr<Services>& services, const std::string& skybox_path);

		unsigned int getSkyboxTex() const {
			return skybox_tex;
		}

		void render(const glm::mat4& view, const glm::mat4& proj);
	};
}

