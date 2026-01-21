#include "pch.h"
#include "sysRender.h"

#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Controller.h"
#include "ECS/Components/AllComponents.h"
#include "Systems/Collision/sBVHSystem.h"
#include "ECS/Components/cAnimation.h" 
#include "ECS/Components/cPhysics.h" // For rendering RigidBody3D
#include "CoreSystems/Renderer/text.h"
#ifdef _DEBUG
#include "LayeredSystems/LevelEditor/Editor.h"
#endif

namespace PAIN {
    namespace Render {

        System::System(std::shared_ptr<Services> svc) 
            : ISystem(svc)
        {

            PN_CORE_INFO("[RenderSystem] Initialized");
        }

        void System::InitializeModelRenderer(entt::entity entity, ModelRenderer& component) {

            //Get asset manager
            auto assetManager = services.lock()->get<Assets::Manager>();

            //Check for valid GUID
            if (!component.modelGUID.IsValid()) return;

            //Set prev GUID
            component.prevModelGUID = component.modelGUID;

            //Initiate scene VBO update
            services.lock()->get<sRenderer>()->initSceneVbo();

            //Boolean for checking for material updates
            bool material_updates = false;
            if (component.cachedModelAsset || component.materials.empty()) material_updates = true;

            //optional material asset
            auto model_opt = assetManager->getAsset<Assets::Model>(component.modelGUID);

            // Load model asset
            component.cachedModelAsset = model_opt.has_value() ? model_opt.value() : nullptr;
            if (!component.cachedModelAsset) {
                PN_CORE_ERROR("Failed to load model asset for entity {}", (uint32_t)entity);
                return;
            }

            //Cache model
            const auto& modelAsset = component.cachedModelAsset;

            //Update material list only when needed
            if (material_updates) {

                //Update new materials
                component.materials.clear();
                component.materials.reserve(modelAsset->materials.size());
                for (const auto& materialPath : modelAsset->materials) {
                    MaterialInstance matInstance;

                    //optional material asset
                    auto materialAssetOpt = assetManager->getAsset<Assets::Material>(materialPath);

                    // Load material asset
                    auto materialAsset = materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;

                    //Check valid material asset
                    if (materialAsset) {
                        matInstance.materialGUID = materialAsset->guid;

                        //Init material overrides
                        matInstance.albedoTextureOverride = assetManager->findGUID(materialAsset->albedoTexturePath);
                        matInstance.normalTextureOverride = assetManager->findGUID(materialAsset->normalTexturePath);
                        matInstance.metallicTextureOverride = assetManager->findGUID(materialAsset->metallicTexturePath);
                        matInstance.roughnessTextureOverride = assetManager->findGUID(materialAsset->roughnessTexturePath);
                        matInstance.aoTextureOverride = assetManager->findGUID(materialAsset->aoTexturePath);
                        matInstance.emissiveTextureOverride = assetManager->findGUID(materialAsset->emissiveTexturePath);
                        matInstance.heightTextureOverride = assetManager->findGUID(materialAsset->heightTexturePath);
                        matInstance.opacityTextureOverride = assetManager->findGUID(materialAsset->opacityTexturePath);
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
            // Cache GL texture handles to avoid AssetManager lookups every frame in DrawGeometry
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
                auto materialAssetOpt = assetManager->getAsset<Assets::Material>(material->materialGUID);
                auto materialAsset = materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;

                if (materialAsset) {
                    // Cache base material properties
                    cache.baseColor = material->useOverrides ? material->baseColorOverride : materialAsset->baseColor;
                    cache.metallic = material->useOverrides ? material->metallicOverride : materialAsset->metallic;
                    cache.roughness = material->useOverrides ? material->roughnessOverride : materialAsset->roughness;

                    // Helper lambda to get GL texture handle from GUID
                    auto getTextureHandle = [&](const Assets::GUID& guid) -> GLuint {
                        if (!guid.IsValid()) return 0;
                        auto texOpt = assetManager->getAsset<Assets::Texture>(guid);
                        return (texOpt.has_value() && texOpt.value()) ? texOpt.value()->gl_texture : 0;
                    };

                    // Cache all texture handles (look up ONCE during initialization)
                    cache.albedoTexture = getTextureHandle(
                        material->useOverrides ? material->albedoTextureOverride :
                        assetManager->findGUID(materialAsset->albedoTexturePath)
                    );

                    cache.normalTexture = getTextureHandle(
                        material->useOverrides ? material->normalTextureOverride :
                        assetManager->findGUID(materialAsset->normalTexturePath)
                    );

                    cache.metallicTexture = getTextureHandle(
                        material->useOverrides ? material->metallicTextureOverride :
                        assetManager->findGUID(materialAsset->metallicTexturePath)
                    );

                    cache.roughnessTexture = getTextureHandle(
                        material->useOverrides ? material->roughnessTextureOverride :
                        assetManager->findGUID(materialAsset->roughnessTexturePath)
                    );

                    cache.aoTexture = getTextureHandle(
                        material->useOverrides ? material->aoTextureOverride :
                        assetManager->findGUID(materialAsset->aoTexturePath)
                    );

                    cache.emissiveTexture = getTextureHandle(
                        material->useOverrides ? material->emissiveTextureOverride :
                        assetManager->findGUID(materialAsset->emissiveTexturePath)
                    );

                    cache.opacityTexture = getTextureHandle(
                        material->useOverrides ? material->opacityTextureOverride :
                        assetManager->findGUID(materialAsset->opacityTexturePath)
                    );

                    cache.cacheValid = true;
                }
            }

            // Initialize bone transforms if animated
            if (!modelAsset->skeleton.empty() && component.boneTransforms.empty()) {
                component.boneTransforms.resize(modelAsset->skeleton.size(), glm::mat4(1.0f));
            }

            // Initialize morph weights
            if (!modelAsset->morphTargets.empty() && component.morphWeights.empty()) {
                component.morphWeights.resize(modelAsset->morphTargets.size(), 0.0f);
            }

            //PN_CORE_INFO("[Render System] Initialized ModelRenderer for entity {} with {} submeshes", (uint32_t)entity, component.materials.size());
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
                    glBindFramebuffer(GL_FRAMEBUFFER, services.lock()->get<sRenderer>()->getFinalFbo());
                    //glViewport(0, 0, fbWidth, fbHeight);
                }
                else {
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
            lcam.position = services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->pos;
            lcam.position.y += 0.1f;	// light on camera = grainy
            lcam.fov = services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->fov;
            lcam.direction = services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->forward;
            lcam.aspect_ratio = services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->aspect_ratio;

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
            if (!rendererService || !rendererService->w_renderer) return;

            glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), 
                       GraphicsSettings::get().getShadowMapWidth());

            auto renderGroup = registry.group<ModelRenderer>(
                entt::get<WorldTransform, Entity::Layer>
            );


            for (const Light& l : LightSources::get().getAll()) {
                if (l.getShadowType() != Light::SHADOW_TYPES::MAPPED) continue;

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
            if (!rendererService || !rendererService->w_renderer) return;

            auto svc = services.lock();
            if (!svc) return;

            auto sceneManager = svc->get<Scene::SceneManager>();
            if (!sceneManager) return;

            const auto& layers = sceneManager->getLayers();
            auto renderGroup = registry.group<ModelRenderer>(
                entt::get<WorldTransform, Entity::Layer>
            );

            if (!rendererService->m_Scene) {
                return;
            }

            rendererService->w_renderer->BeginGeometryPass(rendererService->m_Scene);

            // Use structured bindings with .each() for proper group iteration
            for (auto [entity, model, transform, layer] : renderGroup.each()) {
                // Components guaranteed to exist by group - no try_get needed!
                if (!model.visible) continue;

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
                rendererService->w_renderer->DrawGeometry(rendererService->m_Scene, const_cast<ModelRenderer&>(model), model_xform);
            }

            rendererService->w_renderer->EndGeometryPass();
        }

        void System::reflectionPass(entt::registry& registry) {

            auto rendererService = services.lock()->get<sRenderer>();
            if (!rendererService || !rendererService->w_renderer) return;

            auto renderGroup = registry.group<ModelRenderer>(
                entt::get<WorldTransform, Entity::Layer>
            );


            for (auto [entity, model, transform, layer] : renderGroup.each()) {
                rendererService->w_renderer->ReflectionPass(model);
            }
        }

        void System::lightingPass(entt::registry& registry) {

            auto rendererService = services.lock()->get<sRenderer>();
            if (!rendererService || !rendererService->w_renderer) return;

            auto svc = services.lock();
            if (!svc) return;

            auto lightingGroup = registry.group<Lighting>(
                entt::get<LocalTransform>
            );


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
                    light.setShadowType(static_cast<Light::SHADOW_TYPES>(lighting.shadow_type));

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
            if (!svc) return;

            auto scene = svc->get<Scene::SceneManager>();
            if (!scene) return;

            Camera* camera = scene->GetActiveCamera();
            if (!camera) return;

            // Mode 1: Draw Physics Colliders (Cyan)
            if (debug_mode == 1) {
                // Iterate entities with both RigidBody3D and Transform
                auto view = registry.view<Physics::RigidBody3D, LocalTransform>();
                glm::vec4 color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan for Physics

                for (auto [entity, rb, trans] : view.each()) {
                    
                    // 1. Calculate the scaled offset and half-extents in local space
                    // Note: Half extents = 0.5 * Transform Scale * Collider Scale
                    glm::vec3 offset_scaled = trans.scale * rb.collider_offset;
                    glm::vec3 half_extents = 0.5f * trans.scale * rb.collider_scale;
                    
                    // 2. Get rotation
                    glm::quat rotation = glm::normalize(trans.rotation);

                    // 3. Calculate the 8 corners of the OBB (Oriented Bounding Box) in World Space
                    // We do this because DebugPass draws AABBs, so we need to find the AABB that wraps our rotated physics box.
                    glm::vec3 min_aabb(std::numeric_limits<float>::max());
                    glm::vec3 max_aabb(std::numeric_limits<float>::lowest());

                    for (int i = 0; i < 8; ++i) {
                        glm::vec3 corner;
                        // Generate local box corner (+/- half_extents)
                        corner.x = (i & 1) ? half_extents.x : -half_extents.x;
                        corner.y = (i & 2) ? half_extents.y : -half_extents.y;
                        corner.z = (i & 4) ? half_extents.z : -half_extents.z;

                        // Apply Offset (in local space, relative to pivot) + Corner
                        glm::vec3 body_space_point = offset_scaled + corner;

                        // Apply Rotation and World Translation
                        // Point_World = World_Pos + Rotation * Point_Local
                        glm::vec3 world_point = trans.position + (rotation * body_space_point);

                        // Expand the AABB to fit this corner
                        min_aabb = glm::min(min_aabb, world_point);
                        max_aabb = glm::max(max_aabb, world_point);
                    }

                    // Draw the bounding box of the physics collider
                    rendererService->w_renderer->DebugPass(min_aabb, max_aabb, color, scene);
                }
            }

            // Mode 2: Draw BVH Tree Nodes
            if (debug_mode == 2) {
                auto ecsController = svc->get<ECS::Controller>();
                if (!ecsController) return;

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
                    if (nodeIndex == -1 || nodeIndex >= nodes.size() || nodes[nodeIndex].height == -1) return;

                    const BVHNode& node = nodes[nodeIndex];
                    glm::vec4 nodeColor;

                    if (node.isLeaf()) {
                        nodeColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for leaves
                    } else {
                        int colorIndex = depth % 6;
                        switch (colorIndex) {
                            case 0: nodeColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Red
                            case 1: nodeColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); break; // Orange
                            case 2: nodeColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow
                            case 3: nodeColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); break; // Cyan
                            case 4: nodeColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); break; // Blue
                            default: nodeColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); break; // Magenta
                        }
                    }
                    
