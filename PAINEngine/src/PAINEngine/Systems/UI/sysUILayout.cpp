/*****************************************************************//**
 * \file   sysUILayout.cpp
 * \brief  Definition of UI Layout system functions
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "Core.h"
#include "sysUILayout.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "Systems/Scripting/GameScriptingSystem.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Scene/sCameraController.h"
#include "CoreSystems/Assets/sAssets.h"   
#include "CoreSystems/Assets/Types/Texture.h"  


namespace PAIN {
	namespace UI {


		LayoutSystem::LayoutSystem(std::shared_ptr<Services> svc) : ISystem(svc)
		{
		}

		LayoutSystem::~LayoutSystem()
		{
		}

		void LayoutSystem::onUpdate(AppTiming timing, entt::registry& registry) {
			auto svc = services.lock();
			auto metadata_service = svc->get<MetaData::Service>();
			auto window_service = svc->get<Window::Window>();

			// Get camera matrices for world-space UI
			auto scene_service = svc->get<Scene::SceneManager>();
			Camera* cam = scene_service->GetActiveCamera();
			glm::mat4 view = cam->view();
			glm::mat4 projection = cam->projection();
			glm::vec2 viewport = window_service->getFrameBuffer();

			//static bool printed = false;
			//if (!printed) {
			//    cam->debugPrintFOV();
			//    printed = true;
			//}

			// For every UI label with a UIFollowsWorldEntity, the position is projected from world to screen and written to the label's UIRectTransform.calculated_world_position.
			updateFloatingLabels(registry, view, projection, viewport);

			//auto canvas_view = registry.view<UICanvas, UIRectTransform, Entity::Hierarchy>();

			auto canvas_group = registry.group<Entity::Hierarchy>(entt::get<UICanvas, UIRectTransform>);

			glm::vec2 screen_size = window_service->getFrameBuffer();

			// Convert this to group
			for (auto [entity, hierarchy, ui_canvas, ui_rect_trans] : canvas_group.each()) {
				// Only process root canvas nodes (no parent)
				if (hierarchy.parentGUID.IsValid()) {
					continue;  // Skip children, they'll be processed recursively
				}

				// Check layer visibility if canvas has a layer
				if (registry.all_of<Entity::Layer>(entity)) {
					const auto& layerComp = registry.get<Entity::Layer>(entity);
					auto scn_service = services.lock()->get<Scene::SceneManager>();
					if (!scn_service->isLayerEnabled(layerComp.layer_id)) {
						continue;  // Skip entire canvas hierarchy if layer is disabled
					}
				}

				// Calculate layout origin
				glm::vec2 layout_origin(0, 0);  // Default to screen origin

				// If canvas has world transform, use it (for world-space UI)
				if (registry.all_of<WorldTransform>(entity) && registry.all_of<UICanvas>(entity)) {
					const auto& ui_canvas = registry.get<UICanvas>(entity);
					if (ui_canvas.render_mode == CanvasRenderMode::WorldSpace) {
						const auto& wtrans = registry.get<WorldTransform>(entity);
						layout_origin = glm::vec2(wtrans.matrix[3].x, wtrans.matrix[3].y);
					}
				}

				processHierarchy(entity, registry, screen_size, layout_origin);
			}
		}


		//void LayoutSystem::processHierarchy(entt::entity entity, entt::registry& registry, const glm::vec2& parent_size, const glm::vec2& parent_pos)
		//{
		//	auto svc = services.lock();
		//	auto ecs = svc->get<ECS::Controller>();
		//	// Use WorldTransform for UI position
		//	glm::vec2 calculated_pos = parent_pos;
		//	glm::vec2 calculated_size = parent_size;

		//	// 1. world-space UI
		//	if (registry.all_of<WorldTransform>(entity) &&
		//		registry.all_of<UIRectTransform>(entity) &&
		//		!registry.all_of<UIFollowsWorldEntity>(entity))
		//	{
		//		const auto& w_trans = registry.get<WorldTransform>(entity);
		//		auto& rect = registry.get<UIRectTransform>(entity);

		//		calculated_pos = glm::vec2(w_trans.matrix[3]);
		//		// Calculate size using UI-specific logic or from the overall world scale
		//		glm::vec3 world_scale(
		//			glm::length(glm::vec3(w_trans.matrix[0])),
		//			glm::length(glm::vec3(w_trans.matrix[1])),
		//			glm::length(glm::vec3(w_trans.matrix[2]))
		//		);

		//		calculated_size = rect.size_delta * glm::vec2(world_scale);

		//		rect.calculated_world_position = calculated_pos;
		//		rect.calculated_world_size = calculated_size;

		//		//// If UIRectTransform has local size, multiply by world_scale.xy for final size
		//		//if (registry.all_of<UIRectTransform>(entity)) {
		//		//	auto& rect = registry.get<UIRectTransform>(entity);

		//		//	if (!registry.all_of<UIFollowsWorldEntity>(entity)) {
		//		//		calculated_size = rect.size_delta * glm::vec2(world_scale);
		//		//		rect.calculated_world_position = calculated_pos;
		//		//		rect.calculated_world_size = calculated_size;
		//		//	}
		//		//}
		//	}
		//	// screen-space UI/ canvas UI (no worldtransform)
		//	else if (registry.all_of<UIRectTransform>(entity) &&
		//		!registry.all_of<UIFollowsWorldEntity>(entity))
		//	{
		//		auto& rect = registry.get<UIRectTransform>(entity);

		//		//// For now: ignore anchors, treat anchored_position as bottom-left offset
		//		//calculated_pos = parent_pos + rect.anchored_position;
		//		//calculated_size = rect.size_delta;

		//		//rect.calculated_world_position = calculated_pos;
		//		//rect.calculated_world_size = calculated_size;

		//		// Compute rect in parent space from anchors + offsets
		//		// parent_pos/parent_size are already in screen pixels.
		//		glm::vec2 anchor_min_pos = parent_pos + rect.anchor_min * parent_size + rect.offset_min;
		//		glm::vec2 anchor_max_pos = parent_pos + rect.anchor_max * parent_size + rect.offset_max;

		//		glm::vec2 size;
		//		glm::vec2 pos;

		//		if (rect.anchor_min == rect.anchor_max) {
		//			// "Simple" case: fixed-size element
		//			size = rect.size_delta;

		//			// anchored_position is relative to anchor; then pivot shifts inside that rect
		//			glm::vec2 anchor_point = parent_pos + rect.anchor_min * parent_size + rect.anchored_position;
		//			pos = anchor_point - rect.pivot * size;
		//		}
		//		else {
		//			// Stretched between anchors
		//			size = anchor_max_pos - anchor_min_pos;
		//			// For now, treat rect_min as position; you can refine with pivot if needed.
		//			pos = anchor_min_pos;
		//		}

		//		rect.calculated_world_position = pos;
		//		rect.calculated_world_size = size;

		//		calculated_pos = pos;
		//		calculated_size = size;
		//	}

		//	// Traverse children using Hierarchy 
		//	if (registry.all_of<Entity::Hierarchy>(entity)) {
		//		const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
		//		for (const auto& childGUID : hierarchy.childrenGUIDs) {
		//			entt::entity child = ecs->resolveGUID(childGUID);
		//			if (child != entt::null && ecs->checkEntity(child)) {
		//				processHierarchy(child, registry, calculated_size, calculated_pos);
		//			}
		//		}
		//	}
		//}

		void LayoutSystem::processHierarchy(entt::entity entity,
			entt::registry& registry,
			const glm::vec2& parent_size,
			const glm::vec2& parent_pos)
		{
			auto svc = services.lock();
			auto ecs = svc->get<ECS::Controller>();
			auto asset_mgr = svc->get<Assets::Manager>();

			glm::vec2 calculated_pos = parent_pos;
			glm::vec2 calculated_size = parent_size;


			// 1) SCREEN-SPACE / CANVAS UI (HUD, buttons, d-pad, etc.)
			//    Any UIRectTransform that is NOT a UIFollowsWorldEntity
			if (registry.all_of<UIRectTransform>(entity) &&
				!registry.all_of<UIFollowsWorldEntity>(entity))
			{
				auto& rect = registry.get<UIRectTransform>(entity);

				glm::vec2 anchor_min_pos = parent_pos + rect.anchor_min * parent_size + rect.offset_min;
				glm::vec2 anchor_max_pos = parent_pos + rect.anchor_max * parent_size + rect.offset_max;

				glm::vec2 size;
				glm::vec2 pos;

				if (rect.anchor_min == rect.anchor_max)
				{
					// --- FIXED SIZE ELEMENTS ---

					// Start with whatever designer put in size_delta
					size = rect.size_delta;

					// If size_delta is "unset" (0,0), try to infer from Texture2D
					if (size == glm::vec2(0.0f) &&
						asset_mgr &&
						registry.all_of<Texture2D>(entity))
					{
						auto& texComp = registry.get<Texture2D>(entity);

						if (texComp.texture_guid.IsValid())
						{
							// Ask Asset Manager for the texture
							auto texOpt = asset_mgr->getAsset<Assets::Texture>(texComp.texture_guid);
							if (texOpt.has_value() && texOpt.value())
							{
								auto tex = texOpt.value();

								// Replace GetWidth()/GetHeight() with your actual API
								float w = static_cast<float>(tex->getWidth());
								float h = static_cast<float>(tex->getHeight());

								size = glm::vec2(w, h) * texComp.texture_scale;
							}
							else
							{
								// Fallback size if asset missing
								size = glm::vec2(100.0f, 100.0f);
							}
						}
						else
						{
							// No texture GUID, fallback
							size = glm::vec2(100.0f, 100.0f);
						}
					}

					// Anchor point in parent space
					glm::vec2 anchor_point =
						parent_pos +
						rect.anchor_min * parent_size +
						rect.anchored_position;

					// pivot is in [0,1] (0=bottom/left, 1=top/right)
					pos = anchor_point - rect.pivot * size;
				}
				else
				{
					// --- STRETCHED ELEMENTS ---
					size = anchor_max_pos - anchor_min_pos;
					pos = anchor_min_pos;
				}

				rect.calculated_world_position = pos;
				rect.calculated_world_size = size;

				calculated_pos = pos;
				calculated_size = size;


				// default pivot to center if uninitialized
				if (rect.pivot == glm::vec2(0.0f)) {
					rect.pivot = glm::vec2(0.5f, 0.5f);
				}

				// Treat as a fixed-size element.
				// size_delta is the width/height in pixels.
				size = rect.size_delta;

				// Anchor point in parent space:
				// - anchor_min picks a point in the parent rect (0..1)
				// - anchored_position is an offset from that point
				glm::vec2 anchor_point =
					parent_pos + rect.anchor_min * parent_size + rect.anchored_position;

				// Move from anchor point to bottom-left using pivot (0..1)
				pos = anchor_point - rect.pivot * size;

				rect.calculated_world_position = pos;
				rect.calculated_world_size = size;

				calculated_pos = pos;
				calculated_size = size;
			}
			// 2) WORLD-FOLLOW LABELS (e.g. “Press E”)
			else if (registry.all_of<UIRectTransform>(entity) &&
				registry.all_of<UIFollowsWorldEntity>(entity))
			{
				auto& rect = registry.get<UIRectTransform>(entity);

				// updateFloatingLabels has already written a screen position
				calculated_pos = rect.calculated_world_position;
				calculated_size = rect.calculated_world_size;
			}
			// 3) Non-UI entities: just propagate the parent rect
			else
			{
				calculated_pos = parent_pos;
				calculated_size = parent_size;
			}

			// Recurse into children
			if (registry.all_of<Entity::Hierarchy>(entity))
			{
				const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
				for (const auto& childGUID : hierarchy.childrenGUIDs)
				{
					entt::entity child = ecs->resolveGUID(childGUID);
					if (child != entt::null && ecs->checkEntity(child))
					{
						processHierarchy(child, registry, calculated_size, calculated_pos);
					}
				}
			}
		}



		// Check if position is behind camera
		bool LayoutSystem::isPositionBehindCamera(const glm::vec4& clip_space_pos)
		{
			return clip_space_pos.w <= 0.0f;
		}

		// Check if NDC coordinates are within camera frustum
		bool LayoutSystem::isInCameraFrustum(const glm::vec3& ndc)
		{
			return ndc.x >= -1.0f && ndc.x <= 1.0f &&
				ndc.y >= -1.0f && ndc.y <= 1.0f &&
				ndc.z >= -1.0f && ndc.z <= 1.0f;
		}

		// Convert world position to clip space
		glm::vec4 LayoutSystem::worldToClipSpace(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& proj)
		{
			return proj * view * glm::vec4(world_pos, 1.0f);
		}

		// Convert clip space to NDC
		glm::vec3 LayoutSystem::clipToNDC(const glm::vec4& clip)
		{
			if (clip.w == 0.0f) return glm::vec3(0.0f);
			return glm::vec3(clip) / clip.w;
		}

		// Convert NDC to screen coordinates
		glm::vec2 LayoutSystem::ndcToScreen(const glm::vec3& ndc, const glm::vec2& viewport)
		{
			glm::vec2 screen;
			screen.x = (ndc.x * 0.5f + 0.5f) * viewport.x;
			// Alternate between these 2 screen y, first one inverts y,, having some issues with this, so we go with the second one whr it sticks above the entity
			//screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport.y;
			screen.y = ((ndc.y * 0.5f + 0.5f)) * viewport.y;
			return screen;
		}

		// Main world to screen function
		glm::vec2 LayoutSystem::worldToScreen(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& proj, const glm::vec2& viewport)
		{
			glm::vec4 clip = worldToClipSpace(world_pos, view, proj);

			if (isPositionBehindCamera(clip)) {
				return { -10000, -10000 };
			}

			glm::vec3 ndc = clipToNDC(clip);
			return ndcToScreen(ndc, viewport);
		}

		// Check if screen position is within viewport bounds
		bool LayoutSystem::isScreenPosVisible(const glm::vec2& screen_pos, const glm::vec2& viewport, float margin = 0.0f)
		{
			return screen_pos.x >= -margin && screen_pos.x <= viewport.x + margin &&
				screen_pos.y >= -margin && screen_pos.y <= viewport.y + margin;
		}

		// Calculate world position of entity with offset
		glm::vec3 LayoutSystem::getEntityWorldPosition(const WorldTransform& transform, const glm::vec3& offset)
		{
			return glm::vec3(transform.matrix * glm::vec4(offset, 1.0f));
		}

		// Update all floating labels
		void LayoutSystem::updateFloatingLabels(entt::registry& registry, const glm::mat4& view, const glm::mat4& proj, const glm::vec2& viewport)
		{
			auto view_floating = registry.view<UIFollowsWorldEntity, UIRectTransform, UIElement>();
			auto svc = services.lock();
			auto ecs = svc->get<ECS::Controller>();

			for (auto&& [entity, follows, rect, elem] : view_floating.each()) {
				entt::entity follow_entity = ecs->resolveGUID(follows.entity_target_guid);

				if (follow_entity == entt::null || !registry.valid(follow_entity)) {
					elem.b_is_enabled = false;
					continue;
				}

				if (!registry.all_of<WorldTransform>(follow_entity)) {
					elem.b_is_enabled = false;
					continue;
				}

				const auto& world_transform = registry.get<WorldTransform>(follow_entity);
				glm::vec3 world_pos = getEntityWorldPosition(world_transform, follows.world_offset);

				glm::vec4 clip = worldToClipSpace(world_pos, view, proj);

				// Check if behind camera
				if (isPositionBehindCamera(clip)) {
					elem.b_is_enabled = false;
					continue;
				}

				glm::vec3 ndc = clipToNDC(clip);

				// Check if within camera frustum (FOV check)
				if (!isInCameraFrustum(ndc)) {
					elem.b_is_enabled = false;
					continue;
				}

				glm::vec2 screen_pos = ndcToScreen(ndc, viewport);

				// Final viewport bounds check
				if (!isScreenPosVisible(screen_pos, viewport, 50.0f)) {
					elem.b_is_enabled = false;
				}
				else {
					elem.b_is_enabled = true;
					rect.calculated_world_position = screen_pos;
				}
			}
		}

	}
}
