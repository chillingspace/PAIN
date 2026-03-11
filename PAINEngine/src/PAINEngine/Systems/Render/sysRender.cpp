#include "sysRender.h"
#include "pch.h"

#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Scene/Scene.h"	  
#include "ECS/Controller.h"
#include "Systems/Collision/sBVHSystem.h"
#include "ECS/sMetaData.h"
#include "Core.h"
#include "Systems/Particle/sysParticleSystem.h"
#include "CoreSystems/Assets/Types/Shader.h"
#include "CoreSystems/Assets/Types/Texture.h"

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

			// Initiate scene VBO update
			services.lock()->get<sRenderer>()->w_renderer->vboDirty = true;

			// Set prev GUID
			component.prevModelGUID = component.modelGUID;

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

				auto rendererService = services.lock()->get<sRenderer>();
				if (!rendererService || !rendererService->w_renderer) {
					return;
				}

				if (rendererService->w_renderer->resizeDirty) {
					rendererService->w_renderer->resizeDirty = false;
					rendererService->w_renderer->_initDeferredShadingBuffers();
				}

				if (editor_visible) {
					glBindFramebuffer(GL_FRAMEBUFFER,
									  rendererService->getFinalFbo());
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
				GraphicsSettings::get().stats.objects_culled = 0;
				GraphicsSettings::get().stats.objects_rendered = 0;
				GraphicsSettings::get().stats.shadow_objects_culled = 0;
				GraphicsSettings::get().stats.shadow_objects_rendered = 0;

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
				minimapPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after minimap pass: {}", err);
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
				volumetricPass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after volumetric pass: {}", err);
				}
				particlePass(registry);
				err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL err after particle pass: {}", err);
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
					
					Frustum lightFrustum = l.getFrustum();

					for (auto [entity, model, transform, layer] : renderGroup.each()) {

						glm::mat4 model_xform = transform.matrix;

						if (model.visible && model.castShadows) {
							// FRUSTUM CULLING FOR SHADOWS
							auto* boundingVol = registry.try_get<BoundingVolume>(entity);
							if (boundingVol && boundingVol->worldAABB.isValid()) {
								if (!isAABBInFrustum(boundingVol->worldAABB, lightFrustum)) {
									GraphicsSettings::get().stats.shadow_objects_culled++;
									continue; // skip shadow casting for this object, outside light frustum
								}
							}
						
							GraphicsSettings::get().stats.shadow_objects_rendered++;
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

			if (rendererService->w_renderer->vboDirty) {
				rendererService->w_renderer->vboDirty = false;
				rendererService->initSceneVbo();
			}

			auto sceneManager = svc->get<Scene::SceneManager>();
			if (!sceneManager)
				return;
				
			Camera* activeCam = sceneManager->GetActiveCamera();
			if (!activeCam)
				return;
				
			Frustum camFrustum = activeCam->getFrustum();

			const auto& layers = sceneManager->getLayers();
			auto renderGroup =
				registry.group<ModelRenderer>(entt::get<WorldTransform, Entity::Layer>);

			if (!rendererService->m_Scene) {
				return;
			}

			// Advance occlusion query double-buffer index before reading results this frame
			if (GraphicsSettings::get().occlusion_culling)
				rendererService->w_renderer->AdvanceOcclusionFrame();

			rendererService->w_renderer->BeginGeometryPass(rendererService->m_Scene);

			auto& gs = GraphicsSettings::get();
			const bool useInstancing = gs.use_instanced_rendering;

			// Instance groups: key -> (representative ModelRenderer*, [matrices])
			// Key = modelVPath + "|" + materialGUIDs (only non-animated, non-override objects)
			struct InstanceGroup {
				ModelRenderer* rep = nullptr;
				std::vector<glm::mat4> matrices;
			};
			std::unordered_map<std::string, InstanceGroup> instanceGroups;

			// Collect frustum-visible AABBs for occlusion query pass (ALL frustum-visible, even if occlusion-culled)
			std::vector<std::pair<entt::entity, BoundingVolume*>> frustumVisibleBVs;

			// Use structured bindings with .each() for proper group iteration
			for (auto [entity, model, transform, layer] : renderGroup.each()) {
				// Components guaranteed to exist by group - no try_get needed!
				if (!model.visible)
					continue;

				// Check layer visibility (not part of group, so still need try_get)
				int layerID = layer.layer_id;
				if (layerID < (int)layers.size() && !layers[layerID].enabled) {
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

				// FRUSTUM CULLING
				auto* boundingVol = registry.try_get<BoundingVolume>(entity);
				if (boundingVol && boundingVol->worldAABB.isValid()) {
					if (!isAABBInFrustum(boundingVol->worldAABB, camFrustum)) {
						gs.stats.objects_culled++;
						continue; // skip rendering this object, it's outside the frustum
					}
				}

				// Collect for occlusion query pass (after depth buffer is populated)
				if (gs.occlusion_culling && boundingVol && boundingVol->worldAABB.isValid())
					frustumVisibleBVs.push_back({entity, boundingVol});

				// OCCLUSION CULLING (1-frame delayed query results)
				if (gs.occlusion_culling) {
					if (!rendererService->w_renderer->GetOcclusionResult(entity)) {
						gs.stats.occlusion_culled++;
						continue;
					}
				}

				gs.stats.objects_rendered++;

				// GPU INSTANCING: batch non-animated, non-override objects by model+material key
				const bool canInstance = useInstancing
					&& model.boneTransforms.empty()
					&& model.cachedModelAsset
					&& model.bufferOffset.isUploaded;

				if (canInstance) {
					bool hasOverride = false;
					std::string key = model.cachedModelAsset->vpath;
					for (const auto& mat : model.materials) {
						if (mat.useOverrides) { hasOverride = true; break; }
						key += '|';
						key += mat.materialGUID.ToString(false);
					}
					if (!hasOverride) {
						auto& grp = instanceGroups[key];
						if (!grp.rep) grp.rep = &const_cast<ModelRenderer&>(model);
						grp.matrices.push_back(model_xform);
						continue; // will be drawn in instanced batch below
					}
				}

				// Non-instanced fallback
				rendererService->w_renderer->DrawGeometry(
					rendererService->m_Scene, const_cast<ModelRenderer&>(model),
					model_xform);
			}

			// Flush instanced batches
			for (auto& [key, grp] : instanceGroups) {
				if (grp.rep && !grp.matrices.empty()) {
					rendererService->w_renderer->DrawGeometryInstanced(
						rendererService->m_Scene, *grp.rep, grp.matrices);
				}
			}

			// Issue occlusion queries while the G-buffer FBO is still bound (correct depth buffer)
			if (gs.occlusion_culling && !frustumVisibleBVs.empty()) {
				std::vector<std::pair<entt::entity, AABB>> queryList;
				queryList.reserve(frustumVisibleBVs.size());
				for (auto [e, bv] : frustumVisibleBVs)
					queryList.push_back({e, bv->worldAABB});
				rendererService->w_renderer->OcclusionQueryPass(rendererService->m_Scene, queryList);
			}

			rendererService->w_renderer->EndGeometryPass();
		}

		void System::minimapPass(entt::registry& registry) {
			(void)registry;

			auto& gs = GraphicsSettings::get();
			if (!gs.minimap_enabled) {
				return;
			}

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer) {
				return;
			}

			auto svc = services.lock();
			if (!svc) {
				return;
			}

			if (!svc->get<Scene::SceneManager>() || !svc->get<MetaData::Service>()) {
				return;
			}

			rendererService->w_renderer->BeginMinimapPass(glm::mat4(1.0f), glm::mat4(1.0f));
			rendererService->w_renderer->EndMinimapPass();
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

			auto scene = svc->get<Scene::SceneManager>();
			if (!scene)
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
					// Use WorldTransform so the spotlight renders at the correct world
					// location even when the entity is a non-root child with a parent
					// that has a non-zero position. Falls back to LocalTransform for
					// root entities where WorldTransform may not yet be computed.
					glm::vec3 entityWorldPos = transform.position;
					if (auto* wt = registry.try_get<WorldTransform>(entity)) {
						entityWorldPos = glm::vec3(wt->matrix[3]);
					}
					light.position = entityWorldPos + lighting.offset;
					light.L_intensity = lighting.light_intensity;
					light.type = static_cast<Light::TYPES>(lighting.light_type);
					light.setShadowType(
						static_cast<Light::SHADOW_TYPES>(lighting.shadow_type));
					light.setShadowResolution(lighting.shadow_resolution);
					light.volumetric = lighting.volumetric;

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

			rendererService->w_renderer->LightingPass(scene, LightSources::get());
		}

		void System::volumetricPass(entt::registry& registry) {
			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			auto svc = services.lock();
			if (!svc) return;

			auto scene = svc->get<Scene::SceneManager>();
			rendererService->w_renderer->VolumetricPass(scene, LightSources::get());
		}

		// ============================================
		// PARTICLE RENDER PASS - GPU Instanced Rendering
		// ============================================
		void System::particlePass(entt::registry& registry) {
			auto logParticleGLError = [](const char* stage) {
				GLenum err = glGetError();
				while (err != GL_NO_ERROR) {
					PN_CORE_ERROR("ParticlePass GL error at {}: {}", stage, err);
					err = glGetError();
				}
			};

			while (glGetError() != GL_NO_ERROR) {}

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer)
				return;

			auto ecsController = services.lock()->get<ECS::Controller>();
			if (!ecsController)
				return;

			// Get particle system
			auto particleSystem = ecsController->getSystem<PAIN::ParticleSystem::System>();
			if (!particleSystem)
				return;

			// Get all entities with ParticleSystemComponent components
			auto particleView = registry.view<ParticleSystemComponent, LocalTransform>();

			// Check if there are any active particles
			bool hasActiveParticles = false;
			for (auto [entity, ps, transform] : particleView.each()) {
				auto* psInstance = particleSystem->GetParticleSystem(entity);
				if (psInstance && psInstance->GetPool().GetAliveCount() > 0) {
					hasActiveParticles = true;
					break;
				}
			}

			if (!hasActiveParticles)
				return;

			// Get camera for view/projection matrices
			auto sceneManager = services.lock()->get<Scene::SceneManager>();
			if (!sceneManager)
				return;
			Camera* activeCam = sceneManager->GetActiveCamera();
			if (!activeCam)
				return;

			// Get or create particle shader
			static std::shared_ptr<Assets::Shader> particleShader;
			static GLuint particleProgram = 0;
			static bool shaderLoaded = false;

			if (shaderLoaded && (!particleShader || !glIsProgram(particleProgram))) {
				shaderLoaded = false;
				particleProgram = 0;
				particleShader.reset();
			}

			if (!shaderLoaded) {
				// Try to load particle shader - using same pattern as WindowsRenderer
				auto assetManager = services.lock()->get<Assets::Manager>();
				if (assetManager) {
#ifdef PN_PLATFORM_WINDOWS
					auto shaderOpt = assetManager->getAsset<Assets::Shader>("engine\\shaders\\particle.vert");
#elif defined(PN_PLATFORM_ANDROID)
					auto shaderOpt = assetManager->getAsset<Assets::Shader>("engine\\shaders\\android_particle.vert");
#endif
					if (shaderOpt.has_value()) {
						particleShader = shaderOpt.value();
						particleProgram = particleShader->GetRendererID();
						shaderLoaded = true;
					}
				}
			}

			GLint linkStatus = GL_FALSE;
			if (particleProgram != 0 && glIsProgram(particleProgram)) {
				glGetProgramiv(particleProgram, GL_LINK_STATUS, &linkStatus);
			}

			if (particleProgram == 0 || !glIsProgram(particleProgram) || linkStatus != GL_TRUE) {
				if (particleProgram != 0 && glIsProgram(particleProgram)) {
					GLint logLen = 0;
					glGetProgramiv(particleProgram, GL_INFO_LOG_LENGTH, &logLen);
					if (logLen > 1) {
						std::string log(static_cast<size_t>(logLen), '\0');
						GLsizei written = 0;
						glGetProgramInfoLog(particleProgram, logLen, &written, log.data());
						PN_CORE_ERROR("ParticlePass shader link log: {}", log);
					}
				}
				// Shader not loaded yet - skip rendering
				PN_CORE_ERROR("ParticlePass shader program invalid/unlinked: {}", particleProgram);
				return;
			}

			// Billboard quad vertices (centered at origin, facing +Z)
			// These are per-vertex, not per-instance
			static const float quadVertices[] = {
				// positions        // texCoords
				-0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
				 0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
				 0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
				-0.5f,  0.5f, 0.0f,  0.0f, 1.0f
			};

			static const unsigned short quadIndices[] = {
				0, 1, 2,
				2, 3, 0
			};

			// Create or reuse VAO and buffers
			static GLuint quadVAO = 0;
			static GLuint quadVBO = 0;
			static GLuint quadEBO = 0;
			static GLuint instanceVBO = 0;
			static size_t instanceCapacity = 10000;

			if (quadVAO == 0) {
				// Create quad VAO
				glGenVertexArrays(1, &quadVAO);
				glGenBuffers(1, &quadVBO);
				glGenBuffers(1, &quadEBO);
				glGenBuffers(1, &instanceVBO);

				// Bind quad VAO
				glBindVertexArray(quadVAO);

				// Quad vertices (per-vertex)
				glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

				// Quad indices
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

				// Vertex attributes (position + texcoords)
				// Location 0: aPos (vec3)
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
				// Location 1: aTexCoords (vec2)
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

				// Set up instance buffer for per-instance data
				// Location 2: aInstancePosition (vec3)
				// Location 3: aInstanceColor (vec4)
				// Location 4: aInstanceSize (float)
				// Location 5: aInstanceRotation (float radians)
				glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
				// Allocate buffer (will be updated each frame)
				glBufferData(GL_ARRAY_BUFFER, instanceCapacity * sizeof(float) * 9, nullptr, GL_STREAM_DRAW); // 9 floats per instance: pos(3) + color(4) + size(1) + rot(1)

				// Instance position (location 2)
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
				glVertexAttribDivisor(2, 1); // Advance once per instance

				// Instance color (location 3)
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
				glVertexAttribDivisor(3, 1); // Advance once per instance

				// Instance size (location 4)
				glEnableVertexAttribArray(4);
				glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
				glVertexAttribDivisor(4, 1); // Advance once per instance

				// Instance rotation (location 5)
				glEnableVertexAttribArray(5);
				glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
				glVertexAttribDivisor(5, 1); // Advance once per instance

				glBindVertexArray(0);
				logParticleGLError("init vao/vbo/ibo");
			}

			// Set up GL state for particles
			const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
			const GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
			GLboolean previousDepthMask = GL_TRUE;
			glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);

			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE); // Particles don't write to depth buffer
			glDisable(GL_CULL_FACE); // Billboard particles face camera
			logParticleGLError("set blend/depth/cull state");

			// Use particle shader
			glUseProgram(particleProgram);
			logParticleGLError("use program");
			glUniform1i(glGetUniformLocation(particleProgram, "tex"), 0);
			logParticleGLError("set tex uniform");

			// Set view and projection matrices
			glUniformMatrix4fv(glGetUniformLocation(particleProgram, "u_V"), 1, GL_FALSE, &activeCam->view()[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(particleProgram, "u_P"), 1, GL_FALSE, &activeCam->projection()[0][0]);
			logParticleGLError("set VP uniforms");

			glBindVertexArray(quadVAO);
			logParticleGLError("bind particle vao");

			std::vector<float> instanceData;
			instanceData.reserve(2048);
			auto assetManager = services.lock()->get<Assets::Manager>();

			for (auto [entity, ps, transform] : particleView.each()) {
				auto* psInstance = particleSystem->GetParticleSystem(entity);
				if (!psInstance)
					continue;

				auto& pool = psInstance->GetPool();
				if (pool.GetAliveCount() == 0)
					continue;

				instanceData.clear();
				struct ParticleInstance {
					glm::vec3 position;
					glm::vec4 color;
					float size;
					float rotation;
					float distanceSq;
				};
				std::vector<ParticleInstance> instances;
				instances.reserve(pool.GetAliveCount());
				const auto* particles = pool.GetParticles();
				const auto& aliveIndices = pool.GetAliveIndices();
				for (int idx : aliveIndices) {
					const auto& p = particles[idx];
					if (!p.alive)
						continue;
					ParticleInstance inst{};
					inst.position = p.position;
					inst.color = p.color;
					inst.size = p.size;
					inst.rotation = p.rotation;
					const glm::vec3 delta = p.position - activeCam->pos;
					inst.distanceSq = glm::dot(delta, delta);
					instances.push_back(inst);
				}

				if (ps.sortMode == ParticleSortMode::BackToFront) {
					std::sort(instances.begin(), instances.end(), [](const ParticleInstance& a, const ParticleInstance& b) {
						return a.distanceSq > b.distanceSq;
					});
				}

				for (const auto& inst : instances) {
					instanceData.push_back(inst.position.x);
					instanceData.push_back(inst.position.y);
					instanceData.push_back(inst.position.z);
					instanceData.push_back(inst.color.r);
					instanceData.push_back(inst.color.g);
					instanceData.push_back(inst.color.b);
					instanceData.push_back(inst.color.a);
					instanceData.push_back(inst.size);
					instanceData.push_back(inst.rotation);
				}

				const size_t instanceCount = instanceData.size() / 9;
				if (instanceCount == 0)
					continue;

				if (instanceCount > instanceCapacity) {
					instanceCapacity = instanceCount;
					glBindVertexArray(quadVAO);
					glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
					glBufferData(GL_ARRAY_BUFFER, instanceCapacity * sizeof(float) * 9, nullptr, GL_STREAM_DRAW);

					glEnableVertexAttribArray(2);
					glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
					glVertexAttribDivisor(2, 1);

					glEnableVertexAttribArray(3);
					glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
					glVertexAttribDivisor(3, 1);

					glEnableVertexAttribArray(4);
					glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
					glVertexAttribDivisor(4, 1);

					glEnableVertexAttribArray(5);
					glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
					glVertexAttribDivisor(5, 1);
					logParticleGLError("realloc instance buffer + rebind attribs");
				}

				GLuint particleTextureId = 0;
				if (assetManager && ps.particleTexture.IsValid()) {
					auto textureOpt = assetManager->getAsset<Assets::Texture>(ps.particleTexture);
					if (textureOpt.has_value() && textureOpt.value() && textureOpt.value()->gl_texture != 0) {
						particleTextureId = textureOpt.value()->gl_texture;
					}
				}

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, particleTextureId);

				switch (ps.blendMode) {
				case ParticleBlendMode::Additive:
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					break;
				case ParticleBlendMode::Premultiplied:
					glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case ParticleBlendMode::Alpha:
				default:
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				}

				glUniform1i(glGetUniformLocation(particleProgram, "u_UseTexture"), particleTextureId != 0 ? 1 : 0);
				glUniform1i(glGetUniformLocation(particleProgram, "u_Shape"), static_cast<int>(ps.renderShape));
				glUniform1f(glGetUniformLocation(particleProgram, "u_SoftEdge"), glm::clamp(ps.softEdge, 0.0f, 1.0f));
				logParticleGLError("set particle uniforms");

				glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, instanceData.size() * sizeof(float), instanceData.data());
				logParticleGLError("upload instance data");
				glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, static_cast<GLsizei>(instanceCount));
				logParticleGLError("draw instanced particles");
			}

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
			glUseProgram(0);
			logParticleGLError("cleanup bindings");

			// Restore GL state
			glDepthMask(previousDepthMask);
			if (wasBlendEnabled)
				glEnable(GL_BLEND);
			else
				glDisable(GL_BLEND);
			if (wasCullEnabled)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);
			logParticleGLError("restore state");
		}

		void System::debugPass(entt::registry& registry, int debug_mode) {

			auto rendererService = services.lock()->get<sRenderer>();
			if (!rendererService || !rendererService->w_renderer) return;


			auto svc = services.lock();
			if (!svc)
				return;

			auto scene = svc->get<Scene::SceneManager>();
			if (!scene)
				return;

			Camera* active_cam = scene->GetActiveCamera();
			if (!active_cam) return;

			// --- GAME CAMERA FRUSTUM RENDER ---
			Camera* game_cam = scene->GetGameCamera();

			if (game_cam && active_cam != game_cam && game_cam->showCollisionGizmo) {
				// 1. Calculate the Inverse View-Projection Matrix
				glm::mat4 view = game_cam->view();
				glm::mat4 proj = game_cam->projection();
				glm::mat4 invVP = glm::inverse(proj * view);

				// 2. Define the 8 corners of the frustum in NDC
				// Order matches your DebugPassOBB back/front face layout
				glm::vec4 ndcCorners[8] = {
					{-1.0f, -1.0f, -1.0f, 1.0f}, // 0: Near Bottom-Left
					{ 1.0f, -1.0f, -1.0f, 1.0f}, // 1: Near Bottom-Right
					{ 1.0f,  1.0f, -1.0f, 1.0f}, // 2: Near Top-Right
					{-1.0f,  1.0f, -1.0f, 1.0f}, // 3: Near Top-Left

					{-1.0f, -1.0f,  1.0f, 1.0f}, // 4: Far Bottom-Left
					{ 1.0f, -1.0f,  1.0f, 1.0f}, // 5: Far Bottom-Right
					{ 1.0f,  1.0f,  1.0f, 1.0f}, // 6: Far Top-Right
					{-1.0f,  1.0f,  1.0f, 1.0f}  // 7: Far Top-Left
				};

				// 3. Transform NDC corners into World Space
				glm::vec3 worldCorners[8];
				for (int i = 0; i < 8; ++i) {
					glm::vec4 worldPos = invVP * ndcCorners[i];
					worldCorners[i] = glm::vec3(worldPos) / worldPos.w; // Perspective divide
				}

				// 4. Draw Frustum (Yellow)
				glm::vec4 frustumColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				rendererService->w_renderer->DebugPassOBB(worldCorners, frustumColor, scene);
			}

			// Mode 1: Draw Physics Colliders (Cyan for basic, different colors for
			// compound shapes)
			if (debug_mode == 1) {
				// Iterate entities with both RigidBody3D and WorldTransform
				auto view = registry.view<Physics::RigidBody3D, WorldTransform>();

				for (auto [entity, rb, world_trans] : view.each()) {
					// Extract world pos, scale, rot
					glm::vec3 worldPos, worldScale, skew;
					glm::vec4 perspective;
					glm::quat worldRot;
					glm::decompose(world_trans.matrix, worldScale, worldRot, worldPos, skew, perspective);
					worldRot = glm::normalize(worldRot);

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
									shape.boxHalfExtents * worldScale * rb.collider_scale;
							} else if (shape.type == ColliderShapeType::Sphere) {
								float scaledRadius =
									shape.sphereRadius *
									glm::max(worldScale.x, glm::max(worldScale.y, worldScale.z));
								half_extents = glm::vec3(scaledRadius);
							} else if (shape.type == ColliderShapeType::Capsule) {
								float scaledRadius =
									shape.capsuleRadius * glm::max(worldScale.x, worldScale.z);
								float scaledHeight = shape.capsuleHalfHeight * worldScale.y;
								half_extents = glm::vec3(scaledRadius, scaledHeight + scaledRadius,
														 scaledRadius);
							} else {
								half_extents = glm::vec3(0.5f) * worldScale;
							}

							// Calculate scaled offset (note: offset does NOT scale by
							// collider_scale per physics engine)
							glm::vec3 scaled_offset = shape.offset * worldScale;

							// Entity rotation combined with shape local rotation
							glm::quat entity_rot = worldRot;
							glm::quat shape_rot = glm::normalize(shape.rotation);
							glm::quat combined_rot = entity_rot * shape_rot;

							// Check if entity has any rotation (non-identity quaternion)
							bool hasRotation = glm::abs(worldRot.w - 1.0f) > 0.0001f ||
											   glm::abs(worldRot.x) > 0.0001f ||
											   glm::abs(worldRot.y) > 0.0001f ||
											   glm::abs(worldRot.z) > 0.0001f;

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
									corners[i] = worldPos + rotated_offset + rotated_corner;
								}

								// Use OBB rendering for rotated entities
								rendererService->w_renderer->DebugPassOBB(corners, color, scene);
							} else {
								// No rotation - use faster AABB path
								glm::vec3 center = worldPos + scaled_offset;
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

						glm::vec3 offset_scaled = worldScale * rb.collider_offset;
						glm::vec3 half_extents = 0.5f * worldScale * rb.collider_scale;
						glm::quat rotation = worldRot;

						// Check if entity has any rotation (non-identity quaternion)
						bool hasRotation = glm::abs(worldRot.w - 1.0f) > 0.0001f ||
										   glm::abs(worldRot.x) > 0.0001f ||
										   glm::abs(worldRot.y) > 0.0001f ||
										   glm::abs(worldRot.z) > 0.0001f;

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
								corners[i] = worldPos + (rotation * body_space_point);
							}

							// Use OBB rendering for rotated entities
							rendererService->w_renderer->DebugPassOBB(corners, color, scene);
						} else {
							// No rotation - use faster AABB path
							glm::vec3 center = worldPos + offset_scaled;
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

			auto metadata_service = services.lock()->get<MetaData::Service>();
			if (!metadata_service) return;


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

			//Graphics settings
			auto& gs = GraphicsSettings::get();

			// ========================================
			// MINIMAP
			// ========================================
			if (gs.minimap_enabled) {
				GLuint minimapTexture = rendererService->w_renderer->getMinimapTexture();
				if (minimapTexture != 0) {
					auto assetManager = services.lock()->get<Assets::Manager>();
					auto window = services.lock()->get<Window::Window>();
					if (window) {
						glm::vec2 framebuffer = window->getFrameBuffer();
						float fbw = glm::max(1.0f, framebuffer.x);
						float fbh = glm::max(1.0f, framebuffer.y);

						const float border_thickness = glm::max(0.0f, gs.minimap_border_thickness);

						const float map_w = glm::clamp(gs.minimap_size_px.x, 32.0f, fbw);
						const float map_h = glm::clamp(gs.minimap_size_px.y, 32.0f, fbh);
						const float outer_w = map_w;
						const float outer_h = map_h;

						float map_x = gs.minimap_pos_px.x;
						float map_y = gs.minimap_pos_px.y;

						if (!gs.minimap_override_position) {
							switch (gs.minimap_recommended_position) {
							case GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::TOP_LEFT:
								map_x = 20.0f;
								map_y = 20.0f;
								break;
							case GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::TOP_RIGHT:
								map_x = fbw - outer_w - 20.0f;
								map_y = 20.0f;
								break;
							case GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::BOTTOM_LEFT:
								map_x = 20.0f;
								map_y = fbh - outer_h - 20.0f;
								break;
							case GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::BOTTOM_RIGHT:
								map_x = fbw - outer_w - 20.0f;
								map_y = fbh - outer_h - 20.0f;
								break;
							case GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::TOP_MIDDLE:
								map_x = (fbw - outer_w) * 0.5f;
								map_y = 20.0f;
								break;
							case GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::BOTTOM_MIDDLE:
							default:
								map_x = (fbw - outer_w) * 0.5f;
								map_y = fbh - outer_h - 20.0f;
								break;
							}
						}

						map_x = glm::clamp(map_x, 0.0f, glm::max(0.0f, fbw - outer_w));
						map_y = glm::clamp(map_y, 0.0f, glm::max(0.0f, fbh - outer_h));

						const float content_x = map_x;
						const float content_y = map_y;
						const float draw_x = content_x;
						const float draw_y = content_y;
						const float draw_w = map_w;
						const float draw_h = map_h;
						const float draw_min = glm::max(1.0f, glm::min(draw_w, draw_h));
						const float draw_w_safe = glm::max(1.0f, draw_w);
						const float draw_h_safe = glm::max(1.0f, draw_h);

						const float center_x_px = draw_x + draw_w * 0.5f;
						const float center_y_px = draw_y + draw_h * 0.5f;

						glm::vec2 minimap_pos(
							(center_x_px / fbw) * 2.0f - 1.0f,
							1.0f - (center_y_px / fbh) * 2.0f);

						glm::vec2 minimap_scale(
							(map_w / fbh),
							(map_h / fbh));

						rendererService->w_renderer->Render2DTexture(minimapTexture, minimap_pos,
							minimap_scale);

						auto toNdc = [&](float px, float py) {
							return glm::vec2(
								(px / fbw) * 2.0f - 1.0f,
								1.0f - (py / fbh) * 2.0f);
							};

						const glm::vec4 border_color(
							gs.minimap_border_color.r,
							gs.minimap_border_color.g,
							gs.minimap_border_color.b,
							glm::max(0.2f, gs.minimap_border_color.a));

						const int border_layers = static_cast<int>(glm::round(border_thickness));
						if (border_layers > 0) {
							for (int i = 0; i < border_layers; ++i) {
								const float inset = static_cast<float>(i) + 0.5f;
								const float x0 = map_x + inset;
								const float y0 = map_y + inset;
								const float x1 = map_x + outer_w - inset;
								const float y1 = map_y + outer_h - inset;

								if (x1 <= x0 || y1 <= y0) {
									break;
								}

								const glm::vec2 tl = toNdc(x0, y0);
								const glm::vec2 tr = toNdc(x1, y0);
								const glm::vec2 br = toNdc(x1, y1);
								const glm::vec2 bl = toNdc(x0, y1);

								rendererService->w_renderer->DebugPass2DLine(tl, tr, border_color);
								rendererService->w_renderer->DebugPass2DLine(tr, br, border_color);
								rendererService->w_renderer->DebugPass2DLine(br, bl, border_color);
								rendererService->w_renderer->DebugPass2DLine(bl, tl, border_color);
							}
						}

						glm::vec3 player_pos(0.0f);
						glm::vec3 player_forward = glm::vec3(0.0f, 0.0f, -1.0f);
						glm::vec3 view_forward = glm::vec3(0.0f, 0.0f, -1.0f);
						bool has_player = false;
						bool has_view_forward = false;

						{
							const auto playersByTag = metadata_service->getEntitiesByTag("Player");
							for (const entt::entity player : playersByTag) {
								if (!registry.valid(player)) {
									continue;
								}
								if (auto* wt = registry.try_get<WorldTransform>(player)) {
									player_pos = glm::vec3(wt->matrix[3]);
									has_player = true;
								}
								if (auto* lt = registry.try_get<LocalTransform>(player)) {
									player_pos = lt->position;
									player_forward = lt->rotation * glm::vec3(0.0f, 0.0f, -1.0f);
									has_player = true;
								}
								if (has_player) {
									break;
								}
							}
						}

						if (!has_player) {
							if (auto playerEntityOpt = metadata_service->getEntityByName("Player")) {
								const entt::entity player = playerEntityOpt.value();
								if (registry.valid(player)) {
									if (auto* wt = registry.try_get<WorldTransform>(player)) {
										player_pos = glm::vec3(wt->matrix[3]);
										has_player = true;
									}
									if (auto* lt = registry.try_get<LocalTransform>(player)) {
										player_pos = lt->position;
										player_forward = lt->rotation * glm::vec3(0.0f, 0.0f, -1.0f);
										has_player = true;
									}
								}
							}
						}

						if (!has_player) {
							if (auto activeCam = scn_service->GetActiveCamera()) {
								player_pos = activeCam->pos;
								view_forward = activeCam->forward;
								has_player = true;
								has_view_forward = true;
							}
						}

						if (!has_view_forward) {
							if (auto activeCam = scn_service->GetActiveCamera()) {
								view_forward = activeCam->forward;
								has_view_forward = true;
							}
						}

						if (!has_player) {
							player_pos = glm::vec3(0.0f);
						}

						if (!has_view_forward) {
							view_forward = player_forward;
						}

						view_forward.y = 0.0f;
						if (glm::dot(view_forward, view_forward) < 0.0001f) {
							view_forward = glm::vec3(0.0f, 0.0f, -1.0f);
						}
						else {
							view_forward = glm::normalize(view_forward);
						}

						// Use camera POV basis and invert once to match on-screen intuition.
						glm::vec2 forward2(-view_forward.x, -view_forward.z);
						if (glm::dot(forward2, forward2) < 0.0001f) {
							forward2 = glm::vec2(0.0f, -1.0f);
						}
						else {
							forward2 = glm::normalize(forward2);
						}
						const glm::vec2 right2(-forward2.y, forward2.x);

						auto getEntityWorldPos = [&](entt::entity entity, glm::vec3& outPos) {
							if (!registry.valid(entity)) {
								return false;
							}
							if (auto* wt = registry.try_get<WorldTransform>(entity)) {
								outPos = glm::vec3(wt->matrix[3]);
								return true;
							}
							if (auto* lt = registry.try_get<LocalTransform>(entity)) {
								outPos = lt->position;
								return true;
							}
							return false;
							};

						auto worldToMinimapNdc = [&](const glm::vec3& worldPos, bool clampToEdge) {
							const glm::vec2 delta(worldPos.x - player_pos.x, worldPos.z - player_pos.z);
							float local_x = delta.x;
							float local_y = -delta.y;
							if (gs.minimap_rotate_with_player) {
								local_x = glm::dot(delta, right2);
								local_y = glm::dot(delta, forward2);
								// Camera-space orientation correction (both axes)
								local_x = -local_x;
								local_y = -local_y;
							}

							const float radius = glm::max(1.0f, gs.minimap_radius);
							const float inv_double_radius_x = (draw_min / draw_w_safe) / (2.0f * radius);
							const float inv_double_radius_y = (draw_min / draw_h_safe) / (2.0f * radius);
							const float u = 0.5f + local_x * inv_double_radius_x;
							const float v = 0.5f + local_y * inv_double_radius_y;
							const float u_draw = clampToEdge ? glm::clamp(u, 0.0f, 1.0f) : u;
							const float v_draw = clampToEdge ? glm::clamp(v, 0.0f, 1.0f) : v;

							const float px = draw_x + u_draw * draw_w;
							const float py = draw_y + (1.0f - v_draw) * draw_h;

							return glm::vec2(
								(px / fbw) * 2.0f - 1.0f,
								1.0f - (py / fbh) * 2.0f);
							};

						auto drawDot = [&](const glm::vec3& pos, float radiusPx, const glm::vec4& color) {
							const glm::vec2 centerNdc = worldToMinimapNdc(pos, true);
							const glm::vec2 radiusNdc(
								(radiusPx / fbw) * 2.0f,
								(radiusPx / fbh) * 2.0f);
							rendererService->w_renderer->DebugPass2DCircle(centerNdc, radiusNdc, color, 18);
							};

						GLuint playerIconTex = 0;
						GLuint itemIconTex = 0;
						GLuint objectiveIconTex = 0;
						if (gs.minimap_use_icon_textures && assetManager) {
							auto resolveIcon = [&](const std::string& path) -> GLuint {
								if (path.empty()) {
									return 0;
								}
								auto iconOpt = assetManager->getAsset<Assets::Texture>(path);
								if (!iconOpt.has_value() || !iconOpt.value()) {
									return 0;
								}
								return iconOpt.value()->gl_texture;
								};

							playerIconTex = resolveIcon(gs.minimap_icon_player_path);
							itemIconTex = resolveIcon(gs.minimap_icon_item_path);
							objectiveIconTex = resolveIcon(gs.minimap_icon_objective_path);
						}

						auto drawMarker = [&](const glm::vec3& pos,
							float radiusPx,
							const glm::vec4& color,
							GLuint iconTex) {
								if (gs.minimap_use_icon_textures && iconTex != 0) {
									const glm::vec2 iconNdcPos = worldToMinimapNdc(pos, true);
									const float iconPx = glm::max(4.0f, radiusPx * 2.0f * gs.minimap_icon_scale);
									glm::vec2 iconNdcScale(iconPx / fbh, iconPx / fbh);
									rendererService->w_renderer->Render2DTexture(
										iconTex,
										iconNdcPos,
										iconNdcScale);
									return;
								}

								drawDot(pos, radiusPx, color);
							};

						auto drawWallFootprint = [&](entt::entity entity, const glm::vec4& color) -> bool {
							if (auto* bv = registry.try_get<BoundingVolume>(entity)) {
								if (bv->worldAABB.isValid()) {
									const glm::vec3 minP = bv->worldAABB.min;
									const glm::vec3 maxP = bv->worldAABB.max;

									const glm::vec3 c0(minP.x, 0.0f, minP.z);
									const glm::vec3 c1(maxP.x, 0.0f, minP.z);
									const glm::vec3 c2(maxP.x, 0.0f, maxP.z);
									const glm::vec3 c3(minP.x, 0.0f, maxP.z);

									const glm::vec2 p0 = worldToMinimapNdc(c0, true);
									const glm::vec2 p1 = worldToMinimapNdc(c1, true);
									const glm::vec2 p2 = worldToMinimapNdc(c2, true);
									const glm::vec2 p3 = worldToMinimapNdc(c3, true);

									rendererService->w_renderer->DebugPass2DLine(p0, p1, color);
									rendererService->w_renderer->DebugPass2DLine(p1, p2, color);
									rendererService->w_renderer->DebugPass2DLine(p2, p3, color);
									rendererService->w_renderer->DebugPass2DLine(p3, p0, color);
									return true;
								}
							}

							return false;
							};

						struct WallMinimapCache {
							const entt::registry* sourceRegistry = nullptr;
							size_t wallEntityCount = 0;
							uint64_t wallEntitySignature = 0;
							bool built = false;
							bool gpuDirty = false; // true when triangleVerticesXZ changed and needs GPU re-upload
							std::vector<glm::vec2> triangleVerticesXZ;
						};

						static WallMinimapCache wallCache;

						auto rebuildWallCache = [&](const std::vector<entt::entity>& wallEntities) {
							wallCache.triangleVerticesXZ.clear();

							for (const entt::entity entity : wallEntities) {
								if (!registry.valid(entity)) {
									continue;
								}

								auto* model = registry.try_get<ModelRenderer>(entity);
								auto* wt = registry.try_get<WorldTransform>(entity);
								if (!model || !wt) {
									continue;

								}

								if (model->modelGUID != model->prevModelGUID) {
									InitializeModelRenderer(entity, *model);
								}

								if (!model->cachedModelAsset) {
									continue;
								}

								const auto& vertices = model->cachedModelAsset->vertices;
								const auto& indices = model->cachedModelAsset->indices;
								if (vertices.empty() || indices.size() < 3) {
									continue;
								}

								const glm::mat4 M = wt->matrix;
								for (size_t i = 0; i + 2 < indices.size(); i += 3) {
									const uint32_t i0 = indices[i];
									const uint32_t i1 = indices[i + 1];
									const uint32_t i2 = indices[i + 2];
									if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
										continue;
									}

									const glm::vec3 w0 = glm::vec3(M * glm::vec4(vertices[i0].pos, 1.0f));
									const glm::vec3 w1 = glm::vec3(M * glm::vec4(vertices[i1].pos, 1.0f));
									const glm::vec3 w2 = glm::vec3(M * glm::vec4(vertices[i2].pos, 1.0f));

									wallCache.triangleVerticesXZ.emplace_back(w0.x, w0.z);
									wallCache.triangleVerticesXZ.emplace_back(w1.x, w1.z);
									wallCache.triangleVerticesXZ.emplace_back(w2.x, w2.z);
								}
							}

							wallCache.sourceRegistry = &registry;
							wallCache.wallEntityCount = wallEntities.size();
                            uint64_t signature = 1469598103934665603ull;
							for (const entt::entity entity : wallEntities) {
								signature ^= static_cast<uint64_t>(entity);
								signature *= 1099511628211ull;
							}
							wallCache.wallEntitySignature = signature;
							wallCache.built = true;
							wallCache.gpuDirty = true;
							};

						glm::vec3 nearest_item_pos(0.0f);
						glm::vec3 nearest_objective_pos(0.0f);
						bool has_nearest_item = false;
						bool has_nearest_objective = false;
						float nearest_item_dist2 = std::numeric_limits<float>::max();
						float nearest_objective_dist2 = std::numeric_limits<float>::max();

						std::unordered_set<uint32_t> uniqueMarkerEntities;
						std::vector<entt::entity> markerEntities;
						const std::vector<entt::entity> wallEntities = metadata_service->getEntitiesByTag("wall");
						auto appendTaggedEntities = [&](const char* tag) {
							for (entt::entity entity : metadata_service->getEntitiesByTag(tag)) {
								const uint32_t key = static_cast<uint32_t>(entity);
								if (uniqueMarkerEntities.insert(key).second) {
									markerEntities.push_back(entity);
								}
							}
							};

						appendTaggedEntities("Player");
						appendTaggedEntities("Enemy");
						appendTaggedEntities("item");
						appendTaggedEntities("objective");
						appendTaggedEntities("letter_collectible");
						appendTaggedEntities("letter_carried");
						appendTaggedEntities("letter_collection");

						const bool has_carried_letter = !metadata_service->getEntitiesByTag("letter_carried").empty();

						// Clip all minimap overlays (walls, markers, danger, route, legend, arrows)
						glEnable(GL_SCISSOR_TEST);
						glScissor(
							static_cast<GLint>(draw_x),
							static_cast<GLint>(fbh - draw_y - draw_h),
							static_cast<GLsizei>(draw_w),
							static_cast<GLsizei>(draw_h));

						if (gs.minimap_show_walls) {
                            uint64_t currentWallSignature = 1469598103934665603ull;
							for (const entt::entity entity : wallEntities) {
								currentWallSignature ^= static_cast<uint64_t>(entity);
								currentWallSignature *= 1099511628211ull;
							}

							const bool needsRebuild =
								!wallCache.built ||
								wallCache.sourceRegistry != &registry ||
								wallCache.wallEntityCount != wallEntities.size() ||
								wallCache.wallEntitySignature != currentWallSignature;

							if (needsRebuild) {
								rebuildWallCache(wallEntities);
							}

							// Upload wall vertices to GPU when cache was rebuilt
							if (wallCache.gpuDirty) {
								rendererService->w_renderer->UploadMinimapWalls(wallCache.triangleVerticesXZ);
								wallCache.gpuDirty = false;
							}

							// GPU draw path: single draw call with scissor clipping
							if (rendererService->w_renderer->hasMinimapWallData()) {
								const float minimapRadius = glm::max(1.0f, gs.minimap_radius);
								const glm::vec2 playerXZ(player_pos.x, player_pos.z);

								// Build 2x2 orientation matrix for vertex shader
								glm::vec2 transformCol0, transformCol1;
								if (gs.minimap_rotate_with_player) {
									transformCol0 = glm::vec2(-right2.x, -forward2.x);
									transformCol1 = glm::vec2(-right2.y, -forward2.y);
								}
								else {
									transformCol0 = glm::vec2(1.0f, 0.0f);
									transformCol1 = glm::vec2(0.0f, -1.0f);
								}

								const glm::vec2 invDoubleRadius(
									(draw_min / draw_w_safe) / (2.0f * minimapRadius),
									(draw_min / draw_h_safe) / (2.0f * minimapRadius));
								const glm::vec2 ndcBase(
									(draw_x / fbw) * 2.0f - 1.0f,
									1.0f - ((draw_y + draw_h) / fbh) * 2.0f);
								const glm::vec2 ndcScale(
									(draw_w / fbw) * 2.0f,
									(draw_h / fbh) * 2.0f);

								rendererService->w_renderer->DrawMinimapWalls(
									playerXZ, transformCol0, transformCol1,
									invDoubleRadius, ndcBase, ndcScale,
									glm::vec4(0.75f, 0.75f, 0.75f, 0.35f));
							}
							else {
								// Fallback to AABB outline when no triangle data available
								for (const entt::entity wallEntity : wallEntities) {
									drawWallFootprint(wallEntity, glm::vec4(0.75f, 0.75f, 0.75f, 1.0f));
								}
							}
						}

						for (entt::entity entity : markerEntities) {
							glm::vec3 pos(0.0f);
							if (!getEntityWorldPos(entity, pos)) {
								continue;
							}

							glm::vec4 color(0.75f, 0.75f, 0.75f, 1.0f);
							float dotRadius = 3.0f;

							if (metadata_service->hasTag(entity, "Player")) {
								if (!gs.minimap_show_player) {
									continue;
								}
								color = glm::vec4(0.2f, 1.0f, 0.2f, 1.0f);
								dotRadius = 5.0f;
								drawMarker(pos, dotRadius, color, playerIconTex);
								continue;
							}
							else if (metadata_service->hasTag(entity, "item") ||
								metadata_service->hasTag(entity, "letter_collectible") ||
								metadata_service->hasTag(entity, "letter_carried")) {
								if (!gs.minimap_show_items) {
									continue;
								}
								color = glm::vec4(1.0f, 0.84f, 0.1f, 1.0f);
								dotRadius = 4.0f;
								const glm::vec3 delta = pos - player_pos;
								const float dist2 = glm::dot(delta, delta);
								if (dist2 < nearest_item_dist2) {
									nearest_item_dist2 = dist2;
									nearest_item_pos = pos;
									has_nearest_item = true;
								}
								drawMarker(pos, dotRadius, color, itemIconTex);
								continue;
							}
							else if (metadata_service->hasTag(entity, "objective") ||
								metadata_service->hasTag(entity, "letter_collection")) {
								if (!gs.minimap_show_objective) {
									continue;
								}
								color = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
								dotRadius = 4.0f;

								const glm::vec3 delta = pos - player_pos;
								const float dist2 = glm::dot(delta, delta);
								if (dist2 < nearest_objective_dist2) {
									nearest_objective_dist2 = dist2;
									nearest_objective_pos = pos;
									has_nearest_objective = true;
								}
								drawMarker(pos, dotRadius, color, objectiveIconTex);
								continue;
							}
							else {
								continue;
							}
						}

						if (gs.minimap_show_danger) {
							std::unordered_set<uint32_t> uniqueDangerEntities;
							std::vector<entt::entity> dangerEntities;
							auto appendDangerEntities = [&](const char* tag) {
								for (entt::entity entity : metadata_service->getEntitiesByTag(tag)) {
									const uint32_t key = static_cast<uint32_t>(entity);
									if (uniqueDangerEntities.insert(key).second) {
										dangerEntities.push_back(entity);
									}
								}
								};
							appendDangerEntities("danger");
							appendDangerEntities("Enemy");

							std::vector<glm::vec2> dangerLineVertices;
							dangerLineVertices.reserve(dangerEntities.size() * 24 * 2);
							for (entt::entity entity : dangerEntities) {
								glm::vec3 pos(0.0f);
								if (!getEntityWorldPos(entity, pos)) {
									continue;
								}

								float danger_radius = 1.0f;
								if (metadata_service->hasTag(entity, "light_cone")) {
									const float maxDistance = 9.0f;
									const float coneAngleDeg = 32.0f;
									const float heightDelta = glm::abs(pos.y - player_pos.y);
									const float coneRadius = glm::tan(glm::radians(coneAngleDeg)) * heightDelta;
									const float sphereRadius =
										heightDelta >= maxDistance
										? 0.0f
										: std::sqrt(maxDistance * maxDistance - heightDelta * heightDelta);
									danger_radius = glm::max(0.25f, glm::min(coneRadius, sphereRadius));
								}
								else if (metadata_service->hasTag(entity, "danger_collision")) {
									danger_radius = 0.5f;
								}
								else if (auto* lt = registry.try_get<LocalTransform>(entity)) {
									danger_radius = glm::max(0.25f, glm::max(lt->scale.x, lt->scale.z));
								}

								const glm::vec3 delta3 = pos - player_pos;
								const float dist_xz = glm::length(glm::vec2(delta3.x, delta3.z));
								if (dist_xz > glm::max(1.0f, gs.minimap_radius) + danger_radius) {
									continue;
								}

								const glm::vec2 centerNdc = worldToMinimapNdc(pos, false);
								const float radiusPx = (danger_radius / (2.0f * glm::max(1.0f, gs.minimap_radius))) * draw_min;
								const glm::vec2 radiusNdc((radiusPx / fbw) * 2.0f, (radiusPx / fbh) * 2.0f);

								for (int i = 0; i < 24; ++i) {
									const float a0 = (static_cast<float>(i) / 24.0f) * glm::two_pi<float>();
									const float a1 = (static_cast<float>(i + 1) / 24.0f) * glm::two_pi<float>();

									const glm::vec2 p0(
										centerNdc.x + std::cos(a0) * radiusNdc.x,
										centerNdc.y + std::sin(a0) * radiusNdc.y);
									const glm::vec2 p1(
										centerNdc.x + std::cos(a1) * radiusNdc.x,
										centerNdc.y + std::sin(a1) * radiusNdc.y);

									dangerLineVertices.push_back(p0);
									dangerLineVertices.push_back(p1);
								}
							}

							if (!dangerLineVertices.empty()) {
								rendererService->w_renderer->DebugPass2DLines(
									dangerLineVertices,
									glm::vec4(1.0f, 0.15f, 0.15f, 1.0f));
							}
						}

						glm::vec3 route_target_world(0.0f);
						bool has_route_target = false;
						if (has_carried_letter) {
							if (has_nearest_objective) {
								route_target_world = nearest_objective_pos;
								has_route_target = true;
							}
							else if (has_nearest_item) {
								route_target_world = nearest_item_pos;
								has_route_target = true;
							}
						}
						else {
							if (has_nearest_item) {
								route_target_world = nearest_item_pos;
								has_route_target = true;
							}
							else if (has_nearest_objective) {
								route_target_world = nearest_objective_pos;
								has_route_target = true;
							}
						}

						if (has_route_target && gs.minimap_show_route) {
							const glm::vec2 p0 = worldToMinimapNdc(player_pos, true);
							const glm::vec2 p1_clamped = worldToMinimapNdc(route_target_world, true);
							const glm::vec2 p1_unclamped = worldToMinimapNdc(route_target_world, false);
							const bool off_map = glm::length(p1_unclamped - p1_clamped) > 0.0001f;

							switch (gs.minimap_route_mode) {
							case GraphicsSettings::MINIMAP_ROUTE_MODE::ROUTE_NEAREST_LINE:
								rendererService->w_renderer->DebugPass2DLine(
									p0,
									p1_clamped,
									glm::vec4(1.0f, 0.9f, 0.2f, 1.0f));
								break;
							case GraphicsSettings::MINIMAP_ROUTE_MODE::ROUTE_BREADCRUMB_DOTS: {
								for (int i = 1; i <= 6; ++i) {
									const float t = static_cast<float>(i) / 7.0f;
									const glm::vec2 p = glm::mix(p0, p1_clamped, t);
									rendererService->w_renderer->DebugPass2DCircle(
										p,
										glm::vec2((2.0f / fbw) * 2.0f, (2.0f / fbh) * 2.0f),
										glm::vec4(1.0f, 0.9f, 0.2f, 1.0f),
										12);
								}
								break;
							}
							case GraphicsSettings::MINIMAP_ROUTE_MODE::ROUTE_EDGE_ARROW:
							case GraphicsSettings::MINIMAP_ROUTE_MODE::ROUTE_LINE_AND_EDGE_ARROW: {
								if (gs.minimap_route_mode == GraphicsSettings::MINIMAP_ROUTE_MODE::ROUTE_LINE_AND_EDGE_ARROW) {
									rendererService->w_renderer->DebugPass2DLine(
										p0,
										p1_clamped,
										glm::vec4(1.0f, 0.9f, 0.2f, 0.7f));
								}

								if (off_map) {
									glm::vec2 dir = p1_clamped - p0;
									if (glm::dot(dir, dir) < 0.0001f) {
										dir = glm::vec2(0.0f, -1.0f);
									}
									else {
										dir = glm::normalize(dir);
									}

									const glm::vec2 arrow_tip = p1_clamped;
									const glm::vec2 arrow_tail = arrow_tip - dir * 0.04f;
									rendererService->w_renderer->DebugPass2DLine(
										arrow_tail,
										arrow_tip,
										glm::vec4(1.0f, 0.9f, 0.2f, 1.0f));
								}
								break;
							}
							default:
								break;
							}
						}

						if (gs.minimap_show_legend) {
							const glm::vec2 legend_base = toNdc(map_x + 10.0f, map_y + 12.0f);
							const glm::vec2 legend_step(0.0f, -(12.0f / fbh) * 2.0f);
							const glm::vec2 legend_radius((2.0f / fbw) * 2.0f, (2.0f / fbh) * 2.0f);

							int row = 0;
							auto drawLegendDot = [&](const glm::vec4& c) {
								rendererService->w_renderer->DebugPass2DCircle(
									legend_base + static_cast<float>(row) * legend_step,
									legend_radius,
									c,
									12);
								row++;
								};

							if (gs.minimap_show_player) drawLegendDot(glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));
							if (gs.minimap_show_danger) drawLegendDot(glm::vec4(1.0f, 0.15f, 0.15f, 1.0f));
							if (gs.minimap_show_items) drawLegendDot(glm::vec4(1.0f, 0.84f, 0.1f, 1.0f));
							if (gs.minimap_show_objective) drawLegendDot(glm::vec4(0.2f, 0.6f, 1.0f, 1.0f));
							if (gs.minimap_show_walls) drawLegendDot(glm::vec4(0.75f, 0.75f, 0.75f, 1.0f));
						}

						glm::vec2 arrow_dir(0.0f, -1.0f);
						if (!gs.minimap_rotate_with_player) {
							arrow_dir = -forward2;
							if (glm::dot(arrow_dir, arrow_dir) < 0.0001f) {
								arrow_dir = glm::vec2(0.0f, -1.0f);
							}
							else {
								arrow_dir = glm::normalize(arrow_dir);
							}
						}

						const float arrow_len_px = glm::max(8.0f, glm::min(map_w, map_h) * 0.15f);
						glm::vec2 arrow_start_px(center_x_px, center_y_px);
						glm::vec2 arrow_end_px = arrow_start_px + arrow_dir * arrow_len_px;

						glm::vec2 arrow_start_ndc(
							(arrow_start_px.x / fbw) * 2.0f - 1.0f,
							1.0f - (arrow_start_px.y / fbh) * 2.0f);
						glm::vec2 arrow_end_ndc(
							(arrow_end_px.x / fbw) * 2.0f - 1.0f,
							1.0f - (arrow_end_px.y / fbh) * 2.0f);

						if (gs.minimap_show_player) {
							rendererService->w_renderer->DebugPass2DLine(
								arrow_start_ndc,
								arrow_end_ndc,
								glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));
						}

						glDisable(GL_SCISSOR_TEST);
					}
				}
			}


			for (auto [entity, texture_comp, ui_elem, rect_comp] : texture_group.each()) {
				// Layer check
				auto layerComp = registry.try_get<Entity::Layer>(entity);
				if (layerComp && !scn_service->isLayerEnabled(layerComp->layer_id)) {
					continue;
				}

#ifdef PN_PLATFORM_WINDOWS
#ifndef _DEBUG
				// Skip android UI
				if (metadata_service->hasTag(entity, "android_ui")) { continue; }
#endif
#endif

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

			// Only sync position/scale from UIRectTransform for entities that follow world entities
			glm::vec2 render_pos = texture_comp.pos;
			glm::vec2 render_scale = texture_comp.texture_scale;
			
			if (registry.all_of<UIFollowsWorldEntity>(entity)) {
				if (rect_comp.layout_dirty) {
					// Write back so pos stays valid even when not dirty
					texture_comp.pos = rect_comp.calculated_world_position;
					texture_comp.texture_scale = glm::vec2(rect_comp.scale.x, rect_comp.scale.y);
					rect_comp.layout_dirty = false;
				}
				render_pos = texture_comp.pos;
				render_scale = texture_comp.texture_scale;
			}

			rendererService->w_renderer->Render2DTexture(
				texture_opt.value()->gl_texture, render_pos,
				render_scale, uv_transform);
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
				if (registry.all_of<UIFollowsWorldEntity>(entity)) {
					if (rect_comp.layout_dirty) {
						text_comp.text_pos = rect_comp.calculated_world_position;
						text_comp.scale_factor = rect_comp.scale.x;
						rect_comp.layout_dirty = false;
					}
					// text_comp.text_pos is now always valid even without dirty
				}
				else if (rect_comp.layout_dirty) {
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
