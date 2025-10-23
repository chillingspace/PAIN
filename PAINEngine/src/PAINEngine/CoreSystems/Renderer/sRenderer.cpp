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

			// Render all
			// populate shadow map first
			auto ecs = services->get<ECS::Controller>();
			auto scene = services->get<Scene>();

			glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), GraphicsSettings::get().getShadowMapWidth());

			// Im sure there is a better way to render shadows
			for (const Light& l : LightSources::get().getAll()) {
				if (l.getShadowType() == Light::SHADOW_TYPES::MAPPED) {
					glBindFramebuffer(GL_FRAMEBUFFER, l.getShadowFbo());
					//glClearDepth(1.0f);  // Explicitly set clear value
					glClear(GL_DEPTH_BUFFER_BIT);

					// Use EnTT view to iterate all entities with EntityName component
					auto& registry = ecs->getRegistry();
					auto view = registry.view<MetaData::EntityName>();

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
							w_renderer->RenderGeometryShadows(mesh_ptr.get(), model, l); // uses shadow_shader
						
						}
						

					}
					
				}
			}

			// render scene
			w_renderer->BeginRendering(m_Scene);

			// Use EnTT view to iterate all entities with EntityName component
			auto& registry = ecs->getRegistry();
			auto view = registry.view<MetaData::EntityName>();

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
						// uses geometry_shader
					w_renderer->RenderGeometry(m_Scene, mesh_ptr.get(), model);
					
				}

			}
			
			w_renderer->EndRendering(m_Scene);

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