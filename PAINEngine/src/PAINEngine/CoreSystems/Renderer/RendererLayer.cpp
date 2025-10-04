#include "RendererLayer.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "Applications/Application.h"

#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/Material.h"

#include "ECS/Controller.h"

//For imgui viewport
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "LayeredSystems/LevelEditor/Editor.h"


#include "CoreSystems/Path/Path.h"

namespace PAIN {
	void RendererLayer::onDetach()
	{
	}
	void RendererLayer::onAttach() {

		WindowsRenderer::get().Init();

		//Init scene
		m_Scene = services->get<Scene>();
		auto ogre_obj = Mesh::LoadObj("ogre.obj");

		if (m_Scene) {
			m_Scene->AddObject(ogre_obj, {0.f, 1.f, 0.f}, {0.f,0.f,0.f, 0.f}, {1.f, 1.f, 1.f});
			m_Scene->AddObject(ogre_obj, { 2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
			m_Scene->AddObject(ogre_obj, { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		}

	}

	void RendererLayer::onUpdate(AppTiming timing) {
		const float dt = timing.dt;

		{
#ifdef DEBUG
			auto editor = services->get<Editor::Editor>();
			bool editor_visible = editor && editor->isVisible();
#else
			bool editor_visible = false;
#endif

			if (editor_visible) {
				glBindFramebuffer(GL_FRAMEBUFFER, WindowsRenderer::get().getFinalFbo());
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

			glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), GraphicsSettings::get().getShadowMapWidth());

			// Im sure there is a better way to render shadows
			for (const Light& l : LightSources::get().getAll()) {
				if (l.getShadowType() == Light::SHADOW_TYPES::MAPPED) {
					glBindFramebuffer(GL_FRAMEBUFFER, l.getShadowFbo());
					//glClearDepth(1.0f);  // Explicitly set clear value
					glClear(GL_DEPTH_BUFFER_BIT);

					for (auto entity : ecs->getAllEntities()) {
						auto transform = ecs->getEntityComponent<Transform>(entity);

						auto mesh = ecs->getEntityComponent<MeshRenderer>(entity);

						glm::mat4 model;
						if (transform.has_value())
						{
							model = transform.value().get().getMatrix();
						}

						if (mesh)
						{
							if (mesh.value().get().mesh)
							{
								WindowsRenderer::get().RenderGeometryShadows(mesh.value().get().mesh.get(), model, l); // uses shadow_shader
							}	
						}
						

					}
					
				}
			}

			WindowsRenderer::get().BeginRendering(m_Scene);
			// render scene


			for (auto entity : ecs->getAllEntities()) {
				auto transform = ecs->getEntityComponent<Transform>(entity);
				auto mesh = ecs->getEntityComponent<MeshRenderer>(entity);
				glm::mat4 model;
				if (transform.has_value())
				{
					model = transform.value().get().getMatrix();
				}
				if (mesh)
				{
					if (mesh.value().get().mesh)
					{
						// uses geometry_shader
						WindowsRenderer::get().RenderGeometry(m_Scene, mesh.value().get().mesh.get(), model);
					}
					
				}
				

			}
			

			
			WindowsRenderer::get().EndRendering(m_Scene);

			glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset
		}


		switch (move_mode) {
		case CAMERA:
			if (m_Scene->GetActiveCamera()->move_mode == Camera::MOVE_MODES::ORBIT_ORIGIN) {
				// spherical
				float radius = glm::length(m_Scene->GetActiveCamera()->pos);
				float theta = atan2(m_Scene->GetActiveCamera()->pos.z, m_Scene->GetActiveCamera()->pos.x);
				float phi = acos(m_Scene->GetActiveCamera()->pos.y / radius);

				if (W_KEYDOWN) radius -= m_Scene->GetActiveCamera()->speed * dt;
				if (S_KEYDOWN) radius += m_Scene->GetActiveCamera()->speed * dt;
				if (A_KEYDOWN) theta += 1.5f * dt;
				if (D_KEYDOWN) theta -= 1.5f * dt;
				if (SPACE_KEYDOWN) phi -= 1.5f * dt;
				if (LCTRL_KEYDOWN) phi += 1.5f * dt;

				// clamp phi
				phi = glm::clamp(phi, 0.01f, glm::pi<float>() - 0.01f);

				// cartesian
				m_Scene->GetActiveCamera()->pos.x = radius * sin(phi) * cos(theta);
				m_Scene->GetActiveCamera()->pos.y = radius * cos(phi);
				m_Scene->GetActiveCamera()->pos.z = radius * sin(phi) * sin(theta);

				// look at origin
				m_Scene->GetActiveCamera()->forward = -glm::normalize(m_Scene->GetActiveCamera()->pos);
			}
			else {
				static glm::mat4 mmtx = glm::scale(glm::mat4(1.f), glm::vec3(1, 0, 1));
				if (W_KEYDOWN) {
					glm::vec3 offset = glm::vec3(mmtx * glm::vec4(m_Scene->GetActiveCamera()->forward, 1.f)) * m_Scene->GetActiveCamera()->speed * dt;
					m_Scene->GetActiveCamera()->pos += offset;
				}
				if (S_KEYDOWN) {
					glm::vec3 offset = glm::vec3(mmtx * glm::vec4(m_Scene->GetActiveCamera()->forward, 1.f)) * m_Scene->GetActiveCamera()->speed * dt;
					m_Scene->GetActiveCamera()->pos -= offset;
				}
				if (A_KEYDOWN) {
					glm::vec3 offset = glm::normalize(glm::cross(m_Scene->GetActiveCamera()->forward, m_Scene->GetActiveCamera()->up)) * m_Scene->GetActiveCamera()->speed * dt;
					m_Scene->GetActiveCamera()->pos -= offset;
				}
				if (D_KEYDOWN) {
					glm::vec3 offset = glm::normalize(glm::cross(m_Scene->GetActiveCamera()->forward, m_Scene->GetActiveCamera()->up)) * m_Scene->GetActiveCamera()->speed * dt;
					m_Scene->GetActiveCamera()->pos += offset;
				}
				if (SPACE_KEYDOWN) {
					glm::vec3 offset = m_Scene->GetActiveCamera()->up * m_Scene->GetActiveCamera()->speed * dt;
					m_Scene->GetActiveCamera()->pos += offset;
				}
				if (LCTRL_KEYDOWN) {
					glm::vec3 offset = m_Scene->GetActiveCamera()->up * m_Scene->GetActiveCamera()->speed * dt;
					m_Scene->GetActiveCamera()->pos -= offset;
				}
			}
			break;
		}

		if (mouseButtonDown && xOffset != 0.f) {
			// transformation matrix(rotate)
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-m_Scene->GetActiveCamera()->sensitivity * xOffset), m_Scene->GetActiveCamera()->up);
			m_Scene->GetActiveCamera()->forward = glm::normalize(glm::vec3(rot * glm::vec4(m_Scene->GetActiveCamera()->forward, 0.f)));
		}
		if (mouseButtonDown && yOffset != 0.f) {
			// transformation matrix(rotate)
			const glm::vec3 right = -glm::normalize(glm::cross(m_Scene->GetActiveCamera()->forward, m_Scene->GetActiveCamera()->up));
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-m_Scene->GetActiveCamera()->sensitivity * yOffset), right);
			m_Scene->GetActiveCamera()->forward = glm::normalize(glm::vec3(rot * glm::vec4(m_Scene->GetActiveCamera()->forward, 0.f)));
		}
		xOffset = 0.f;
		yOffset = 0.f;
		//#endif


