#include "pch.h"
#include "ViewportPanel.h"
#include "ImGuizmo.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

#ifdef _DEBUG
#include "../Editor.h" 
#include "ECS/Controller.h"
#include "CoreSystems/Scene/Scene.h"
#include "EntityPanel.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

			ViewportPanel::ViewportPanel()
				: renderTexture(0), texWidth(0), texHeight(0), isInputPaused(true), isSimulationPaused(false),
				m_GizmoOperation(ImGuizmo::TRANSLATE), m_GizmoMode(ImGuizmo::WORLD)
			{
				name = "##ViewportPanel";

				flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
					ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
			}

			void ViewportPanel::nextWindowSettings() {
				// No fullscreen behavior here — keep it dockable like AudioPanel
			}

			void ViewportPanel::setRenderTexture(ImTextureID texID, int width, int height) {
				renderTexture = texID;
				texWidth = width;
				texHeight = height;
			}

			void ViewportPanel::onAttach()
			{
			}

			float ViewportPanel::getTimeScale() const {
				return isSimulationPaused ? 0.0f : 1.0f;
			}

			void ViewportPanel::onUpdate(AppTiming timing) {

				if (!renderTexture) return;

				// Larger initial size
				ImVec2 initialSize(1280, 720);
				ImGui::SetNextWindowSize(initialSize, ImGuiCond_FirstUseEver);

				// Begin viewport window
				if (ImGui::Begin("Scene Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

					// Toolbar with Play/Pause buttons and Gizmo controls
					ImGui::BeginChild("##ViewportToolbar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar);
					{
						auto editor = services->get<PAIN::Editor::Editor>();

						// Simulation controls
						if (ImGui::Button(editor->isPaused() ? "Play Scene" : "Pause Scene")) {
							editor->togglePause();
						}

						ImGui::SameLine();
						ImGui::Spacing();
						ImGui::SameLine();

						// Gizmo operation buttons
						if (ImGui::RadioButton("Translate (T)", m_GizmoOperation == ImGuizmo::TRANSLATE))
							m_GizmoOperation = ImGuizmo::TRANSLATE;
						ImGui::SameLine();
						if (ImGui::RadioButton("Rotate (R)", m_GizmoOperation == ImGuizmo::ROTATE))
							m_GizmoOperation = ImGuizmo::ROTATE;
						ImGui::SameLine();
						if (ImGui::RadioButton("Scale (Y)", m_GizmoOperation == ImGuizmo::SCALE))
							m_GizmoOperation = ImGuizmo::SCALE;

						ImGui::SameLine();
						ImGui::Spacing();
						ImGui::SameLine();

						// Mode toggle
						if (ImGui::RadioButton("World", m_GizmoMode == ImGuizmo::WORLD))
							m_GizmoMode = ImGuizmo::WORLD;
						ImGui::SameLine();
						if (ImGui::RadioButton("Local", m_GizmoMode == ImGuizmo::LOCAL))
							m_GizmoMode = ImGuizmo::LOCAL;
					}
					ImGui::EndChild();

					ImVec2 avail = ImGui::GetContentRegionAvail();

					// Maintain aspect ratio
					float aspect = (float)texWidth / (float)texHeight;
					ImVec2 size = avail;
					if (size.x / size.y > aspect) {
						size.x = size.y * aspect;
					}
					else {
						size.y = size.x / aspect;
					}

					// Get viewport position for ImGuizmo - capture BEFORE rendering the image
					ImVec2 viewportPos = ImGui::GetCursorScreenPos();

					// Flip Y because ImGui expects UVs differently than many renderers
					ImGui::Image(renderTexture, size, ImVec2(0, 1), ImVec2(1, 0));

					contentHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup
						| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
					isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

					// ========================================
					// === ImGuizmo for Selected Entity ===
					// ========================================

					auto editor = services->get<PAIN::Editor::Editor>();
					auto scene = services->get<Scene>();
					auto ecs = services->get<ECS::Controller>();

					// Get the EntityPanel to access selected entity
					auto entityPanel = services->get<EntityPanel>();

					// Only render gizmo if an entity is selected
					if (entityPanel) {
						entt::entity selectedEntity = entityPanel->getSelectedEntity();

						if (selectedEntity != entt::null) {
							// Get the transform component from the selected entity
							auto transformOpt = ecs->getEntityComponent<Transform>(selectedEntity);

							if (transformOpt.has_value()) {
								Transform& transform = transformOpt.value().get();

								// Get camera matrices from the active scene camera
								auto camera = scene->GetActiveCamera();
								if (camera) {
									// View matrix (using your Camera's view() function)
									glm::mat4 viewMatrix = camera->view();

									// Projection matrix (using your Camera's projection() function)
									glm::mat4 projectionMatrix = camera->projection();

									// Entity's transform matrix
									glm::mat4 modelMatrix = transform.getMatrix();

									// Configure ImGuizmo
									ImGuizmo::SetOrthographic(false);
									ImGuizmo::SetDrawlist();
									ImGuizmo::SetRect(viewportPos.x, viewportPos.y, size.x, size.y);

									// Hotkeys to switch operations
									if (ImGui::IsKeyPressed(ImGuiKey_T))
										m_GizmoOperation = ImGuizmo::TRANSLATE;
									if (ImGui::IsKeyPressed(ImGuiKey_R))
										m_GizmoOperation = ImGuizmo::ROTATE;
									if (ImGui::IsKeyPressed(ImGuiKey_Y))
										m_GizmoOperation = ImGuizmo::SCALE;

									// Draw and manipulate the gizmo
									ImGuizmo::Manipulate(
										glm::value_ptr(viewMatrix),
										glm::value_ptr(projectionMatrix),
										m_GizmoOperation,
										m_GizmoMode,
										glm::value_ptr(modelMatrix)
									);

									// If gizmo was manipulated, update the entity's transform
									if (ImGuizmo::IsUsing()) {
										// Decompose the modified matrix back into transform components
										float translation[3], rotation[3], scale[3];
										ImGuizmo::DecomposeMatrixToComponents(
											glm::value_ptr(modelMatrix),
											translation,
											rotation,
											scale
										);

										// Update the entity's transform component
										transform.position = glm::vec3(translation[0], translation[1], translation[2]);
										transform.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
										transform.scale = glm::vec3(scale[0], scale[1], scale[2]);

										// Optional: Display debug info
										ImGui::SetNextWindowPos(ImVec2(viewportPos.x + 10, viewportPos.y + size.y - 80));
										ImGui::SetNextWindowBgAlpha(0.8f);
										ImGui::BeginChild("##GizmoDebug", ImVec2(400, 70), true, ImGuiWindowFlags_NoScrollbar);
										ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Transforming Entity!");
										ImGui::Text("Position: %.2f, %.2f, %.2f", translation[0], translation[1], translation[2]);
										ImGui::Text("Rotation: %.2f, %.2f, %.2f", rotation[0], rotation[1], rotation[2]);
										ImGui::Text("Scale: %.2f, %.2f, %.2f", scale[0], scale[1], scale[2]);
										ImGui::EndChild();
									}
								}
							}
						}
					}

					// ========================================
					// === End ImGuizmo Code ===
					// ========================================

					// Forward input only when NOT paused AND the viewport wants it
					// Don't forward input if ImGuizmo is being used
					if (!isSimulationPaused && wantsInput() && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver()) {
						ImGuiIO& io = ImGui::GetIO();

						auto camera = services->get<sCameraController>();
						if (camera) {
							// Keyboard
							camera->W_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_W);
							camera->A_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_A);
							camera->S_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_S);
							camera->D_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_D);
							camera->SPACE_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_Space);
							camera->LCTRL_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

							// Mouse (LMB drag rotates in your code)
							camera->mouseButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

							// Provide per-frame mouse movement
							if (camera->mouseButtonDown) {
								camera->xOffset = io.MouseDelta.x;
								camera->yOffset = io.MouseDelta.y;
							}
						}
					}
					else {
						// When viewport loses focus/hover OR is paused OR gizmo is active, ensure keys don't "stick"
						if (auto camera = services->get<sCameraController>()) {
							camera->W_KEYDOWN = camera->A_KEYDOWN = camera->S_KEYDOWN = camera->D_KEYDOWN = false;
							camera->SPACE_KEYDOWN = camera->LCTRL_KEYDOWN = false;
							camera->mouseButtonDown = false;
						}
					}

				}
				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
