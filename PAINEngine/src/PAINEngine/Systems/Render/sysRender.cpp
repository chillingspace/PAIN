#include "pch.h"
#include "sysRender.h"

#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Controller.h"
#include "ECS/Components/AllComponents.h"
#include "Systems/Collision/sBVHSystem.h"
#include "ECS/Components/cAnimation.h" 

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

            // Initialize bone transforms if animated
            if (!modelAsset->skeleton.empty() && component.boneTransforms.empty()) {
                component.boneTransforms.resize(modelAsset->skeleton.size(), glm::mat4(1.0f));
            }

            // Initialize morph weights
            if (!modelAsset->morphTargets.empty() && component.morphWeights.empty()) {
                component.morphWeights.resize(modelAsset->morphTargets.size(), 0.0f);
            }

            PN_CORE_INFO("Initialized ModelRenderer for entity {} with {} submeshes",
                (uint32_t)entity, component.materials.size());
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
            lcam.forward = services.lock()->get<Scene::SceneManager>()->GetActiveCamera()->forward;
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

            auto view = registry.view<Entity::Name>();
            glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), 
                       GraphicsSettings::get().getShadowMapWidth());

            for (const Light& l : LightSources::get().getAll()) {
                if (l.getShadowType() != Light::SHADOW_TYPES::MAPPED) continue;

                rendererService->w_renderer->BeginShadowPass(l);

                for (auto e : view) {
                    auto transform = registry.try_get<WorldTransform>(e);
                    auto mdl = registry.try_get<ModelRenderer>(e);

                    glm::mat4 model_xform(1.0f);
                    if (transform) {
                        model_xform = transform->matrix;
                    }

                    if (mdl && mdl->castShadows) {
                        rendererService->w_renderer->DrawShadows(*mdl, model_xform, l);
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
            auto view = registry.view<Entity::Name>();

            rendererService->w_renderer->BeginGeometryPass(rendererService->m_Scene);

            for (auto e : view) {
                auto transform = registry.try_get<WorldTransform>(e);
                auto mdl = registry.try_get<ModelRenderer>(e);

                if (!mdl || !mdl->visible) continue;

                // Check layer visibility
                auto layerComp = registry.try_get<Entity::Layer>(e);
                if (layerComp) {
                    int layerID = layerComp->layer_id;
                    if (layerID < layers.size() && !layers[layerID].enabled) {
                        continue;
                    }
                }

                glm::mat4 model_xform(1.0f);
                if (transform) {
                    model_xform = transform->matrix;
                }

                // Initialize component if model GUID changed
                if (mdl->modelGUID != mdl->prevModelGUID) {
                    InitializeModelRenderer(e, *const_cast<ModelRenderer*>(mdl));
                }

                if (!mdl->visible) {
                    continue;
                }


                // Get Animation component if present
                Animation* anim = registry.try_get<Animation>(e);

                // Draw geometry, passing animation pointer (may be null)
                rendererService->w_renderer->DrawGeometry(rendererService->m_Scene, *mdl, model_xform);
            }

            rendererService->w_renderer->EndGeometryPass();
        }


        void System::reflectionPass(entt::registry& registry) {
            auto rendererService = services.lock()->get<sRenderer>();
            if (!rendererService || !rendererService->w_renderer) return;

            auto view = registry.view<Entity::Name>();

            for (auto e : view) {
                auto transform = registry.try_get<WorldTransform>(e);
                auto mdl = registry.try_get<ModelRenderer>(e);

                if (mdl) {
                    rendererService->w_renderer->ReflectionPass(*mdl);
                }
            }
        }

        void System::lightingPass(entt::registry& registry) {
            auto rendererService = services.lock()->get<sRenderer>();
            if (!rendererService || !rendererService->w_renderer) return;

            auto svc = services.lock();
            if (!svc) return;

            auto view = registry.view<Entity::Name>();

            // Cache to track which lights are still active this frame
            std::unordered_set<std::string> activeLightNames;

            for (auto entity : view) {
                auto light_comp = registry.try_get<Lighting>(entity);
                auto trans_comp = registry.try_get<LocalTransform>(entity);

                if (!light_comp || !trans_comp) continue;

                std::string lightName = "light_" + std::to_string((uint32_t)entity);
                activeLightNames.insert(lightName);

                // Create light if new
                if (!LightSources::get().get(lightName)) {
                    LightSources::get().create(lightName);
                }

                if (auto lightOpt = LightSources::get().get(lightName)) {
                    Light& light = lightOpt.value();
                    light.position = trans_comp->position + light_comp->offset;
                    light.L_intensity = light_comp->light_intensity;
                    light.type = static_cast<Light::TYPES>(light_comp->light_type);
                    light.setShadowType(static_cast<Light::SHADOW_TYPES>(light_comp->shadow_type));
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

            // Mode 1: Draw World AABBs from cBoundingVolume
            if (debug_mode == 1) {
                auto view = registry.view<BoundingVolume>();
                glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for AABBs

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
        }

    } // namespace Render
} // namespace PAIN
