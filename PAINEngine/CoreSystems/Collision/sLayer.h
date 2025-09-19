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

namespace PAIN {
	namespace Layer {

		/**********************************************************************
		* LAYER TYPES
		******************************************************************/
		// Broad phase layer 1-1 mapping with number of obj layers
		// 4 due to trigger, static, dynamic, character (player) layers

		// Object layers are used to define who collides with who. (Static collide with dynamic)
		// (Dynamic collide with Static, but static will not collide with static)
		// If is trigger, no physics resolve, just report overlap
		namespace ObjectLayers
		{
			static constexpr JPH::ObjectLayer ol_static = 0;
			static constexpr JPH::ObjectLayer ol_dynamic = 1;
			static constexpr JPH::ObjectLayer ol_trigger = 2;
			static constexpr JPH::ObjectLayer ol_character = 3;

			static constexpr JPH::ObjectLayer num_obj_layers = 4;
		}

		// From gpt explanation: 
		/*BroadPhase Layers are categories you assign to objects so the broad phase can skip unnecessary checks.
		Example:

		Static terrain → never collides with other static terrain.

		Projectiles → might only collide with characters, not with each other.*/
		namespace BroadPhaseLayers
		{
			/* Gpt explanation : Characters (usually kinematic controllers like capsules) are a special case — they typically:
			Collide with static & dynamic so they walk on floors and bump into things.
			Don’t collide with other characters (to avoid jitter/pushing).
			Usually don’t collide with triggers physically, but still report overlaps (so you can detect pickups/volumes).*/

			// Trigger layers are like for pickup objects, detect collision but no resolution

			static constexpr JPH::BroadPhaseLayer bp_static{ 0 };
			static constexpr JPH::BroadPhaseLayer bp_dynamic{ 1 };
			static constexpr JPH::BroadPhaseLayer bp_trigger{ 2 };
			static constexpr JPH::BroadPhaseLayer bp_character{ 3 };

			static constexpr JPH::uint num_bp_layers = 4;
		}

		/**********************************************************************
		* LAYER MAPPING AND FILTERING WITH BROADPHASE LAYERS TO OBJ (GAME) LAYERS
		******************************************************************/

		class PAINObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
		{
		public:
			// Map each object layer to a broad-phase layer
			JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer obj_layer) const
			{
				switch (obj_layer)
				{
				case ObjectLayers::ol_static:    return BroadPhaseLayers::bp_static;
				case ObjectLayers::ol_dynamic:   return BroadPhaseLayers::bp_dynamic;
				case ObjectLayers::ol_character: return BroadPhaseLayers::bp_character;
				case ObjectLayers::ol_trigger:   return BroadPhaseLayers::bp_trigger;
				default:                         return BroadPhaseLayers::bp_dynamic;
				}
			}

			// This can just return true, broadphase will use the mapping above
			bool ShouldCollide(JPH::ObjectLayer obj_layer, JPH::BroadPhaseLayer broad_phase_layer) const override
			{
				// Optional: prevent static vs static checks
				if (obj_layer == ObjectLayers::ol_static && broad_phase_layer == BroadPhaseLayers::bp_static)
					return false;

				return true;
			}
		};

		class PAINObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
		{
		public:
			bool ShouldCollide(JPH::ObjectLayer obj_layer_1, JPH::ObjectLayer obj_layer_2) const override
			{
				// Example rules:
				// Characters don’t collide with each other
				if (obj_layer_1 == ObjectLayers::ol_character && obj_layer_2 == ObjectLayers::ol_character)
					return false;

				// Triggers report overlaps but no physical collision
				if (obj_layer_1 == ObjectLayers::ol_trigger || obj_layer_2 == ObjectLayers::ol_trigger)
					return true;

				// Everything else collides
				return true;
			}
		};

		// Adjust as needed
		constexpr size_t MAXLAYERS = 32; 

		class Service
		{
		public:

			using layerMask = std::bitset<MAXLAYERS>;

			Service();
			~Service();

		private:
			std::string layer_tag;
			// Jolt ObjectLayer
			JPH::ObjectLayer objLayer;        
			// Jolt BroadPhaseLayer
			JPH::BroadPhaseLayer bpLayer;     
			// Filtering
			layerMask mask;      
			// Internal layer ID
			unsigned int id;       
			// Active/inactive
			bool b_state;       
			// Enable Y sorting
			bool b_ysort;                     
			// Entities in order
			//std::array<Entity::Type, MAX_ENTITY> entities; 
		};
	}
}




#endif