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
					// Skip children, they'll be processed recursively
					continue;  
				}

				// Check layer visibility if canvas has a layer
				if (registry.all_of<Entity::Layer>(entity)) {
					const auto& layerComp = registry.get<Entity::Layer>(entity);
					auto scn_service = services.lock()->get<Scene::SceneManager>();
					if (!scn_service->isLayerEnabled(layerComp.layer_id)) {
						// Skip entire canvas hierarchy if layer is disabled
						continue;  
					}
				}

				// Calculate canvas position and size
				glm::vec2 canvas_pos(0, 0);
				glm::vec2 canvas_size = screen_size;

				// For WorldSpace mode, position is from world transform
				if (ui_canvas.render_mode == CanvasRenderMode::WorldSpace) {
					if (registry.all_of<WorldTransform>(entity)) {
						const auto& wtrans = registry.get<WorldTransform>(entity);
						canvas_pos = glm::vec2(wtrans.matrix[3].x, wtrans.matrix[3].y);
						// Optionally scale canvas size by world_scale
						canvas_size *= ui_canvas.world_scale;
					}
				}

				// Set canvas's own rect transform (so it knows its bounds)
				ui_rect_trans.calculated_world_position = canvas_pos;
				ui_rect_trans.calculated_world_size = canvas_size;

				// Process children with canvas as parent
				processHierarchy(entity, registry, canvas_size, canvas_pos);
			}

		}


		void LayoutSystem::processHierarchy(entt::entity entity, entt::registry& registry,
			const glm::vec2& parent_size, const glm::vec2& parent_pos)
		{
			auto svc = services.lock();
			auto ecs = svc->get<ECS::Controller>();

			if (!registry.all_of<UIRectTransform>(entity)) {
				return;
			}

			if (registry.all_of<UIFollowsWorldEntity>(entity)) {
				return;
			}

			auto& rect = registry.get<UIRectTransform>(entity);

			// Calculate anchor points in parent space
			glm::vec2 anchor_min_pos = parent_pos + rect.anchor_min * parent_size;
			glm::vec2 anchor_max_pos = parent_pos + rect.anchor_max * parent_size;

			// Calculate size based on anchors
			glm::vec2 calculated_size;
			glm::vec2 calculated_pos;

			if (rect.anchor_min == rect.anchor_max) {
				// Anchors together - use size_delta as absolute size
				calculated_size = rect.size_delta;
				calculated_pos = anchor_min_pos + rect.anchored_position;
			}
			else {
				// Anchors apart - stretch between them with offsets
				calculated_size = anchor_max_pos - anchor_min_pos +
					glm::vec2(rect.offset_max.x - rect.offset_min.x,
						rect.offset_max.y - rect.offset_min.y);
				calculated_pos = anchor_min_pos + rect.offset_min;
			}

			// Apply pivot (adjust position based on pivot point)
			calculated_pos -= rect.pivot * calculated_size;

			// Apply local position, rotation, scale
			calculated_pos += glm::vec2(rect.local_position);
			calculated_size *= glm::vec2(rect.scale);

			// Write back calculated values
			rect.calculated_world_position = calculated_pos;
			rect.calculated_world_size = calculated_size;

			// Process children
			if (registry.all_of<Entity::Hierarchy>(entity)) {
				const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
				for (const auto& childGUID : hierarchy.childrenGUIDs) {
					entt::entity child = ecs->resolveGUID(childGUID);
					if (child != entt::null && ecs->checkEntity(child)) {
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
			auto view_floating = registry.group<UIFollowsWorldEntity>(entt::get<UIRectTransform, UIElement>);
			auto svc = services.lock();
			auto ecs = svc->get<ECS::Controller>();

			// Get camera position for distance calculation
			auto scene_service = svc->get<Scene::SceneManager>();
			Camera* cam = scene_service->GetActiveCamera();
			glm::vec3 cam_pos = cam->pos;

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

					// Calculate distance-based scale
					float distance = glm::length(world_pos - cam_pos);

					// Option 1: Perspective-correct scaling (text stays consistent size)
					// Use the clip.w (depth) for perspective-correct scaling
					//float scale_factor = 1.0f / (clip.w * 0.01f);  // Adjust 0.01f for desired scaling

					// Option 2: Simple distance-based scaling
					const float reference_distance = 10.0f;
					float scale_factor = reference_distance / distance;

					// Clamp to reasonable range
					scale_factor = glm::clamp(scale_factor, 0.2f, 3.0f);

					// Store scale in rect for renderer to use
					rect.scale = glm::vec3(scale_factor, scale_factor, 1.0f);
				}
			}
		}


	}
}
