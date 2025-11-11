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
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"


namespace PAIN {
	class Skybox {
	private:
		Skybox();
		~Skybox();

		unsigned int skybox_tex;
		unsigned int cubemap_tex;

		void convertEquirectangularToCubemap();
		void renderCube();

		std::shared_ptr<Assets::Shader> conversionShader;
		std::shared_ptr<Assets::Shader> shader;

		std::shared_ptr<Services> services;

		//int winWidth = 0;
		//int winHeight = 0;
	private:
		// for image based lighting

		unsigned int irradiance_map;
		void generateIrradianceMap();

		unsigned int prefilter_map;
		void generatePrefilterMap();

		unsigned int brdf_tex;
		void generateBRDFLUT();

		void renderQuad();


	public:
		static Skybox& get() {
			static Skybox instance;
			return instance;
		}

		void init(const std::shared_ptr<Services>& services, const std::filesystem::path& skybox_path);

		unsigned int getSkyboxTex() const {
			return skybox_tex;
		}

		unsigned int getIrradianceMap() const {
			return irradiance_map;
		}

		unsigned int getPrefilterMap() const {
			return prefilter_map;
		}

		unsigned int getBrdfLUT() const {
			return brdf_tex;
		}

		void render(const glm::mat4& view, const glm::mat4& proj);
	};
}

