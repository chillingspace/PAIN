#include "sRenderer.h"
#include "Core.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/Material.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Renderer/skybox.h"

#include "ECS/Controller.h"
#include "ECS/Components/cBoundingVolume.h"

#include "CoreSystems/Windows/GLFW/GLFWWindow.h"

//For imgui viewport
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#include "Systems/Collision/sBVHSystem.h"

namespace PAIN {
	void sRenderer::onDetach()
	{
		w_renderer = nullptr;
	}
	void sRenderer::onAttach() {

		//Create window render
		w_renderer = std::make_unique<WindowsRenderer>();

		w_renderer->Init(services);

		//Init scene
		m_Scene = services->get<Scene::SceneManager>();
		
		//Call update one frame to ensure initialization
		onUpdate(AppTiming());

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after sRenderer attach: {}", err);
		}
	}

	void sRenderer::InitializeModelRenderer(entt::entity entity, ModelRenderer& component) {

		//Get asset manager
		auto assetManager = services->get<Assets::Manager>();

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

	void sRenderer::shadowPass()
	{
		// populate shadow map first
		auto ecs = services->get<ECS::Controller>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<Entity::Name>();

		glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), GraphicsSettings::get().getShadowMapWidth());

		// Im sure there is a better way to render shadows
		for (const Light& l : LightSources::get().getAll()) {

			if (l.getShadowType() != Light::SHADOW_TYPES::MAPPED) continue;

			w_renderer->BeginShadowPass(l);

			for (auto e : view) {

				auto transform = ecs->getEntityComponent<WorldTransform>(e);

				auto mdl = ecs->getEntityComponent<ModelRenderer>(e);

				glm::mat4 model_xform;
				if (transform.has_value())
				{
					model_xform = transform.value().get().matrix;
				}

				if (mdl.has_value() && mdl->get().castShadows)
				{
					w_renderer->DrawShadows(mdl->get(), model_xform, l); // uses shadow_shader

				}


			}
			w_renderer->EndShadowPass();
		}
	}

	void sRenderer::geometryPass()
	{
		auto ecs = services->get<ECS::Controller>();
		auto sceneManager = services->get<Scene::SceneManager>();
		const auto& layers = sceneManager->getLayers();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<Entity::Name>();

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before geometry pass: {}", err);
		}

		w_renderer->BeginGeometryPass(m_Scene);
		for (auto e : view) {

  			auto transform = ecs->getEntityComponent<WorldTransform>(e);
			auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
			if (!mdl.has_value() || !mdl->get().visible) continue;

			// Check layer visibility
			if (auto layerComp = ecs->getEntityComponent<Entity::Layer>(e)) {
				int layerID = layerComp->get().layer_id;
				if (layerID < layers.size() && !layers[layerID].enabled) {
					continue;
				}
			}

			glm::mat4 model_xform{ 1.f };
			if (transform.has_value())
			{
				model_xform *= transform.value().get().matrix;
			}
			if (mdl.has_value())
			{
				//Init component if not ready or if model GUID has been updated
				if (mdl->get().modelGUID != mdl->get().prevModelGUID) {
					InitializeModelRenderer(e, mdl->get());
				}

				// Skip if not visible or not ready
				if (!mdl->get().visible) {
					continue;
				}

				//Draw geo
				w_renderer->DrawGeometry(m_Scene, mdl->get(), model_xform);
			}
		}
		w_renderer->EndGeometryPass();

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after geometry pass: {}", err);
		}
	}

	void sRenderer::reflectionPass()
	{
		auto ecs = services->get<ECS::Controller>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<Entity::Name>();

		for (auto e : view) {

			auto transform = ecs->getEntityComponent<WorldTransform>(e);
			auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
			glm::mat4 model_xform;
			if (transform.has_value())
			{
				model_xform = transform.value().get().matrix;
			}
			if (mdl.has_value())
			{
				w_renderer->ReflectionPass(mdl->get());
			}

		}
	}

	void sRenderer::lightingPass()
	{
		auto ecs = services->get<ECS::Controller>();
		auto& registry = ecs->getRegistry();
		auto view = registry.view<Entity::Name>();

		// Cache to track which lights are still active this frame
		std::unordered_set<std::string> activeLightNames;


		for (auto entity : view) {
			auto light_comp = ecs->getEntityComponent<Lighting>(entity);
			auto trans_comp = ecs->getEntityComponent<LocalTransform>(entity);

			if (!light_comp.has_value() || !trans_comp.has_value())
				continue;

			std::string lightName = "light_" + std::to_string((uint32_t)entity);
			activeLightNames.insert(lightName);

			// Create light if new
			if (!LightSources::get().get(lightName)) {
				LightSources::get().create(lightName);
			}

			if (auto lightOpt = LightSources::get().get(lightName)) {
				Light& light = lightOpt.value();
				light.position = trans_comp->get().position + light_comp->get().offset;
				light.L_intensity = light_comp->get().light_intensity;
				light.type = static_cast<Light::TYPES>(light_comp->get().light_type);
				light.setShadowType(static_cast<Light::SHADOW_TYPES>(light_comp->get().shadow_type));
			}

		}

		// Remove lights no longer active
		for (auto& [key, lightRef] : LightSources::get().getAllWithKeys()) {

			std::string const& name = key;
			Light& light = lightRef.get();

			// Skip the special lights "cam" and "world"
			if (name == "cam" || name == "world") {
				continue;
			}

			if (activeLightNames.find(name) == activeLightNames.end()) {
				LightSources::get().destroy(name);
			}
		}

		auto scene = services->get<Scene::SceneManager>();
		w_renderer->LightingPass(scene, LightSources::get());

		//Skybox::get().render(scene->GetActiveCamera()->view(), scene->GetActiveCamera()->projection());
	}

	void sRenderer::debugPass(int debug_mode)
	{
		// Mode 0 is OFF
		if (debug_mode == 0) { return; }

		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene::SceneManager>();
		
		if (!ecs || !scene || !w_renderer) {
			PN_CORE_WARN("DebugPass skipped: Missing required services.");
			return;
		}

		Camera* camera = scene->GetActiveCamera();
		if (!camera) {
			PN_CORE_WARN("DebugPass skipped: No active camera.");
			return;
		}

		// --- Option A (Mode 1): Draw World AABBs from cBoundingVolume ---
		if (debug_mode == 1)
		{
			auto& registry = ecs->getRegistry();
			// Iterate over BoundingVolume, not EntityName
			auto view = registry.view<BoundingVolume>();
			glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for AABBs

			for (auto entity : view) {
				// Get component directly from the view
				auto& bounding_vol = view.get<BoundingVolume>(entity);
				w_renderer->DebugPass(bounding_vol.worldAABB.min, bounding_vol.worldAABB.max, color, scene);
			}
		}



		// --- Option B (Mode 2): Draw BVH Tree Nodes ---
		if (debug_mode == 2)
		{

			auto bvhSystem = ecs->getSystem<sBVHSystem>(); // Get BVH system from ECS

			if (!bvhSystem) { // This check is what's firing in your log
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
					switch (colorIndex) { // Cycle colors for internal nodes
						case 0: nodeColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Red
						case 1: nodeColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); break; // Orange
						case 2: nodeColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow
						case 3: nodeColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); break; // Cyan
						case 4: nodeColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); break; // Blue
						default: nodeColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); break; // Magenta
					}
				}
				w_renderer->DebugPass(node.aabb.min, node.aabb.max, nodeColor, scene);

				if (!node.isLeaf()) {
					drawNodeRecursive(node.child1Index, depth + 1);
					drawNodeRecursive(node.child2Index, depth + 1);
				}
			};


			if (rootIndex != -1) {
				drawNodeRecursive(rootIndex, 0);
			}

		}
		// --- End Option B ---
	}


	void sRenderer::postProcessPass()
	{
		w_renderer->PostProcessPass();
	}


	void sRenderer::onUpdate(AppTiming timing) {

		{
#ifdef _DEBUG
			auto editor = services->get<Editor::Editor>();
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
				glBindFramebuffer(GL_FRAMEBUFFER, w_renderer->getFinalFbo());
				//glViewport(0, 0, fbWidth, fbHeight);
			}
			else {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// Match viewport to window size
				auto window = services->get<Window::Window>();
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
			shadowPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after shadow pass: {}", err);
			}
			geometryPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after geometry pass: {}", err);
			}
			reflectionPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after reflection pass: {}", err);
			}
			lightingPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after lighting pass: {}", err);
			}
		
			debugPass(editor_debug_mode);
			postProcessPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after post process pass: {}", err);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset
		}

		// set cam light to cam
		auto olcam = LightSources::get().get("cam");
		Light& lcam = olcam.value();
		lcam.position = m_Scene->GetActiveCamera()->pos;
		lcam.position.y += 0.1f;	// light on camera = grainy
		lcam.fov = m_Scene->GetActiveCamera()->fov;
		lcam.forward = m_Scene->GetActiveCamera()->forward;
		lcam.aspect_ratio = m_Scene->GetActiveCamera()->aspect_ratio;

		GLenum err = glGetError();
		while (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err on update loop end: {}", err);
		}
	}


	void sRenderer::onEvent(Event::Event& e) {
#ifdef PN_PLATFORM_WINDOWS
		if (e.getType() == Event::Type::WindowResize) {
			//PN_CORE_INFO("window resized");

			//glfwGetWindowSize(Window::GLFW_Window::getWindow(), &WindowsRenderer::winWidth, &WindowsRenderer::winHeight);

			w_renderer->Cleanup();
			w_renderer->Init(services);
		}
#endif
	}

}