                    rendererService->w_renderer->DebugPass(node.aabb.min, node.aabb.max, nodeColor, scene);

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
                        bounding_vol.worldAABB.min, 
                        bounding_vol.worldAABB.max, 
                        color, 
                        scene
                    );
                }
            }
        }

        void System::uiPass(entt::registry& registry) {

            auto rendererService = services.lock()->get<sRenderer>();
            if (!rendererService || !rendererService->w_renderer) return;

            //Texture and text groups
            auto texture_group = registry.group<Texture2D>(entt::get<UIElement, UIRectTransform>);
            auto text_group = registry.group<UIElement>(entt::get<UIText, UIRectTransform>);
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
            glDisable(GL_CULL_FACE);  // UI usually doesn't need culling

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

                if (!ui_elem.b_is_enabled) continue;

                auto texture_opt = services.lock()->get<Assets::Manager>()->getAsset<Assets::Texture>(texture_comp.texture_guid);
                if (!texture_opt.has_value()) continue;

                // Only auto-set size_delta if it's not already set
                if (rect_comp.size_delta.x == 0.0f || rect_comp.size_delta.y == 0.0f) {
                    rect_comp.size_delta.x = texture_opt.value().get()->width;
                    rect_comp.size_delta.y = texture_opt.value().get()->height;
                }

                rendererService->w_renderer->Render2DTexture(texture_opt.value()->gl_texture, texture_comp.pos, texture_comp.texture_scale);
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

                if (!ui_elem.b_is_enabled) continue;

                auto font_opt = services.lock()->get<Assets::Manager>()->getAsset<Assets::Fonts::FontFace>(text_comp.font_guid);
                if (!font_opt.has_value()) continue;

                PN_CORE_INFO("[Render System] Rendering Font: {}", font_opt.value()->name);

                text_comp.text_pos = rect_comp.calculated_world_position;
                text_comp.scale_factor = rect_comp.scale.x;

                TextRenderer::get().renderText(text_comp);

                // CHECK FOR ERRORS AFTER EACH TEXT RENDER
                err = glGetError();
                if (err != GL_NO_ERROR) {
                    PN_CORE_ERROR("[Render System] GL error after rendering font '{}': 0x{:X}",
                        font_opt.value()->name, err);
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
