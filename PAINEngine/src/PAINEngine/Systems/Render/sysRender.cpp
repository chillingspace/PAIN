#include "sysRender.h"
#include "pch.h"

#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Components/AllComponents.h"
#include "ECS/Components/cAnimation.h"
#include "ECS/Components/cCompoundCollider.h" // For CompoundCollider visualization
#include "ECS/Components/cPhysics.h"		  // For rendering RigidBody3D
#include "ECS/Controller.h"
#include "Systems/Collision/sBVHSystem.h"

#ifdef _DEBUG
#include "LayeredSystems/LevelEditor/Editor.h"
#endif
#include <Systems/UI/sysUIInput.h>

namespace PAIN {
	namespace Render {

		System::System(std::shared_ptr<Services> svc) : ISystem(svc) {

			PN_CORE_INFO("[RenderSystem] Initialized");
		}

		void System::InitializeModelRenderer(entt::entity entity,
											 ModelRenderer& component) {

			// Get asset manager
			auto assetManager = services.lock()->get<Assets::Manager>();

			// Check for valid GUID
			if (!component.modelGUID.IsValid())
				return;

			// Set prev GUID
			component.prevModelGUID = component.modelGUID;

			// Initiate scene VBO update
			services.lock()->get<sRenderer>()->initSceneVbo();

			// Boolean for checking for material updates
			bool material_updates = false;
			if (component.cachedModelAsset || component.materials.empty())
				material_updates = true;

			// optional material asset
			auto model_opt = assetManager->getAsset<Assets::Model>(component.modelGUID);

			// Load model asset
			component.cachedModelAsset =
				model_opt.has_value() ? model_opt.value() : nullptr;
			if (!component.cachedModelAsset) {
				PN_CORE_ERROR("Failed to load model asset for entity {}", (uint32_t)entity);
				return;
			}

			// Cache model
			const auto& modelAsset = component.cachedModelAsset;

			// Update material list only when needed
			if (material_updates) {

				// Update new materials
				component.materials.clear();
				component.materials.reserve(modelAsset->materials.size());
				for (const auto& materialPath : modelAsset->materials) {
					MaterialInstance matInstance;

					// optional material asset
					auto materialAssetOpt =
						assetManager->getAsset<Assets::Material>(materialPath);

					// Load material asset
					auto materialAsset =
						materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;

					// Check valid material asset
					if (materialAsset) {
						matInstance.materialGUID = materialAsset->guid;

						// Init material overrides
						matInstance.albedoTextureOverride =
							assetManager->findGUID(materialAsset->albedoTexturePath);
						matInstance.normalTextureOverride =
							assetManager->findGUID(materialAsset->normalTexturePath);
						matInstance.metallicTextureOverride =
							assetManager->findGUID(materialAsset->metallicTexturePath);
						matInstance.roughnessTextureOverride =
							assetManager->findGUID(materialAsset->roughnessTexturePath);
						matInstance.aoTextureOverride =
							assetManager->findGUID(materialAsset->aoTexturePath);
						matInstance.emissiveTextureOverride =
							assetManager->findGUID(materialAsset->emissiveTexturePath);
						matInstance.heightTextureOverride =
							assetManager->findGUID(materialAsset->heightTexturePath);
						matInstance.opacityTextureOverride =
							assetManager->findGUID(materialAsset->opacityTexturePath);
						matInstance.baseColorOverride = materialAsset->baseColor;
						matInstance.metallicOverride = materialAsset->metallic;
						matInstance.roughnessOverride = materialAsset->roughness;
						matInstance.emissiveOverride = materialAsset->emissive;
					}

					component.materials.push_back(std::move(matInstance));
				}
			}

			// ========================================
			// PERFORMANCE OPTIMIZATION: Cache Texture Handles
			// ========================================
			// Cache GL texture handles to avoid AssetManager lookups every frame in
			// DrawGeometry
			component.submeshCaches.clear();
			component.submeshCaches.resize(modelAsset->submeshes.size());

			for (size_t i = 0; i < modelAsset->submeshes.size(); ++i) {
				const auto& submesh = modelAsset->submeshes[i];

				if (submesh.materialIndex >= component.materials.size()) {
					continue;
				}

				auto& cache = component.submeshCaches[i];
				MaterialInstance* material = &component.materials[submesh.materialIndex];

				// Look up material asset once
				auto materialAssetOpt =
					assetManager->getAsset<Assets::Material>(material->materialGUID);
				auto materialAsset =
					materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;

				if (materialAsset) {
					// Cache base material properties
					cache.baseColor = material->useOverrides ? material->baseColorOverride
															 : materialAsset->baseColor;
					cache.metallic = material->useOverrides ? material->metallicOverride
															: materialAsset->metallic;
					cache.roughness = material->useOverrides ? material->roughnessOverride
															 : materialAsset->roughness;

					// Helper lambda to get GL texture handle from GUID
					auto getTextureHandle = [&](const Assets::GUID& guid) -> GLuint {
						if (!guid.IsValid())
							return 0;
						auto texOpt = assetManager->getAsset<Assets::Texture>(guid);
						return (texOpt.has_value() && texOpt.value())
								   ? texOpt.value()->gl_texture
								   : 0;
					};

					// Cache all texture handles (look up ONCE during initialization)
					cache.albedoTexture = getTextureHandle(
						material->useOverrides
							? material->albedoTextureOverride
							: assetManager->findGUID(materialAsset->albedoTexturePath));

					cache.normalTexture = getTextureHandle(
						material->useOverrides
							? material->normalTextureOverride
							: assetManager->findGUID(materialAsset->normalTexturePath));

					cache.metallicTexture = getTextureHandle(
						material->useOverrides
							? material->metallicTextureOverride
							: assetManager->findGUID(materialAsset->metallicTexturePath));

					cache.roughnessTexture = getTextureHandle(
						material->useOverrides
							? material->roughnessTextureOverride
							: assetManager->findGUID(materialAsset->roughnessTexturePath));

					cache.aoTexture = getTextureHandle(
						material->useOverrides
							? material->aoTextureOverride
							: assetManager->findGUID(materialAsset->aoTexturePath));

					cache.emissiveTexture = getTextureHandle(
						material->useOverrides
							? material->emissiveTextureOverride
							: assetManager->findGUID(materialAsset->emissiveTexturePath));

					cache.opacityTexture = getTextureHandle(
						material->useOverrides
							? material->opacityTextureOverride
							: assetManager->findGUID(materialAsset->opacityTexturePath));

					cache.cacheValid = true;
				}
			}

			// Initialize bone transforms if animated
			if (!modelAsset->skeleton.empty() && component.boneTransforms.empty()) {
				component.boneTransforms.resize(modelAsset->skeleton.size(),
												glm::mat4(1.0f));
			}

			// Initialize morph weights
			if (!modelAsset->morphTargets.empty() && component.morphWeights.empty()) {
				component.morphWeights.resize(modelAsset->morphTargets.size(), 0.0f);
			}

			// PN_CORE_INFO("[Render System] Initialized ModelRenderer for entity {} with
			// {} submeshes", (uint32_t)entity, component.materials.size());
		}

