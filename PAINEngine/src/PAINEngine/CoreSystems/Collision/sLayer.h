/*****************************************************************//**
 * \file   sLayer.h
 * \brief  Declaration of layer service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef LAYER_SERVICE_H
#define LAYER_SERVICE_H

#include "pch.h"

// To-do for ben: add proper documentation

// Layer collision rules :
// NON_MOVING collides with : MOVING, DEBRIS
//
// MOVING collides with	    : NON_MOVING, MOVING, SENSOR
//
// DEBRIS collides with     : NON_MOVING only
//
// SENSOR collides with     : MOVING only


namespace PAIN {
	namespace Layer {

		static constexpr JPH::ObjectLayer UNUSED1 = 0; // 4 unused values so that broadphase layers values don't match with object layer values (for testing purposes)
		static constexpr JPH::ObjectLayer UNUSED2 = 1;
		static constexpr JPH::ObjectLayer UNUSED3 = 2;
		static constexpr JPH::ObjectLayer UNUSED4 = 3;
		static constexpr JPH::ObjectLayer NON_MOVING = 4;
		static constexpr JPH::ObjectLayer MOVING = 5;
		static constexpr JPH::ObjectLayer DEBRIS = 6; // Example: Debris collides only with NON_MOVING
		static constexpr JPH::ObjectLayer SENSOR = 7; // Sensors only collide with MOVING objects
		static constexpr JPH::ObjectLayer NUM_LAYERS = 8;

	}

	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool					ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			switch (inObject1)
			{
			case Layer::UNUSED1:
			case Layer::UNUSED2:
			case Layer::UNUSED3:
			case Layer::UNUSED4:
				return false;
			case Layer::NON_MOVING:
				return inObject2 == Layer::MOVING || inObject2 == Layer::DEBRIS;
			case Layer::MOVING:
				return inObject2 == Layer::NON_MOVING || inObject2 == Layer::MOVING || inObject2 == Layer::SENSOR;
			case Layer::DEBRIS:
				return inObject2 == Layer::NON_MOVING;
			case Layer::SENSOR:
				return inObject2 == Layer::MOVING;
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

	/// Broadphase Layer
	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::BroadPhaseLayer DEBRIS(2);
		static constexpr JPH::BroadPhaseLayer SENSOR(3);
		static constexpr JPH::BroadPhaseLayer UNUSED(4);
		static constexpr glm::uint NUM_LAYERS(5);
	};

	/// BroadPhaseLayerInterface implementation
	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl()
		{
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[Layer::UNUSED1] = BroadPhaseLayers::UNUSED;
			mObjectToBroadPhase[Layer::UNUSED2] = BroadPhaseLayers::UNUSED;
			mObjectToBroadPhase[Layer::UNUSED3] = BroadPhaseLayers::UNUSED;
			mObjectToBroadPhase[Layer::UNUSED4] = BroadPhaseLayers::UNUSED;
			mObjectToBroadPhase[Layer::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layer::MOVING] = BroadPhaseLayers::MOVING;
			mObjectToBroadPhase[Layer::DEBRIS] = BroadPhaseLayers::DEBRIS;
			mObjectToBroadPhase[Layer::SENSOR] = BroadPhaseLayers::SENSOR;
		}

		virtual glm::uint					GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual JPH::BroadPhaseLayer			GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			JPH_ASSERT(inLayer < Layer::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch ((JPH::BroadPhaseLayer::Type)inLayer)
			{
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::DEBRIS:		return "DEBRIS";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::SENSOR:		return "SENSOR";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::UNUSED:		return "UNUSED";
			default:													JPH_ASSERT(false); return "INVALID";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer					mObjectToBroadPhase[Layer::NUM_LAYERS];
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool					ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
			case Layer::NON_MOVING:
				return inLayer2 == BroadPhaseLayers::MOVING || inLayer2 == BroadPhaseLayers::DEBRIS;
			case Layer::MOVING:
				return inLayer2 == BroadPhaseLayers::NON_MOVING || inLayer2 == BroadPhaseLayers::MOVING || inLayer2 == BroadPhaseLayers::SENSOR;
			case Layer::DEBRIS:
				return inLayer2 == BroadPhaseLayers::NON_MOVING;
			case Layer::SENSOR:
				return inLayer2 == BroadPhaseLayers::MOVING;
			case Layer::UNUSED1:
			case Layer::UNUSED2:
			case Layer::UNUSED3:
				return false;
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

}




#endif