		// set cam light to cam
		auto olcam = LightSources::get().get("cam");
		Light& lcam = olcam.value();
		lcam.position = m_Scene->GetActiveCamera()->pos;
		lcam.position.y += 0.1f;	// light on camera = grainy
		lcam.fov = m_Scene->GetActiveCamera()->fov;
		lcam.target = m_Scene->GetActiveCamera()->forward;
		lcam.aspect_ratio = m_Scene->GetActiveCamera()->aspect_ratio;
	}

	void RendererLayer::renderScene()
	{
		//auto ecs = services->get<ECS::Controller>();
		//auto drawable_entities = ecs->getEntitiesWithComponents<Transform>();

		//for (auto entity : drawable_entities) {
		//	auto& transform = ecs->getEntityComponent<Transform>(entity)->get();
		//	RenderGeometry(scene, mesh, transform.matrix); // or however your Transform stores it
		//}
	}

	void RendererLayer::onEvent(Event::Event& e) {
#ifndef PN_PLATFORM_ANDROID
		Event::Dispatcher dispatcher(e);

		dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {

			switch (e.getKeyCode()) {
			case PAIN_KEY_W:
				W_KEYDOWN = true;
				break;
			case PAIN_KEY_A:
				A_KEYDOWN = true;
				break;
			case PAIN_KEY_S:
				S_KEYDOWN = true;
				break;
			case PAIN_KEY_D:
				D_KEYDOWN = true;
				break;
			case PAIN_KEY_SPACE:
				SPACE_KEYDOWN = true;
				break;
			case PAIN_KEY_LEFT_CONTROL:
				LCTRL_KEYDOWN = true;
				break;
			default:
				break;
			}


			return false;
			});

		dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {
			//PN_CORE_INFO(e.toString());
			switch (e.getKeyCode()) {
			case PAIN_KEY_W:
				W_KEYDOWN = false;
				break;
			case PAIN_KEY_A:
				A_KEYDOWN = false;
				break;
			case PAIN_KEY_S:
				S_KEYDOWN = false;
				break;
			case PAIN_KEY_D:
				D_KEYDOWN = false;
				break;
			case PAIN_KEY_SPACE:
				SPACE_KEYDOWN = false;
				break;
			case PAIN_KEY_LEFT_CONTROL:
				LCTRL_KEYDOWN = false;
				break;
			default:
				break;
			}
			return false;
			});

		dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {

			switch (e.getKeyCode()) {
			case PAIN_KEY_TAB:
				move_mode = static_cast<MOVE_MODES>((move_mode + 1) % NUM_MOVE_MODES);
				break;
			case PAIN_KEY_O:
				m_Scene->GetActiveCamera()->move_mode = static_cast<Camera::MOVE_MODES>((m_Scene->GetActiveCamera()->move_mode + 1) % Camera::MOVE_MODES::NUM_MOVE_MODES);
				break;
			default:
				break;
			}

			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
			//PN_CORE_INFO(e.toString());

			if (e.getBtnCode() == PAIN_MOUSE_BUTTON_LEFT) {
				mouseButtonDown = true;
			}

			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {

			if (e.getBtnCode() == PAIN_MOUSE_BUTTON_LEFT) {
				mouseButtonDown = false;
			}

			return false;
			});

		dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
			static float lastX = 0.0f;
			static float lastY = 0.0f;

			xOffset = e.getWindowPos().x - lastX;
			yOffset = lastY - e.getWindowPos().y; // reversed since y-coordinates go from bottom to top


			lastX = e.getWindowPos().x;
			lastY = e.getWindowPos().y;

			return false;
			});

		dispatcher.Dispatch<Event::WindowFocused>([&](Event::WindowFocused& e) -> bool {
			PN_CORE_INFO(e.toString());
			return false;
			});
#endif
	}
}