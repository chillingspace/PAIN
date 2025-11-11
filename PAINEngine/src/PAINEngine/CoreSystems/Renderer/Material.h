#pragma once

#include "pch.h"
#include "Applications/AppSystem.h"
#include "CoreSystems/Assets/AssetLoader.h"

namespace PAIN {
  //  struct Material {
		//float rough{ 0.1f };        // 0.1 -> smooth, 1 -> rough
  //      float metal{ 0.3f };
  //      glm::vec3 color{ 1.f,0.f,0.f };
  //      bool useTex{ false };        // pass in as float
		//unsigned int tex{ 0 };      // just for redundanncy
  //      // sampler2D tex;
		//bool alwaysLit{ false };
		//bool useAo{ false };
		//unsigned int aoTex{ 0 };


		//enum REFLECTION_TYPES {
		//	NONE = 0,
		//	PLANAR,
		//	SCREEN_SPACE,
		//	NUM_REFLECTION_TYPES,
		//};
		//REFLECTION_TYPES reflection_type{ REFLECTION_TYPES::NONE };
  //  };

	/*
	void InitMaterial(std::shared_ptr<Services> services, Assets::Material& mat, const std::string& base_path) {
		std::string vpath;

		auto getGlTexInt = [&](const std::string& map_name) {
			auto texture_path = services->get<Path::Path>()->resolvePath(base_path + map_name);
			return RawLoader::load(texture_path, texture_path);
			};

		mat.gl_diffuse_tex = getGlTexInt(mat.diffuseMap);
		mat.gl_normal_tex = getGlTexInt(mat.normalMap);
		mat.gl_metallic_tex = getGlTexInt(mat.metallicMap);
		mat.gl_roughness_tex = getGlTexInt(mat.roughnessMap);
		mat.gl_ao_tex = getGlTexInt(mat.aoMap);
		mat.gl_emission_tex = getGlTexInt(mat.emissionMap);
	}
	*/

}

