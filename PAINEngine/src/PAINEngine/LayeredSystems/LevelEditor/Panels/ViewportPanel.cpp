#ifdef _DEBUG

#include "pch.h"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "ViewportPanel.h"
#include "ImGuizmo.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>


#include "../Editor.h" 
#include "ECS/Controller.h"
#include "CoreSystems/Scene/Scene.h"
#include "EntityPanel.h"

#include "Systems/Physics/sysPhysics.h"

namespace PAIN {

	// helper so viewport wont need to know jolt details
	static void SyncBodyToTransform(
		entt::entity e,
		ECS::Controller* ecs,
		const Transform& t,
		bool dragging // true while gizmo is moving, false on release
	) {
		if (!ecs) return;
		auto& reg = ecs->getRegistry();
		if (!reg.all_of<Physics::RigidBody3D>(e)) return;

		auto& rb = reg.get<Physics::RigidBody3D>(e);
		if (auto phys = ecs->getSystem<Physics::System>()) {
			phys->teleportBodyToTransform(e, t, rb);
		}
	}

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

			glm::vec3 ViewportPanel::getCameraPosition(const glm::mat4& viewMatrix) {
				glm::mat4 inverseView = glm::inverse(viewMatrix);
				return glm::vec3(inverseView[3]);
			}
			
			glm::vec3 ViewportPanel::screenToWorldRay(ImVec2 mousePos, ImVec2 viewportSize,
				const glm::mat4& view, const glm::mat4& projection) {

				float x = mousePos.x / viewportSize.x;
				float y = mousePos.y / viewportSize.y;

				// Remove the negative sign from ndcX
				float ndcX = x * 2.0f - 1.0f;  // Back to original
				float ndcY = (1.0f - y) * 2.0f - 1.0f;

				glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
				glm::vec4 rayEye = glm::inverse(projection) * rayClip;
				rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

				glm::vec4 rayWorldTemp = glm::inverse(view) * rayEye;
				glm::vec3 rayWorld = glm::vec3(rayWorldTemp.x, rayWorldTemp.y, rayWorldTemp.z);

				return glm::normalize(rayWorld);
			}

