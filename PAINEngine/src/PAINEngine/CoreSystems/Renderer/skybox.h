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

		unsigned int skybox_tex = 0;
		unsigned int cubemap_tex = 0;

		void convertEquirectangularToCubemap();
		void ConfigureSourceCubemapForIblSampling();
		void renderCube();

		std::shared_ptr<Assets::Shader> conversionShader;
		std::shared_ptr<Assets::Shader> shader;

		std::shared_ptr<Services> services;

		//int winWidth = 0;
		//int winHeight = 0;
	private:
		// for image based lighting

		unsigned int irradiance_map = 0;
		void generateIrradianceMap();

		unsigned int prefilter_map = 0;
		void generatePrefilterMap();

		unsigned int brdf_tex = 0;
		void generateBRDFLUT();

		void renderQuad();

	public:

		static Skybox& get() {
			static Skybox instance;
			return instance;
		}

		void init(const std::shared_ptr<Services>& services);

		void setTexture(Assets::GUID const& id);
		void setTexture(const std::filesystem::path& skybox_path);

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

