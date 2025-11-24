/*****************************************************************//**
 * \file   cTransform.h
 * \brief  Transform
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author 
 * \date   September 2025
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef C_TRANSFORM_H
#define C_TRANSFORM_H

#include "pch.h"
#include <refl.hpp>
#include "GLMSerialization.h"
#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	// for animated objects
	struct RootTransform {
		glm::vec3 scale{ 1.f };
		glm::quat rotation{ 1.f, 0.f, 0.f, 0.f };
		glm::vec3 translate{ 1.f };

		//glm::mat4 xform;

		static constexpr bool shouldSerialize = false;

		//RootTransform(glm::vec3 s, glm::quat r, glm::vec3 t) : scale(s), rotation(r), translate(t) {
		//	xform =
		//		glm::translate(glm::mat4(1.f), t) *
		//		glm::mat4_cast(r) *
		//		glm::scale(glm::mat4(1.f), s);
		//}
	};


	struct LocalTransform {
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };		// note that this is in parent space. so essentially this is translate, not pos
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

		//Serialization flag
		static constexpr bool ShouldSerialize = true;
	};

	// sysTransform.cpp handles init
	struct WorldTransform {
		glm::mat4 matrix{ 1.0f };
		bool dirty = true;

		//Serialization flag
		static constexpr bool ShouldSerialize = false;
	};

	//struct Transform {
	//	glm::f32vec3 position{ 0 ,0 ,0 };
	//	glm::f32quat rotation;
	//	glm::f32vec3 scale{ 1, 1, 1 };

	//	glm::mat4 getMatrix() const {
	//		glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
	//		glm::mat4 R = glm::mat4_cast(rotation);
	//		glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
	//		return T * R * S;
	//	}
	//};
}

 //No refl macro here, have to use custom GLM serializers, 
 // refl-cpp doesn't automatically know how to reflect GLM types

 //This is needed as json still does not now how to handle seri for the custom comps,
 //These types not supported by refl, so we need add struct-level seri 
namespace nlohmann {
	template<>
	struct adl_serializer<PAIN::LocalTransform> {
		static void to_json(json& j, const PAIN::LocalTransform& t) {
			j["position"] = t.position;
			j["rotation"] = t.rotation;
			j["scale"] = t.scale;
		}

		static void from_json(const json& j, PAIN::LocalTransform& t) {
			t.position = j["position"].get<glm::vec3>();
			t.rotation = j["rotation"].get<glm::quat>();
			t.scale = j["scale"].get<glm::vec3>();
		}
	};
}

REFL_TYPE(PAIN::LocalTransform)
REFL_FIELD(position)
REFL_FIELD(rotation)
REFL_FIELD(scale)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::LocalTransform>);

#endif