			// sphere ray intersect 
			bool ViewportPanel::rayIntersectsSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
				const glm::vec3& sphereCenter, float sphereRadius,
				float& distance) {
				glm::vec3 oc = rayOrigin - sphereCenter;

				float a = glm::dot(rayDir, rayDir);
				float b = 2.0f * glm::dot(oc, rayDir);
				float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;

				float discriminant = b * b - 4.0f * a * c;

				if (discriminant < 0.0f) {
					return false;
				}

				float t = (-b - sqrt(discriminant)) / (2.0f * a);

				if (t < 0.0f) {
					t = (-b + sqrt(discriminant)) / (2.0f * a);
					if (t < 0.0f) {
						return false;
					}
				}

				distance = t;
				return true;
			}

			// AABB ray intersect for picking
			bool ViewportPanel::rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
				const Transform& transform, float& distance) {
				glm::vec3 minBound = transform.position - transform.scale * 0.5f;
				glm::vec3 maxBound = transform.position + transform.scale * 0.5f;

				float tMin = 0.0f;
				float tMax = (std::numeric_limits<float>::max)();

				for (int i = 0; i < 3; i++) {
					if (abs(rayDir[i]) < 0.0001f) {
						if (rayOrigin[i] < minBound[i] || rayOrigin[i] > maxBound[i]) {
							return false;
						}
					}
					else {
						float t1 = (minBound[i] - rayOrigin[i]) / rayDir[i];
						float t2 = (maxBound[i] - rayOrigin[i]) / rayDir[i];

						if (t1 > t2) std::swap(t1, t2);

						tMin = std::max(tMin, t1);
						tMax = std::min(tMax, t2);

						if (tMin > tMax) {
							return false;
						}
					}
				}

				distance = tMin;
				return tMin >= 0.0f;
			}

			// ImGuizmo picking logic
			void ViewportPanel::performMousePicking(ImVec2 localMousePos, ImVec2 viewportSize) {
				auto scene = services->get<Scene>();
				auto camera = scene->GetActiveCamera();
				auto ecs = services->get<ECS::Controller>();

				if (!camera || !ecs || !m_EntityPanel) {
					return;
				}

				// Get camera matrices
				glm::mat4 viewMatrix = camera->view();
				glm::mat4 projMatrix = camera->projection();

				// Get ray origin and direction
				glm::vec3 rayOrigin = getCameraPosition(viewMatrix);
				glm::vec3 rayDirection = screenToWorldRay(localMousePos, viewportSize, viewMatrix, projMatrix);

				// Find closest entity
				entt::entity closestEntity = entt::null;
				float closestDistance = (std::numeric_limits<float>::max)();

				// Iterate through all entities with transforms
				auto view = ecs->getRegistry().view<Transform>();

				for (auto entity : view) {
					auto& transform = view.get<Transform>(entity);

					// Skip very large objects (likely background/floor)
					if (transform.scale.x > 10.0f || transform.scale.y > 10.0f || transform.scale.z > 10.0f) {
						continue;
					}

					float distance;

					// Use AABB only for accurate picking
					if (rayIntersectsAABB(rayOrigin, rayDirection, transform, distance)) {
						if (distance < closestDistance) {
							closestDistance = distance;
							closestEntity = entity;
						}
					}
				}

				// Update EntityPanel selection
				m_EntityPanel->setSelectedEntity(closestEntity);
			}


			void ViewportPanel::onUpdate(AppTiming timing) {
				if (!renderTexture) return;

				ImVec2 initialSize(1280, 720);
				ImGui::SetNextWindowSize(initialSize, ImGuiCond_FirstUseEver);

				if (ImGui::Begin("Scene Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

					auto editor = services->get<PAIN::Editor::Editor>();
					auto scene = services->get<Scene>();
					auto ecs = services->get<ECS::Controller>();

					// Toolbar
					ImGui::BeginChild("##ViewportToolbar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar);
					{
						if (ImGui::Button(editor->isPaused() ? "Play Scene" : "Pause Scene")) {
							editor->togglePause();
						}

						ImGui::SameLine();
						ImGui::Spacing();
						ImGui::SameLine();

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

						if (ImGui::RadioButton("World", m_GizmoMode == ImGuizmo::WORLD))
							m_GizmoMode = ImGuizmo::WORLD;
						ImGui::SameLine();
						if (ImGui::RadioButton("Local", m_GizmoMode == ImGuizmo::LOCAL))
							m_GizmoMode = ImGuizmo::LOCAL;
					}
					ImGui::EndChild();

					// Viewport rendering
					ImVec2 avail = ImGui::GetContentRegionAvail();
					float aspect = (float)texWidth / (float)texHeight;
					ImVec2 size = avail;

					if (size.x / size.y > aspect) {
						size.x = size.y * aspect;
					}
					else {
						size.y = size.x / aspect;
					}

					ImVec2 viewportPos = ImGui::GetCursorScreenPos();
					ImGui::Image(renderTexture, size, ImVec2(0, 1), ImVec2(1, 0));

					contentHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup
						| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
					isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

					// ========================================
					// === ImGuizmo - RENDER FIRST ===
					// ========================================
					if (m_EntityPanel) {
						entt::entity selectedEntity = m_EntityPanel->getSelectedEntity();

						if (selectedEntity != entt::null) {
							auto transformOpt = ecs->getEntityComponent<Transform>(selectedEntity);

							if (transformOpt.has_value()) {
								Transform& transform = transformOpt.value().get();
								auto camera = scene->GetActiveCamera();

								if (camera) {
									glm::mat4 viewMatrix = camera->view();
									glm::mat4 projectionMatrix = camera->projection();
									glm::mat4 modelMatrix = transform.getMatrix();

									ImGuizmo::SetOrthographic(false);
									ImGuizmo::SetDrawlist();
									ImGuizmo::SetRect(viewportPos.x, viewportPos.y, size.x, size.y);

									// Hotkeys
									if (ImGui::IsKeyPressed(ImGuiKey_T))
										m_GizmoOperation = ImGuizmo::TRANSLATE;
									if (ImGui::IsKeyPressed(ImGuiKey_R))
										m_GizmoOperation = ImGuizmo::ROTATE;
									if (ImGui::IsKeyPressed(ImGuiKey_Y))
										m_GizmoOperation = ImGuizmo::SCALE;

									ImGuizmo::SetGizmoSizeClipSpace(0.15f);

									// Setup snapping
									bool useSnap = ImGui::GetIO().KeyCtrl; // Hold Ctrl to enable snapping
									float snapValue = 0.5f;

									// Different snap values for different operations
									if (m_GizmoOperation == ImGuizmo::ROTATE) {
										snapValue = 45.0f; // Snap to 45 degrees for rotation
									}
									else if (m_GizmoOperation == ImGuizmo::TRANSLATE) {
										snapValue = 0.5f; // Snap to 0.5 units for translation
									}
									else if (m_GizmoOperation == ImGuizmo::SCALE) {
										snapValue = 0.1f; // Snap to 0.1 for scale
									}

									float snapValues[3] = { snapValue, snapValue, snapValue };

									// Draw the gizmo
									ImGuizmo::Manipulate(
										glm::value_ptr(viewMatrix),
										glm::value_ptr(projectionMatrix),
										m_GizmoOperation,
										m_GizmoMode,
										glm::value_ptr(modelMatrix),
										nullptr,
										useSnap ? snapValues : nullptr // Pass snap values if Ctrl is held
									);

									// FIXED: Cache values and only update the component being manipulated
									static bool wasUsing = false;
									static glm::f32vec3 cachedPosition;
									static glm::f32quat cachedRotation;
									static glm::f32vec3 cachedScale;

									static entt::entity lastSelectedEntity = entt::null;
									static glm::mat4 originalMatrix = glm::mat4(1.0f);

									bool isCurrentlyUsing = ImGuizmo::IsUsing();


									// Reset cache if entity changed
									if (selectedEntity != lastSelectedEntity) {
										wasUsing = false;
										lastSelectedEntity = selectedEntity;
									}

									// Just started using - cache the original values AND matrix
									if (isCurrentlyUsing && !wasUsing) {
										cachedPosition = transform.position;
										cachedRotation = transform.rotation;
										cachedScale = transform.scale;

										originalMatrix = modelMatrix;
									}

									// Get RigidBody ID for selected object if it has one
									auto rbOpt = ecs->getEntityComponent<Physics::RigidBody3D>(selectedEntity);

									// Currently manipulating
									if (isCurrentlyUsing) {
										float translation[3], rotation[3], scale[3];
										ImGuizmo::DecomposeMatrixToComponents(
											glm::value_ptr(modelMatrix),
											translation,
											rotation,
											scale
										);

										if (m_GizmoOperation == ImGuizmo::TRANSLATE) {

											transform.position = glm::vec3(translation[0], translation[1], translation[2]);
											transform.rotation = cachedRotation;
											transform.scale = cachedScale;

											SyncBodyToTransform(selectedEntity, ecs.get(), transform, /*dragging=*/true);

											// If object has RigidBody3D, it is a physics object, disable physics temporarily
											if (rbOpt.has_value()) {
												auto& rb = rbOpt.value().get();
												auto physics_system = ecs->getSystem<Physics::System>();

												if (physics_system) {
													JPH::BodyInterface& body_interface = physics_system->GetPhysicsSystem()->GetBodyInterface();

													body_interface.DeactivateBody(rb.bodyID);
													body_interface.SetMotionType(rb.bodyID, JPH::EMotionType::Kinematic, JPH::EActivation::DontActivate);

													JPH::RVec3 pos(transform.position.x, transform.position.y, transform.position.z);

													body_interface.SetPosition(rb.bodyID, pos, JPH::EActivation::DontActivate);
												}
											}
										}
										else if (m_GizmoOperation == ImGuizmo::ROTATE) {									

											transform.rotation = glm::quat(glm::radians(glm::vec3(rotation[0], rotation[1], rotation[2])));							
											transform.position = cachedPosition;
											transform.scale = cachedScale;

											SyncBodyToTransform(selectedEntity, ecs.get(), transform, /*dragging=*/true);

											if (rbOpt.has_value()) {
												auto& rb = rbOpt.value().get();
												auto physics_system = ecs->getSystem<Physics::System>();

												if (physics_system) {
													JPH::BodyInterface& body_interface = physics_system->GetPhysicsSystem()->GetBodyInterface();

													// Disable physics temporarily
													body_interface.DeactivateBody(rb.bodyID);
													body_interface.SetMotionType(rb.bodyID, JPH::EMotionType::Kinematic, JPH::EActivation::DontActivate);

													JPH::Quat rot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

													body_interface.SetRotation(rb.bodyID, rot, JPH::EActivation::DontActivate);
												}
											}
										}
										else if (m_GizmoOperation == ImGuizmo::SCALE) {
									
											transform.position = cachedPosition;
											transform.rotation = cachedRotation;
											transform.scale = glm::vec3(scale[0], scale[1], scale[2]);

											// Jolt does not allow run-time scale changing.
										}
									}

									// Just released - final update
									if (!isCurrentlyUsing && wasUsing) {
										float translation[3], rotation[3], scale[3];
										ImGuizmo::DecomposeMatrixToComponents(
											glm::value_ptr(modelMatrix),
											translation,
											rotation,
											scale
										);

										if (m_GizmoOperation == ImGuizmo::TRANSLATE) {
											transform.position = glm::vec3(translation[0], translation[1], translation[2]);
										}
										else if (m_GizmoOperation == ImGuizmo::ROTATE) {

											transform.rotation = glm::quat(glm::radians(glm::vec3(rotation[0], rotation[1], rotation[2])));

										}
										else if (m_GizmoOperation == ImGuizmo::SCALE) {
											transform.scale = glm::vec3(scale[0], scale[1], scale[2]);
										}

										// Reactivate physics for physics object
										if (rbOpt.has_value()) {
											auto& rb = rbOpt.value().get();
											auto physics_system = ecs->getSystem<Physics::System>();
											JPH::BodyInterface& body_interface = physics_system->GetPhysicsSystem()->GetBodyInterface();

											// Set back to dynamic (or whatever it was before)
											body_interface.SetMotionType(rb.bodyID, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);

											// Optional: ensure the body is awake
											body_interface.ActivateBody(rb.bodyID);
										}
									}

									wasUsing = isCurrentlyUsing;

								}
							}
						}
					}

					// ========================================
					// === Mouse Picking - AFTER GIZMO ===
					// ========================================
					if (contentHovered
						&& ImGui::IsMouseClicked(ImGuiMouseButton_Left)
						&& !ImGuizmo::IsUsing()
						&& !ImGuizmo::IsOver()) {

						ImVec2 mousePos = ImGui::GetMousePos();
						ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);

						performMousePicking(localMousePos, size);
					}

					// ========================================
					// === Camera Controls ===
					// ========================================

					ImGuiIO& io = ImGui::GetIO();
					auto camera = services->get<sCameraController>();

					if (camera) {

#ifdef PN_PLATFORM_WINDOWS
						bool rightMouseHeld = ImGui::IsMouseDown(ImGuiMouseButton_Right);
#else
						camera->m_vpHeight = size.y;
						camera->m_vpWidth = size.x;
						camera->m_vpPosX = viewportPos.x;
						camera->m_vpPosY = viewportPos.y;
						camera->vp_hovered = contentHovered;
						bool rightMouseHeld = contentHovered;
#endif 

						// Check if gizmo is active
						bool gizmoActive = ImGuizmo::IsUsing() || ImGuizmo::IsOver();

						// Camera controls ONLY work when RIGHT MOUSE is held (and conditions met)
						if (!isSimulationPaused && contentHovered && !gizmoActive && rightMouseHeld) {
							// Enable all camera movement keys
							camera->W_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_W);
							camera->A_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_A);
							camera->S_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_S);
							camera->D_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_D);
							camera->SPACE_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_Space);
							camera->LCTRL_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

							// Enable mouse look
							camera->mouseButtonDown = true;