		void System::onUpdate(AppTiming timing, entt::registry& registry) {
			{
#ifdef _DEBUG
				auto editor = services.lock()->get<Editor::Editor>();
				bool editor_visible = editor && editor->isVisible();
				int editor_debug_mode = editor ? editor->getDebugMode() : 0;

#else
				bool editor_visible = false;
				int editor_debug_mode = 0;
#endif

				GLenum err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err on update loop begin: {}", err);
				}

				if (editor_visible) {
					glBindFramebuffer(GL_FRAMEBUFFER,
									  services.lock()->get<sRenderer>()->getFinalFbo());
					// glViewport(0, 0, fbWidth, fbHeight);
				} else {
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					// Match viewport to window size
					auto window = services.lock()->get<Window::Window>();
					auto frame_buffer = window->getFrameBuffer();
					glViewport(0, 0, frame_buffer.x, frame_buffer.y);
				}

				// Clear
				glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err before render passes: {}", err);
				}

				// Render all passes
				shadowPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after shadow pass: {}", err);
				}
				geometryPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after geometry pass: {}", err);
				}
				reflectionPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after reflection pass: {}", err);
				}
				lightingPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after lighting pass: {}", err);
				}
				debugPass(registry, editor_debug_mode);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after debug pass: {}", err);
				}
				uiPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after UI pass: {}", err);
				}
				services.lock()->get<sRenderer>()->postProcessPass();
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after post process pass: {}", err);
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset
			}

			// set cam light to cam
			auto olcam = LightSources::get().get("cam");
			Light& lcam = olcam.value();
			lcam.position =
				services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->pos;
			lcam.position.y += 0.1f; // light on camera = grainy
			lcam.fov =
				services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->fov;
			lcam.direction =
				services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->forward;
			lcam.aspect_ratio = services.lock()
									->get<Scene::SceneManager>()
									->GetActiveCamera()
									->aspect_ratio;

			GLenum err = glGetError();
			while (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err on update loop end: {}", err);
			}
		}

		void System::onFixedUpdate(AppTiming timing, entt::registry& registry) {
			// Rendering doesn't need fixed updates
		}

		void System::onEvent(Event::Event& e) {
			// Handle render-related events if needed
		}

		void System::shadowPass(entt::registry& registry) {
			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(),
					   GraphicsSettings::get().getShadowMapWidth());

			auto renderGroup =
				registry.group<ModelRenderer>(entt::get<WorldTransform, Entity::Layer>);

			for (const Light& l : LightSources::get().getAll()) {
				if (l.getShadowType() != Light::SHADOW_TYPES::MAPPED)
					continue;

				rendererService->w_renderer->BeginShadowPass(l);

				for (auto [entity, model, transform, layer] : renderGroup.each()) {

					glm::mat4 model_xform = transform.matrix;

					if (model.visible && model.castShadows) {
						rendererService->w_renderer->DrawShadows(model, model_xform, l);
					}
				}

				rendererService->w_renderer->EndShadowPass();
			}
		}

		void System::geometryPass(entt::registry& registry) {

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			auto svc = services.lock();
			if (!svc)
				return;

			auto sceneManager = svc->get<Scene::SceneManager>();
			if (!sceneManager)
				return;

			const auto& layers = sceneManager->getLayers();
			auto renderGroup =
				registry.group<ModelRenderer>(entt::get<WorldTransform, Entity::Layer>);

			if (!rendererService->m_Scene) {
				return;
			}

			rendererService->w_renderer->BeginGeometryPass(rendererService->m_Scene);

			// Use structured bindings with .each() for proper group iteration
			for (auto [entity, model, transform, layer] : renderGroup.each()) {
				// Components guaranteed to exist by group - no try_get needed!
				if (!model.visible)
					continue;

				// Check layer visibility (not part of group, so still need try_get)
				int layerID = layer.layer_id;
				if (layerID < layers.size() && !layers[layerID].enabled) {
					continue;
				}

				glm::mat4 model_xform = transform.matrix;

				// Initialize component if model GUID changed
				if (model.modelGUID != model.prevModelGUID) {
					InitializeModelRenderer(entity, const_cast<ModelRenderer&>(model));
				}

				if (!model.visible) {
					continue;
				}

				// Draw geometry, passing animation pointer (may be null)
				rendererService->w_renderer->DrawGeometry(
					rendererService->m_Scene, const_cast<ModelRenderer&>(model),
					model_xform);
			}

			rendererService->w_renderer->EndGeometryPass();
		}

		void System::reflectionPass(entt::registry& registry) {

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			auto renderGroup =
				registry.group<ModelRenderer>(entt::get<WorldTransform, Entity::Layer>);

			for (auto [entity, model, transform, layer] : renderGroup.each()) {
				rendererService->w_renderer->ReflectionPass(model);
			}
		}

		void System::lightingPass(entt::registry& registry) {

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			auto svc = services.lock();
			if (!svc)
				return;

			auto lightingGroup = registry.group<Lighting>(entt::get<LocalTransform>);

			// Cache to track which lights are still active this frame
			std::unordered_set<std::string> activeLightNames;

			for (auto [entity, lighting, transform] : lightingGroup.each()) {

				std::string lightName = "light_" + std::to_string((uint32_t)entity);
				activeLightNames.insert(lightName);

				// Create light if new
				if (!LightSources::get().get(lightName)) {
					LightSources::get().create(lightName);
				}

				if (auto lightOpt = LightSources::get().get(lightName)) {
					Light& light = lightOpt.value();
					light.position = transform.position + lighting.offset;
					light.L_intensity = lighting.light_intensity;
					light.type = static_cast<Light::TYPES>(lighting.light_type);
					light.setShadowType(
						static_cast<Light::SHADOW_TYPES>(lighting.shadow_type));

					// works for non point light
					light.direction = lighting.direction;

					// for spotlight only
					light.inner_angle = lighting.inner_angle;
					light.outer_angle = lighting.outer_angle;
				}
			}

			// Remove lights no longer active
			for (auto& [key, lightRef] : LightSources::get().getAllWithKeys()) {
				const std::string& name = key;

				// Skip the special lights "cam" and "world"
				if (name == "cam" || name == "world") {
					continue;
				}

				if (activeLightNames.find(name) == activeLightNames.end()) {
					LightSources::get().destroy(name);
				}
			}

			auto scene = svc->get<Scene::SceneManager>();
			rendererService->w_renderer->LightingPass(scene, LightSources::get());
		}

		void System::debugPass(entt::registry& registry, int debug_mode) {

			auto rendererService = services.lock()->get<sRenderer>();
			if (debug_mode == 0 || !rendererService || !rendererService->w_renderer) {
				return;
			}

			auto svc = services.lock();
			if (!svc)
				return;

			auto scene = svc->get<Scene::SceneManager>();
			if (!scene)
				return;

			Camera* camera = scene->GetActiveCamera();
			if (!camera)
				return;

			// Mode 1: Draw Physics Colliders (Cyan for basic, different colors for
			// compound shapes)
			if (debug_mode == 1) {
				// Iterate entities with both RigidBody3D and Transform
				auto view = registry.view<Physics::RigidBody3D, LocalTransform>();

				for (auto [entity, rb, trans] : view.each()) {
					// Check if entity has CompoundCollider
					auto* compCollider = registry.try_get<CompoundCollider>(entity);

					if (compCollider && compCollider->useCompoundCollider &&
						!compCollider->shapes.empty()) {
						// Draw each compound sub-shape with alternating colors
						int shapeIndex = 0;
						for (const auto& shape : compCollider->shapes) {
							// Pick color based on shape index
							glm::vec4 color;
							switch (shapeIndex % 3) {
							case 0:
								color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
								break; // Cyan
							case 1:
								color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
								break; // Magenta
							case 2:
								color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
								break; // Yellow
							}

							// Get shape dimensions based on type
							glm::vec3 half_extents;
							if (shape.type == ColliderShapeType::Box) {
								half_extents =
									shape.boxHalfExtents * trans.scale * rb.collider_scale;
							} else if (shape.type == ColliderShapeType::Sphere) {
								float scaledRadius =
									shape.sphereRadius *
									glm::max(trans.scale.x, glm::max(trans.scale.y, trans.scale.z));
								half_extents = glm::vec3(scaledRadius);
							} else if (shape.type == ColliderShapeType::Capsule) {
								float scaledRadius =
									shape.capsuleRadius * glm::max(trans.scale.x, trans.scale.z);
								float scaledHeight = shape.capsuleHalfHeight * trans.scale.y;
								half_extents = glm::vec3(scaledRadius, scaledHeight + scaledRadius,
														 scaledRadius);
							} else {
								half_extents = glm::vec3(0.5f) * trans.scale;
							}

							// Calculate scaled offset (note: offset does NOT scale by
							// collider_scale per physics engine)
							glm::vec3 scaled_offset = shape.offset * trans.scale;

							// Entity rotation combined with shape local rotation
							glm::quat entity_rot = glm::normalize(trans.rotation);
							glm::quat shape_rot = glm::normalize(shape.rotation);
							glm::quat combined_rot = entity_rot * shape_rot;

							// Check if entity has any rotation (non-identity quaternion)
							bool hasRotation = glm::abs(trans.rotation.w - 1.0f) > 0.0001f ||
											   glm::abs(trans.rotation.x) > 0.0001f ||
											   glm::abs(trans.rotation.y) > 0.0001f ||
											   glm::abs(trans.rotation.z) > 0.0001f;

							if (hasRotation) {
								// Calculate 8 corners of OBB for oriented visualization
								// Order must match edge list: back face (0,1,2,3), front face
								// (4,5,6,7)
								glm::vec3 he = half_extents;
								glm::vec3 local_corners[8] = {
									glm::vec3(-he.x, -he.y, -he.z), // 0: back-bottom-left
									glm::vec3(+he.x, -he.y, -he.z), // 1: back-bottom-right
									glm::vec3(+he.x, +he.y, -he.z), // 2: back-top-right
									glm::vec3(-he.x, +he.y, -he.z), // 3: back-top-left
									glm::vec3(-he.x, -he.y, +he.z), // 4: front-bottom-left
									glm::vec3(+he.x, -he.y, +he.z), // 5: front-bottom-right
									glm::vec3(+he.x, +he.y, +he.z), // 6: front-top-right
									glm::vec3(-he.x, +he.y, +he.z), // 7: front-top-left
								};

								glm::vec3 corners[8];
								glm::vec3 rotated_offset = entity_rot * scaled_offset;
								for (int i = 0; i < 8; ++i) {
									glm::vec3 rotated_corner = combined_rot * local_corners[i];
									corners[i] = trans.position + rotated_offset + rotated_corner;
								}

								// Use OBB rendering for rotated entities
								rendererService->w_renderer->DebugPassOBB(corners, color, scene);
							} else {
								// No rotation - use faster AABB path
								glm::vec3 center = trans.position + scaled_offset;
								glm::vec3 min_aabb = center - half_extents;
								glm::vec3 max_aabb = center + half_extents;
								rendererService->w_renderer->DebugPass(min_aabb, max_aabb, color,
																	   scene);
							}
							shapeIndex++;
						}
					} else {
						// Fall back to basic RigidBody3D collider (original behavior)
						glm::vec4 color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan for Physics

						glm::vec3 offset_scaled = trans.scale * rb.collider_offset;
						glm::vec3 half_extents = 0.5f * trans.scale * rb.collider_scale;
						glm::quat rotation = glm::normalize(trans.rotation);

						// Check if entity has any rotation (non-identity quaternion)
						bool hasRotation = glm::abs(trans.rotation.w - 1.0f) > 0.0001f ||
										   glm::abs(trans.rotation.x) > 0.0001f ||
										   glm::abs(trans.rotation.y) > 0.0001f ||
										   glm::abs(trans.rotation.z) > 0.0001f;

						if (hasRotation) {
							// Calculate 8 corners of OBB for oriented visualization
							// Order must match edge list: back face (0,1,2,3), front face
							// (4,5,6,7)
							glm::vec3 he = half_extents;
							glm::vec3 local_corners[8] = {
								glm::vec3(-he.x, -he.y, -he.z), // 0: back-bottom-left
								glm::vec3(+he.x, -he.y, -he.z), // 1: back-bottom-right
								glm::vec3(+he.x, +he.y, -he.z), // 2: back-top-right
								glm::vec3(-he.x, +he.y, -he.z), // 3: back-top-left
								glm::vec3(-he.x, -he.y, +he.z), // 4: front-bottom-left
								glm::vec3(+he.x, -he.y, +he.z), // 5: front-bottom-right
								glm::vec3(+he.x, +he.y, +he.z), // 6: front-top-right
								glm::vec3(-he.x, +he.y, +he.z), // 7: front-top-left
							};

							glm::vec3 corners[8];
							for (int i = 0; i < 8; ++i) {
								glm::vec3 body_space_point = offset_scaled + local_corners[i];
								corners[i] = trans.position + (rotation * body_space_point);
							}

							// Use OBB rendering for rotated entities
							rendererService->w_renderer->DebugPassOBB(corners, color, scene);
						} else {
							// No rotation - use faster AABB path
							glm::vec3 center = trans.position + offset_scaled;
							glm::vec3 min_aabb = center - half_extents;
							glm::vec3 max_aabb = center + half_extents;
							rendererService->w_renderer->DebugPass(min_aabb, max_aabb, color,
																   scene);
						}
					}
				}
			}

			// Mode 2: Draw BVH Tree Nodes
			if (debug_mode == 2) {
				auto ecsController = svc->get<ECS::Controller>();
				if (!ecsController)
					return;

				auto bvhSystem = ecsController->getSystem<sBVHSystem>();
				if (!bvhSystem) {
					PN_CORE_WARN("DebugPass skipped: Missing BVH System.");
					return;
				}

				const BVH& bvh = bvhSystem->getBVH();
				const auto& nodes = bvh.getNodes();
				int rootIndex = bvh.getRootIndex();

				std::function<void(int nodeIndex, int depth)> drawNodeRecursive =
					[&](int nodeIndex, int depth) {
						if (nodeIndex == -1 || nodeIndex >= nodes.size() ||
							nodes[nodeIndex].height == -1)
							return;

						const BVHNode& node = nodes[nodeIndex];
						glm::vec4 nodeColor;

						if (node.isLeaf()) {
							nodeColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for leaves
						} else {
							int colorIndex = depth % 6;
							switch (colorIndex) {
							case 0:
								nodeColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
								break; // Red
							case 1:
								nodeColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
								break; // Orange
							case 2:
								nodeColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
								break; // Yellow
							case 3:
								nodeColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
								break; // Cyan
							case 4:
								nodeColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
								break; // Blue
							default:
								nodeColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
								break; // Magenta
							}
						}

						rendererService->w_renderer->DebugPass(node.aabb.min, node.aabb.max,
															   nodeColor, scene);

						if (!node.isLeaf()) {
							drawNodeRecursive(node.child1Index, depth + 1);
							drawNodeRecursive(node.child2Index, depth + 1);
						}
					};

				if (rootIndex != -1) {
					drawNodeRecursive(rootIndex, 0);
				}
			}

			// Mode 3: Draw Original World AABBs from cBoundingVolume (Red)
			if (debug_mode == 3) {
				auto view = registry.view<BoundingVolume>();
				glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for Visual AABBs

				for (auto entity : view) {
					auto& bounding_vol = view.get<BoundingVolume>(entity);
					rendererService->w_renderer->DebugPass(
						bounding_vol.worldAABB.min, bounding_vol.worldAABB.max, color, scene);
				}
			}
		}

		void System::uiPass(entt::registry& registry) {

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			// Texture and text groups
			auto texture_group =
				registry.group<Texture2D>(entt::get<UIElement, UIRectTransform>);
			auto text_group =
				registry.group<UIElement>(entt::get<UIText, UIRectTransform>);
			auto scn_service = services.lock()->get<Scene::SceneManager>();

			// ========================================
			// CLEAR ANY PRE-EXISTING ERRORS
			// ========================================
			GLenum err;
			while ((err = glGetError()) != GL_NO_ERROR) {
				PN_CORE_WARN("[UI Pass] Clearing pre-existing GL error: 0x{:X}", err);
			}

			// ========================================
			// SET GL STATE ONCE FOR ENTIRE UI PASS
			// ========================================
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE); // UI usually doesn't need culling

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("[UI Pass] Error setting initial GL state: 0x{:X}", err);
			}

			for (auto [entity, texture_comp, ui_elem, rect_comp] : texture_group.each()) {
				// Layer check
				auto layerComp = registry.try_get<Entity::Layer>(entity);
				if (layerComp && !scn_service->isLayerEnabled(layerComp->layer_id)) {
					continue;
				}

				if (!ui_elem.b_is_enabled)
					continue;

				auto texture_opt =
					services.lock()->get<Assets::Manager>()->getAsset<Assets::Texture>(
						texture_comp.texture_guid);
				if (!texture_opt.has_value())
					continue;

				// Only auto-set size_delta if it's not already set
				if (rect_comp.size_delta.x == 0.0f || rect_comp.size_delta.y == 0.0f) {
					float width = static_cast<float>(texture_opt.value().get()->width);
					float height = static_cast<float>(texture_opt.value().get()->height);

					// If spritesheet, divide by columns/rows to get FRAME size
					if (registry.all_of<UIAnimation>(entity)) {
						const auto& anim = registry.get<UIAnimation>(entity);
						if (anim.spritesheet_columns > 0)
							width /= anim.spritesheet_columns;
						if (anim.spritesheet_rows > 0)
							height /= anim.spritesheet_rows;
					}

					rect_comp.size_delta.x = width;
					rect_comp.size_delta.y = height;
				}

				// Calculate UV transform
				glm::vec4 uv_transform(1.0f, 1.0f, 0.0f,
									   0.0f); // Default: scale(1,1), offset(0,0)

				auto uv_comp = registry.try_get<UVCoordinates>(entity);
				if (uv_comp) {
					float width = uv_comp->uv.z - uv_comp->uv.x;
					float height = uv_comp->uv.w - uv_comp->uv.y;
					uv_transform = glm::vec4(width, height, uv_comp->uv.x, uv_comp->uv.y);
				}

				rendererService->w_renderer->Render2DTexture(
					texture_opt.value()->gl_texture, texture_comp.pos,
					texture_comp.texture_scale, uv_transform);
			}

			// ========================================
			// RENDER TEXT
			// ========================================
			for (auto [entity, ui_elem, text_comp, rect_comp] : text_group.each()) {
				// Layer check
				auto layerComp = registry.try_get<Entity::Layer>(entity);
				if (layerComp && !scn_service->isLayerEnabled(layerComp->layer_id)) {
					continue;
				}

				if (!ui_elem.b_is_enabled)
					continue;

				auto font_opt =
					services.lock()
						->get<Assets::Manager>()
						->getAsset<Assets::Fonts::FontFace>(text_comp.font_guid);
				if (!font_opt.has_value())
					continue;

				// PN_CORE_INFO("[Render System] Rendering Font: {}",
				// font_opt.value()->name);

				// Only recalculate text_pos when layout changes
				if (rect_comp.layout_dirty) {
					text_comp.text_pos = rect_comp.calculated_world_position;
					text_comp.scale_factor = rect_comp.scale.x;
					rect_comp.layout_dirty = false;
				}

				TextRenderer::get().renderText(text_comp);

				// CHECK FOR ERRORS AFTER EACH TEXT RENDER
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR(
						"[Render System] GL error after rendering font '{}': 0x{:X}",
						font_opt.value()->name, err);
				}
			}

			// ========================================
			// DEBUG UI HITBOXES
			// ========================================
			auto& gs = GraphicsSettings::get();
			if (gs.DEBUG_DRAW_UI_HITBOXES) {
				// Draw hitboxes for entities with CustomHitbox2D
				auto hitbox_view = registry.view<CustomHitbox2D, Texture2D, UIElement, UIRectTransform>();
				for (auto [entity, hitbox, tex, element, rect] : hitbox_view.each()) {
					if (!element.b_is_enabled) continue;

					// Calculate hitbox bounds (same logic as in raycastUI)
					glm::vec2 hitbox_center = tex.pos + UI::normalizeSize(hitbox.position_offset, services);
					glm::vec2 normalized_min = UI::normalizeSize(hitbox.min_point, services);
					glm::vec2 normalized_max = UI::normalizeSize(hitbox.max_point, services);
					glm::vec2 rect_min = hitbox_center + normalized_min;
					glm::vec2 rect_max = hitbox_center + normalized_max;

					// Draw debug rectangle
					rendererService->w_renderer->DebugPass2D(rect_min, rect_max, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)); // Magenta
				}
			}

			// ========================================
			// RESTORE GL STATE ONCE AFTER UI PASS
			// ========================================
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glEnable(GL_CULL_FACE);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("[UI Pass] Error restoring GL state: 0x{:X}", err);
			}
		}

	} // namespace Render
} // namespace PAIN
