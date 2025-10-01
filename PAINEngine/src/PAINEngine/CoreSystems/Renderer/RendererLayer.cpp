#include "RendererLayer.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "Applications/Application.h"


#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/Material.h"

//For imgui viewport
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "LayeredSystems/LevelEditor/Editor.h"


#include "CoreSystems/Path/Path.h"

namespace PAIN {

	void RendererLayer::onAttach() {

		WindowsRenderer::get().Init();
				
		//PN_CORE_INFO("jspoh attach r2");

		// Create framebuffer for ImGui viewport
		//glGenFramebuffers(1, &fbo);
		//glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		//// Create color texture
		//glGenTextures(1, &fboTexture);
		//glBindTexture(GL_TEXTURE_2D, fboTexture);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbWidth, fbHeight, 0,
		//	GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		//	GL_TEXTURE_2D, fboTexture, 0);

		//// Create depth-stencil buffer
		//glGenRenderbuffers(1, &rbo);
		//glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
		//	GL_RENDERBUFFER, rbo);

		//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		//	PN_CORE_ERROR("Framebuffer is not complete!");
		//}

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//#endif

		//Init scene
		m_Scene = std::make_shared<Scene>();

		glm::mat4 transform1 = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0.f));
		glm::mat4 transform2 = glm::translate(glm::mat4(1.f), glm::vec3(2.f, 1.f, 0.f));


		if (m_Scene) {
			m_Scene->AddObject(Mesh::LoadObj("ogre.obj"), transform1);
			m_Scene->AddObject(Mesh::LoadObj(), transform2);
		}

	}

	void RendererLayer::onUpdate(AppTiming timing) {
		const float dt = timing.dt;

		//#else
		//if (w_renderer) 
		{
			auto editor = services->get<Editor::Editor>();
			bool editor_visible = editor && editor->isVisible();

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

			// Render skybox and light
			WindowsRenderer::get().Render(m_Scene);

			glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset
		}

#ifdef PN_PLATFORM_ANDROID
		move_mode = LIGHT;
		light.move_mode = Light::MOVE_MODES::ORBIT_ORIGIN;
		A_KEYDOWN = true;
#endif


		switch (move_mode) {
		case CAMERA:
			if (Camera::get().move_mode == Camera::MOVE_MODES::ORBIT_ORIGIN) {
				// spherical
				float radius = glm::length(Camera::get().pos);
				float theta = atan2(Camera::get().pos.z, Camera::get().pos.x);
				float phi = acos(Camera::get().pos.y / radius);

				if (W_KEYDOWN) radius -= Camera::get().speed * dt;
				if (S_KEYDOWN) radius += Camera::get().speed * dt;
				if (A_KEYDOWN) theta += 1.5f * dt;
				if (D_KEYDOWN) theta -= 1.5f * dt;
				if (SPACE_KEYDOWN) phi -= 1.5f * dt;
				if (LCTRL_KEYDOWN) phi += 1.5f * dt;

				// clamp phi
				phi = glm::clamp(phi, 0.01f, glm::pi<float>() - 0.01f);

				// cartesian
				Camera::get().pos.x = radius * sin(phi) * cos(theta);
				Camera::get().pos.y = radius * cos(phi);
				Camera::get().pos.z = radius * sin(phi) * sin(theta);

				// look at origin
				Camera::get().forward = -glm::normalize(Camera::get().pos);
			}
			else {
				if (W_KEYDOWN) {
					glm::vec3 offset = Camera::get().forward * Camera::get().speed * dt;
					Camera::get().pos += offset;
				}
				if (S_KEYDOWN) {
					glm::vec3 offset = Camera::get().forward * Camera::get().speed * dt;
					Camera::get().pos -= offset;
				}
				if (A_KEYDOWN) {
					glm::vec3 offset = glm::normalize(glm::cross(Camera::get().forward, Camera::get().up)) * Camera::get().speed * dt;
					Camera::get().pos -= offset;
				}
				if (D_KEYDOWN) {
					glm::vec3 offset = glm::normalize(glm::cross(Camera::get().forward, Camera::get().up)) * Camera::get().speed * dt;
					Camera::get().pos += offset;
				}
				if (SPACE_KEYDOWN) {
					glm::vec3 offset = Camera::get().up * Camera::get().speed * dt;
					Camera::get().pos += offset;
				}
				if (LCTRL_KEYDOWN) {
					glm::vec3 offset = Camera::get().up * Camera::get().speed * dt;
					Camera::get().pos -= offset;
				}
			}
			break;
		case LIGHT:
			static constexpr float light_speed = 15.f;
			static constexpr float angle_speed = 1.5f; // radians per second

			glm::vec3 forward, right, up;

			if (light.move_mode == Light::FREE) {
				forward = glm::vec3(0.f, 0.f, -1.f);
				right = glm::vec3(1.f, 0.f, 0.f);
				up = glm::vec3(0.f, 1.f, 0.f);

				if (W_KEYDOWN) {
					light.position += forward * light_speed * dt;
				}
				if (S_KEYDOWN) {
					light.position += -forward * light_speed * dt;
				}
				if (A_KEYDOWN) {
					light.position += -right * light_speed * dt;
				}
				if (D_KEYDOWN) {
					light.position += right * light_speed * dt;
				}
				if (SPACE_KEYDOWN) {
					light.position += up * light_speed * dt;
				}
				if (LCTRL_KEYDOWN) {
					light.position += -up * light_speed * dt;
				}
			}
			else if (light.move_mode == Light::ORBIT_ORIGIN) {
				// Convert current position to spherical
				float radius = glm::length(light.position);
				float theta = atan2(light.position.z, light.position.x);
				float phi = acos(light.position.y / radius);

				// Modify spherical coordinates
				if (W_KEYDOWN) radius -= light_speed * dt;
				if (S_KEYDOWN) radius += light_speed * dt;
				if (A_KEYDOWN) theta += angle_speed * dt;
				if (D_KEYDOWN) theta -= angle_speed * dt;
				if (SPACE_KEYDOWN) phi -= angle_speed * dt;
				if (LCTRL_KEYDOWN) phi += angle_speed * dt;

				// Clamp phi to avoid flipping
				phi = glm::clamp(phi, 0.01f, glm::pi<float>() - 0.01f);

				// Convert back to Cartesian
				light.position.x = radius * sin(phi) * cos(theta);
				light.position.y = radius * cos(phi);
				light.position.z = radius * sin(phi) * sin(theta);
			}

			break;
		}

		if (mouseButtonDown && xOffset != 0.f) {
			// transformation matrix(rotate)
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-Camera::get().sensitivity * xOffset), Camera::get().up);
			Camera::get().forward = glm::vec3(rot * glm::vec4(Camera::get().forward, 0.f));
		}
		if (mouseButtonDown && yOffset != 0.f) {
			// transformation matrix(rotate)
			const glm::vec3 right = -glm::normalize(glm::cross(Camera::get().forward, Camera::get().up));
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-Camera::get().sensitivity * yOffset), right);
			Camera::get().forward = glm::vec3(rot * glm::vec4(Camera::get().forward, 0.f));
		}
		xOffset = 0.f;
		yOffset = 0.f;
		//#endif
	}

	void RendererLayer::onEvent(Event::Event& e) {
		//#ifndef PN_PLATFORM_ANDROID
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
				Camera::get().move_mode = static_cast<Camera::MOVE_MODES>((Camera::get().move_mode + 1) % Camera::MOVE_MODES::NUM_MOVE_MODES);
				light.move_mode = static_cast<Light::MOVE_MODES>((light.move_mode + 1) % Light::MOVE_MODES::NUM_MOVE_MODES);
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
		//#endif
	}
}