#ifdef PN_PLATFORM_WINDOWS
							camera->xOffset = io.MouseDelta.x;
							camera->yOffset = -io.MouseDelta.y;
#endif
						}
						else {
							// Reset all camera inputs when right mouse NOT held
							camera->W_KEYDOWN = false;
							camera->A_KEYDOWN = false;
							camera->S_KEYDOWN = false;
							camera->D_KEYDOWN = false;
							camera->SPACE_KEYDOWN = false;
							camera->LCTRL_KEYDOWN = false;
							camera->mouseButtonDown = false;
							camera->xOffset = 0.0f;
							camera->yOffset = 0.0f;
						}

						// Mouse wheel zoom (works independently of right-click)
						if (contentHovered && !gizmoActive && io.MouseWheel != 0.0f) {
							float mouseWheel = io.MouseWheel;
							float zoomSpeed = 0.1f;
							auto activeCamera = scene->GetActiveCamera();

							if (activeCamera) {
								glm::mat4 mmtx = glm::scale(glm::mat4(1.f), glm::vec3(1, 0, 1));

								if (mouseWheel > 0.0f) {
									glm::vec3 offset = glm::vec3(mmtx * glm::vec4(activeCamera->forward, 1.f))
										* activeCamera->speed * zoomSpeed * mouseWheel;
									activeCamera->pos += offset;
								}
								else if (mouseWheel < 0.0f) {
									glm::vec3 offset = glm::vec3(mmtx * glm::vec4(activeCamera->forward, 1.f))
										* activeCamera->speed * zoomSpeed * abs(mouseWheel);
									activeCamera->pos -= offset;
								}
							}
						}
					}


					
				}
				ImGui::End();
			}



		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
