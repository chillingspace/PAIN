#include "sRenderer.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Scene/Scene.h"

#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/Material.h"

#include "ECS/Controller.h"

//For imgui viewport
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Audio/Audio.h"
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
		m_Scene = services->get<Scene>();
		
	}

	void sRenderer::shadowPass()
	{
		// populate shadow map first
		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<MetaData::EntityName>();

		glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), GraphicsSettings::get().getShadowMapWidth());

		// Im sure there is a better way to render shadows
		for (const Light& l : LightSources::get().getAll()) {

			if (l.getShadowType() != Light::SHADOW_TYPES::MAPPED) continue;

			w_renderer->BeginShadowPass(l);

			for (auto e : view) {

				auto transform = ecs->getEntityComponent<Transform>(e);

				auto mesh = ecs->getEntityComponent<MeshRenderer>(e);

				glm::mat4 model;
				if (transform.has_value())
				{
					model = transform.value().get().getMatrix();
				}

				if (mesh.has_value())
				{
					auto mesh_ptr = scene->getMesh(mesh->get().mesh_id);
					w_renderer->DrawShadows(mesh_ptr.get(), model, l); // uses shadow_shader

				}


			}
			w_renderer->EndShadowPass();
		}
	}

	void sRenderer::geometryPass()
	{
		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<MetaData::EntityName>();

		w_renderer->BeginGeometryPass(scene);
		for (auto e : view) {

			auto transform = ecs->getEntityComponent<Transform>(e);
			auto mesh = ecs->getEntityComponent<MeshRenderer>(e);
			glm::mat4 model;
			if (transform.has_value())
			{
				model = transform.value().get().getMatrix();
			}
			if (mesh.has_value())
			{
				auto mesh_ptr = scene->getMesh(mesh->get().mesh_id);
				w_renderer->DrawGeometry(m_Scene, mesh_ptr.get(), model);
			}

		}
		w_renderer->EndGeometryPass();
	}

	void sRenderer::lightingPass()
	{
		auto scene = services->get<Scene>();
		w_renderer->LightingPass(scene, LightSources::get());
	}
	void sRenderer::postProcessPass()
	{
		w_renderer->PostProcessPass();
	}

	void sRenderer::onUpdate(AppTiming timing) {

		{
#ifdef DEBUG
			auto editor = services->get<Editor::Editor>();
			bool editor_visible = editor && editor->isVisible();
#else
			bool editor_visible = false;
#endif

			if (editor_visible) {
				glBindFramebuffer(GL_FRAMEBUFFER, w_renderer->getFinalFbo());
				//glViewport(0, 0, fbWidth, fbHeight);
			}
			else {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// Match viewport to window size
				auto window = services->get<Window::Window>();
#ifdef PN_PLATFORM_WINDOWS
				glfwGetFramebufferSize((GLFWwindow*)window->getNativeWindow(), &winWidth, &winHeight);
				glViewport(0, 0, winWidth, winHeight);
#else
				ANativeWindow* nativeWindow = (ANativeWindow*)window->getNativeWindow();
				winWidth = ANativeWindow_getWidth(nativeWindow);
				winHeight = ANativeWindow_getHeight(nativeWindow);
				glViewport(0, 0, winWidth, winHeight);
#endif

			}

			// Clear
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Render all passes
			shadowPass();
			geometryPass();
			lightingPass();
			postProcessPass();

			// --- BVH Debug Drawing ---
			auto bvhSystem = services->get<sBVHSystem>(); // Get the BVH system

			// Check if BVH system, renderer, scene are valid, and if BVH is not empty
			if (bvhSystem && w_renderer && m_Scene && !bvhSystem->getBVH().isEmpty()) {
				const BVH& bvh = bvhSystem->getBVH();
				const std::vector<BVHNode>& nodes = bvh.getNodes();
				int rootIndex = bvh.getRootIndex();

				// Calculate the combined View-Projection matrix from the active camera
				glm::mat4 vpMatrix = m_Scene->GetActiveCamera()->projection() * m_Scene->GetActiveCamera()->view();

				// Define a recursive lambda function to traverse and draw BVH nodes
				std::function<void(int nodeIndex, int depth)> drawNodeRecursive =
					[&](int nodeIndex, int depth) {
					// Stop recursion if index is invalid or node is marked as free
					if (nodeIndex == -1 || nodeIndex >= nodes.size() || nodes[nodeIndex].height == -1) {
						return;
					}
					const BVHNode& node = nodes[nodeIndex];

					// Select color: green for leaves, varying colors for internal nodes based on depth
					glm::vec3 color;
					if (node.isLeaf()) {
						color = glm::vec3(0.0f, 1.0f, 0.0f); // Green
					} else {
						// Simple color cycling based on depth modulo 6
						int colorIndex = depth % 6;
						switch (colorIndex) {
							case 0: color = glm::vec3(1.0f, 0.0f, 0.0f); break; // Red
							case 1: color = glm::vec3(1.0f, 0.5f, 0.0f); break; // Orange
							case 2: color = glm::vec3(1.0f, 1.0f, 0.0f); break; // Yellow
							case 3: color = glm::vec3(0.0f, 1.0f, 1.0f); break; // Cyan
							case 4: color = glm::vec3(0.0f, 0.0f, 1.0f); break; // Blue
							default: color = glm::vec3(1.0f, 0.0f, 1.0f); break; // Magenta
						}
						color *= 0.8f; // Slightly dim internal nodes
					}

					// Call the renderer's function to draw the wireframe AABB
					w_renderer->DrawAABBWireframe(node.aabb, vpMatrix, color);

					// If it's an internal node, recurse for its children
					if (!node.isLeaf()) {
						drawNodeRecursive(node.child1Index, depth + 1);
						drawNodeRecursive(node.child2Index, depth + 1);
					}
				};

				// Prepare OpenGL state for drawing lines on top of the scene
				glDisable(GL_DEPTH_TEST); // Draw lines regardless of scene depth

				// Start the recursive drawing process from the root node
				drawNodeRecursive(rootIndex, 0);

				// Restore OpenGL state
				glEnable(GL_DEPTH_TEST); // Re-enable depth testing
			}
			// --- End BVH Debug Drawing ---

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
	}


	void sRenderer::onEvent(Event::Event& e) {}
}