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
#include "ResourcePanel.h"
#include "ECS/Components/cMeshRenderer.h"

#include "Systems/Physics/sysPhysics.h"
#include "ECS/Components/cEntity.h"
#include "Systems/Transform/sysTransform.h"

#include "CoreSystems/Prefabs/sPrefab.h"
#include "CoreSystems/EntityTemplate/sEntityTemplate.h"


namespace PAIN {

	// helper so viewport wont need to know jolt details
	static void SyncBodyToTransform(
		entt::entity e,
		entt::registry& registry,
		ECS::Controller* ecs,
		const LocalTransform& t,
		bool dragging // true while gizmo is moving, false on release
	) {
		if (!ecs) return;
		if (!registry.all_of<Physics::RigidBody3D>(e)) return;

		auto& rb = registry.get<Physics::RigidBody3D>(e);
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

			glm::vec3 ViewportPanel::getWorldPositionAtMouse(ImVec2 localMousePos,ImVec2 viewportSize,float defaultDistance) {
				auto scene = services->get<Scene::SceneManager>();
				auto camera = scene->GetActiveCamera();
				auto ecs = services->get<ECS::Controller>();

				if (!camera || !ecs) {
					return glm::vec3(0.0f);  // Fallback to origin
				}

				// Get camera matrices
				glm::mat4 viewMatrix = camera->view();
				glm::mat4 projMatrix = camera->projection();

				// Get ray
				glm::vec3 rayOrigin = getCameraPosition(viewMatrix);
				glm::vec3 rayDirection = screenToWorldRay(localMousePos, viewportSize, viewMatrix, projMatrix);

				// ========================================
				// TRY 1: Find intersection with existing geometry
				// ========================================
				float closestDistance = std::numeric_limits<float>::max();
				bool foundHit = false;

				auto view = ecs->getRegistry(currentRegistryID).view<LocalTransform, ModelRenderer>();

				for (auto [entity, transform, model] : view.each()) {
					if (!model.visible) continue;

					// Try AABB intersection
					float distance;
					if (rayIntersectsAABB(rayOrigin, rayDirection, transform, distance)) {
						if (distance < closestDistance) {
							closestDistance = distance;
							foundHit = true;
						}
					}
				}

				// ========================================
				// TRY 2: If hit found, use that position
				// ========================================
				if (foundHit) {
					return rayOrigin + (rayDirection * closestDistance);
				}

				// ========================================
				// TRY 3: No hit - project onto ground plane (Y=0)
				// ========================================
				// Plane equation: dot(N, (P - P0)) = 0
				// For Y=0 plane: N = (0, 1, 0), P0 = (0, 0, 0)

				glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
				glm::vec3 planePoint(0.0f, 0.0f, 0.0f);

				float denom = glm::dot(rayDirection, planeNormal);

				if (abs(denom) > 0.0001f) {
					// Ray intersects plane
					float t = glm::dot((planePoint - rayOrigin), planeNormal) / denom;

					if (t >= 0.0f) {
						// Valid intersection
						return rayOrigin + (rayDirection * t);
					}
				}

				// ========================================
				// FALLBACK: Fixed distance in front of camera
				// ========================================
				return rayOrigin + (rayDirection * defaultDistance);
			}

			void ViewportPanel::handlePrefabDrop(File* prefabFile,ImVec2 localMousePos,ImVec2 viewportSize) {
				auto prefabService = services->get<Prefab::Service>();
				auto assetManager = services->get<Assets::Manager>();
				auto ecs = services->get<ECS::Controller>();

				if (!prefabService || !prefabFile || !assetManager || !ecs) {
					PN_CORE_ERROR("[ViewportPanel] Required services not available for prefab drop");
					return;
				}

				// ========================================
				// Get world position at mouse cursor
				// ========================================
				glm::vec3 worldPosition = getWorldPositionAtMouse(localMousePos, viewportSize, 10.0f);

				PN_CORE_INFO("[ViewportPanel] Dropping prefab at world position: ({}, {}, {})",
					worldPosition.x, worldPosition.y, worldPosition.z);

				// ========================================
				// Instantiate prefab at that position
				// ========================================
				Assets::GUID prefabGUID = prefabFile->id;

				entt::entity instantiatedEntity = prefabService->instantiatePrefab(
					prefabGUID,
					currentRegistryID,
					worldPosition  // <-- Pass the world position!
				);

				if (instantiatedEntity != entt::null) {
					PN_CORE_INFO("[ViewportPanel] Successfully instantiated prefab '{}' as entity {}",
						prefabFile->file_name,
						static_cast<uint32_t>(instantiatedEntity));

					// Select the newly created entity
					if (m_EntityPanel) {
						m_EntityPanel->setSelectedEntity(instantiatedEntity);
					}
				}
				else {
					PN_CORE_ERROR("[ViewportPanel] Failed to instantiate prefab '{}'", prefabFile->file_name);
				}
			}
			
			void ViewportPanel::handleTemplateDrop(File* prefabFile, ImVec2 localMousePos, ImVec2 viewportSize) {
				auto templateService = services->get<EntityTemplate::Service>();
				auto assetManager = services->get<Assets::Manager>();
				auto ecs = services->get<ECS::Controller>();

				if (!templateService || !prefabFile || !assetManager || !ecs) {
					PN_CORE_ERROR("[ViewportPanel] Required services not available for prefab drop");
					return;
				}

				// ========================================
				// Get world position at mouse cursor
				// ========================================
				glm::vec3 worldPosition = getWorldPositionAtMouse(localMousePos, viewportSize, 10.0f);

				PN_CORE_INFO("[ViewportPanel] Dropping template at world position: ({}, {}, {})",
					worldPosition.x, worldPosition.y, worldPosition.z);

				// ========================================
				// Instantiate prefab at that position
				// ========================================
				Assets::GUID prefabGUID = prefabFile->id;

				entt::entity instantiatedEntity = templateService->spawn(
					prefabGUID,
					currentRegistryID,
					worldPosition
				);

				if (instantiatedEntity != entt::null) {
					PN_CORE_INFO("[ViewportPanel] Successfully instantiated template '{}' as entity {}",
						prefabFile->file_name,
						static_cast<uint32_t>(instantiatedEntity));

					// Select the newly created entity
					if (m_EntityPanel) {
						m_EntityPanel->setSelectedEntity(instantiatedEntity);
					}
				}
				else {
					PN_CORE_ERROR("[ViewportPanel] Failed to instantiate template '{}'", prefabFile->file_name);
				}
			}

			void ViewportPanel::onAttach()
			{
				//Init with reference to entity and comp panel
				m_EntityPanel = services->get<Editor>()->getPanel<EntityPanel>();
#ifdef PN_PLATFORM_WINDOWS
				m_comp_panel = services->get<Editor>()->getPanel<ComponentsPanel>();
				m_prefab_panel = services->get<Editor>()->getPanel<PrefabPanel>();
#endif
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
				const LocalTransform& transform, float& distance) {
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

			// Helper method to find entity at mouse position - IMPROVED VERSION
			entt::entity ViewportPanel::findEntityAtMousePos(ImVec2 localMousePos, ImVec2 viewportSize) {
				auto scene = services->get<Scene::SceneManager>();
				auto camera = scene->GetActiveCamera();
				auto ecs = services->get<ECS::Controller>();

				if (!camera || !ecs) {
					return entt::null;
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

				// Iterate through entities with ModelRenderer (more precise than just Transform)
				auto view = ecs->getRegistry(currentRegistryID).view<LocalTransform, WorldTransform, ModelRenderer>();

				for (auto [entity, local, world, model] : view.each()) {

					// Skip invisible objects
					if (!model.visible) {
						continue;
					}

					// Skip very large objects (likely background/floor)
					if (local.scale.x > 10.0f || local.scale.y > 10.0f || local.scale.z > 10.0f) {
						continue;
					}

					float distance;

					// First do a quick AABB test
					if (!rayIntersectsAABB(rayOrigin, rayDirection, local, distance)) {
						continue;
					}

					// For more precision, use a tighter bounding sphere based on model scale
					glm::vec3 sphereCenter = local.position;
					float sphereRadius = glm::length(local.scale) * 0.5f; // More conservative radius

					float sphereDistance;
					if (rayIntersectsSphere(rayOrigin, rayDirection, sphereCenter, sphereRadius, sphereDistance)) {
						// Use the sphere distance for more accurate sorting
						if (sphereDistance < closestDistance) {
							closestDistance = sphereDistance;
							closestEntity = entity;
						}
					}
				}

				// Fallback to entities with just Transform (in case some don't have ModelRenderer)
				if (closestEntity == entt::null) {
					auto transformOnlyView = ecs->getRegistry(currentRegistryID).view<LocalTransform>();

					for (auto entity : transformOnlyView) {
						// Skip if we already checked this entity
						if (ecs->getRegistry(currentRegistryID).all_of<ModelRenderer>(entity)) {
							continue;
						}

						auto& transform = transformOnlyView.get<LocalTransform>(entity);

						// Skip very large objects
						if (transform.scale.x > 10.0f || transform.scale.y > 10.0f || transform.scale.z > 10.0f) {
							continue;
						}

						float distance;
						if (rayIntersectsAABB(rayOrigin, rayDirection, transform, distance)) {
							if (distance < closestDistance) {
								closestDistance = distance;
								closestEntity = entity;
							}
						}
					}
				}

				return closestEntity;
			}


			// ImGuizmo picking logic
			void ViewportPanel::performMousePicking(ImVec2 localMousePos, ImVec2 viewportSize) {
				auto scene = services->get<Scene::SceneManager>();
				auto camera = scene->GetActiveCamera();
				auto ecs = services->get<ECS::Controller>();

				if (!camera || !ecs || !m_EntityPanel) {
					return;
				}

				entt::entity closestEntity = findEntityAtMousePos(localMousePos, viewportSize);

				// Update EntityPanel selection
				m_EntityPanel->setSelectedEntity(closestEntity);
			}

			void ViewportPanel::handleMaterialDrop(File* materialFile,
				ImVec2 localMousePos,
				ImVec2 viewportSize) {
				auto scene = services->get<Scene::SceneManager>();
				auto camera = scene->GetActiveCamera();
				auto ecs = services->get<ECS::Controller>();
				auto assetService = services->get<Assets::Manager>();

				if (!camera || !ecs || !materialFile || !assetService) {
					return;
				}

				// Find entity at drop location
				entt::entity closestEntity = findEntityAtMousePos(localMousePos, viewportSize);

				// Apply material to the closest entity
				if (closestEntity != entt::null) {
					// Check if entity has a ModelRenderer component
					if (ecs->getRegistry(currentRegistryID).all_of<ModelRenderer>(closestEntity)) {
						auto& modelRenderer = ecs->getRegistry(currentRegistryID).get<ModelRenderer>(closestEntity);

						// Apply the material to all submeshes (you can modify this logic)
						if (modelRenderer.materials.empty()) {
							// If no materials exist, create one
							MaterialInstance matInstance;
							matInstance.materialGUID = materialFile->id;
							modelRenderer.materials.push_back(matInstance);

							PN_CORE_INFO("Applied material '{}' to entity (new material instance)", materialFile->file_name);
						}
						else {
							// Apply to first material (or you could apply to all)
							modelRenderer.materials[0].materialGUID = materialFile->id;

							PN_CORE_INFO("Applied material '{}' to entity (replaced first material)", materialFile->file_name);
						}
					}
					else {
						PN_CORE_WARN("Entity does not have a ModelRenderer component");
					}
				}
				else {
					PN_CORE_INFO("No entity found at drop location");
				}
			}

			bool isPointInsideRect(const ImVec2& point, const ImVec2& rectMin, const ImVec2& rectMax) {
				return point.x >= rectMin.x && point.x <= rectMax.x &&
					point.y >= rectMin.y && point.y <= rectMax.y;
			}

			void ViewportPanel::onUpdate(AppTiming timing) {
				if (!renderTexture) return;

				ImVec2 initialSize(1280, 720);
				ImGui::SetNextWindowSize(initialSize, ImGuiCond_FirstUseEver);

				if (ImGui::Begin("Scene Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

					ImGuizmo::BeginFrame();

					auto editor = services->get<PAIN::Editor::Editor>();
					auto scene = services->get<Scene::SceneManager>();
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

#ifdef PN_PLATFORM_WINDOWS

						ImGui::SameLine();
						ImGui::Spacing();
						ImGui::SameLine();

						//Toggle mode
						if (m_prefab_panel->getEditingMode()) {

							//Toggle between registries
							if (ImGui::SmallButton(currentRegistryID == ECS::MAIN_REGISTRY_ID ? "Edit Prefab" : "Edit Scene")) {
								//Set prev registry to auto simulate
								ecs->setRegistryAutoSimulate(currentRegistryID, false);
								if (currentRegistryID != ECS::MAIN_REGISTRY_ID) {
									currentRegistryID = ECS::MAIN_REGISTRY_ID;
								}
								else {
									currentRegistryID = m_prefab_panel->getEditRegistryID();
								}
								//Set curr registry to auto simulate
								ecs->setRegistryAutoSimulate(currentRegistryID, true);
							}

							//Set comp and entity panel registries
							if (m_comp_panel->getCurrentRegistry() != currentRegistryID) m_comp_panel->setRegistry(currentRegistryID);
							if (m_EntityPanel->getCurrentRegistry() != currentRegistryID) m_EntityPanel->setRegistry(currentRegistryID);
						}
						else {

							//Ensure main registry auto simulates
							if(!ecs->isRegistryAutoSimulate(ECS::MAIN_REGISTRY_ID))ecs->setRegistryAutoSimulate(ECS::MAIN_REGISTRY_ID, true);

							//Reset registry IDs
							if(currentRegistryID != ECS::MAIN_REGISTRY_ID) currentRegistryID = ECS::MAIN_REGISTRY_ID;

							//Reset registry IDs
							if (m_comp_panel->getCurrentRegistry() != currentRegistryID) m_comp_panel->setRegistry(currentRegistryID);
							if (m_EntityPanel->getCurrentRegistry() != currentRegistryID) m_EntityPanel->setRegistry(currentRegistryID);
						}
#endif
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
					// === Camera Controls ===
					// ========================================

					ImGuiIO& io = ImGui::GetIO();
					auto cameraController = services->get<sCameraController>();

					if (cameraController) {

#ifdef PN_PLATFORM_WINDOWS
						bool rightMouseHeld = ImGui::IsMouseDown(ImGuiMouseButton_Right);
#else
						cameraController->m_vpHeight = size.y;
						cameraController->m_vpWidth = size.x;
						cameraController->m_vpPosX = viewportPos.x;
						cameraController->m_vpPosY = viewportPos.y;
						cameraController->vp_hovered = contentHovered;
						bool rightMouseHeld = contentHovered;
#endif 

						// Check if gizmo is currently being manipulated (object gizmo OR view manipulate)
						bool gizmoActive = ImGuizmo::IsUsing() || ImGuizmo::IsUsingViewManipulate();

						if (!isSimulationPaused && contentHovered && !gizmoActive && rightMouseHeld) {
							cameraController->W_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_W);
							cameraController->A_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_A);
							cameraController->S_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_S);
							cameraController->D_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_D);
							cameraController->SPACE_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_Space);
							cameraController->LCTRL_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

							cameraController->mouseButtonDown = true;

#ifdef PN_PLATFORM_WINDOWS
							cameraController->xOffset = io.MouseDelta.x;
							cameraController->yOffset = -io.MouseDelta.y;
#endif
						}
						else {
							cameraController->W_KEYDOWN = false;
							cameraController->A_KEYDOWN = false;
							cameraController->S_KEYDOWN = false;
							cameraController->D_KEYDOWN = false;
							cameraController->SPACE_KEYDOWN = false;
							cameraController->LCTRL_KEYDOWN = false;
							cameraController->mouseButtonDown = false;
							cameraController->xOffset = 0.0f;
							cameraController->yOffset = 0.0f;
						}

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

                    // ========================================
                    // === DRAG & DROP TARGET FOR MATERIALS ===
                    // ========================================

                    // Reset drag hover entity
                    m_DragHoveredEntity = entt::null;

                    // Declare isUsingGizmo once at the top
					bool isUsingGizmo = false;

					if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
						isUsingGizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver() || ImGuizmo::IsUsingViewManipulate();

					}

                    if (!isUsingGizmo) {
                        // Use invisible button overlay for reliable drag-drop
                        ImGui::SetCursorScreenPos(viewportPos);

						// Define cube rect
						float cubeSize = 128.0f;
						ImVec2 cubePos(viewportPos.x + size.x - cubeSize - 10.0f, viewportPos.y + 10.0f);
						ImVec2 cubeRectMin = cubePos;
						ImVec2 cubeRectMax = ImVec2(cubePos.x + cubeSize, cubePos.y + cubeSize);

						// Only create drag-drop target if mouse NOT over cube rect to avoid blocking cube's mouse events
						if (!isPointInsideRect(ImGui::GetMousePos(), cubeRectMin, cubeRectMax)) {
							ImGui::SetCursorScreenPos(viewportPos);
							if(size.x != 0 && size.y != 0) ImGui::InvisibleButton("##ViewportDropZone", size);

							auto mat_filetype_string = PAIN::Assets::assetTypeToString(Assets::Type::Material);
							if (ImGui::BeginDragDropTarget()) {
								// Check what payload is available
								if (const ImGuiPayload* payload = ImGui::GetDragDropPayload()) {
									if (strcmp(payload->DataType, std::string(mat_filetype_string + "_FILE").c_str()) == 0) {
										m_DragHoveredEntity = findEntityAtMousePos(
											ImVec2(ImGui::GetMousePos().x - viewportPos.x, ImGui::GetMousePos().y - viewportPos.y),
											size
										);

										if (m_DragHoveredEntity != entt::null) {
											ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
										}
									}
								}

								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(mat_filetype_string + "_FILE").c_str())) {
									PN_CORE_INFO("Material payload accepted!");
									File* droppedFile = (File*)payload->Data;

									ImVec2 mousePos = ImGui::GetMousePos();
									ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);

									handleMaterialDrop(droppedFile, localMousePos, size);
								}
								ImGui::EndDragDropTarget();
							}

							auto prefab_filetype_string = PAIN::Assets::assetTypeToString(Assets::Type::Prefabs);
							if (ImGui::BeginDragDropTarget()) {
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
									std::string(prefab_filetype_string + "_FILE").c_str())) {

									PN_CORE_INFO("Prefab payload accepted!");
									File* droppedFile = (File*)payload->Data;

									ImVec2 mousePos = ImGui::GetMousePos();
									ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);

									handlePrefabDrop(droppedFile, localMousePos, size);
								}
								ImGui::EndDragDropTarget();
							}

							auto template_filetype_string = PAIN::Assets::assetTypeToString(Assets::Type::Templates);
							if (ImGui::BeginDragDropTarget()) {
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
									std::string(template_filetype_string + "_FILE").c_str())) {

									PN_CORE_INFO("Template payload accepted!");
									File* droppedFile = (File*)payload->Data;

									ImVec2 mousePos = ImGui::GetMousePos();
									ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);

									handleTemplateDrop(droppedFile, localMousePos, size);
								}
								ImGui::EndDragDropTarget();
							}

							auto model_filetype_string = PAIN::Assets::assetTypeToString(Assets::Type::Model);
							if (ImGui::BeginDragDropTarget()) {
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
									std::string(model_filetype_string + "_FILE").c_str())) {

									PN_CORE_INFO("Prefab payload accepted!");
									File* droppedFile = (File*)payload->Data;

									ImVec2 mousePos = ImGui::GetMousePos();
									ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);

									//Get world position for placing model
									glm::vec3 worldPosition = getWorldPositionAtMouse(localMousePos, size, 10.0f);

									//Create an entity
									entt::entity entity = ecs->createEntity(currentRegistryID); // Auto-assigns GUID

									//Craft entity name
									std::string e_name = droppedFile->file_name.substr(0, droppedFile->file_name.find_first_of('.'));
									e_name += std::to_string(static_cast<int>(entity));

									// Add core components
									ecs->addEntityComponent(entity, Entity::Name{ e_name }, currentRegistryID);
									ecs->addEntityComponent(entity, LocalTransform{}, currentRegistryID);
									ecs->addEntityComponent(entity, WorldTransform{}, currentRegistryID);
									ecs->addEntityComponent(entity, Entity::Hierarchy{}, currentRegistryID);
									ecs->addEntityComponent(entity, ModelRenderer{ droppedFile->id }, currentRegistryID);

									//Move created model
									auto l_trans_opt = ecs->getEntityComponent<LocalTransform>(entity, currentRegistryID);
									if (l_trans_opt.has_value()) {
										l_trans_opt.value().get().position = worldPosition;
									}

									PN_CORE_INFO("[ViewportPanel] Dropping model at world position: ({}, {}, {})",
										worldPosition.x, worldPosition.y, worldPosition.z);
								}
								ImGui::EndDragDropTarget();
							}
						}

                        // Update hover state from invisible button
                        contentHovered = contentHovered || ImGui::IsItemHovered();
                    }

					// ========================================
					// === ImGuizmo - RENDER FIRST ===
					// ========================================
					if (m_EntityPanel) {
						entt::entity selectedEntity = m_EntityPanel->getSelectedEntity();

						if (selectedEntity != entt::null) {
							auto localTransformOpt = ecs->getEntityComponent<LocalTransform>(selectedEntity, currentRegistryID);
							auto worldTransformOpt = ecs->getEntityComponent<WorldTransform>(selectedEntity, currentRegistryID);

							if (localTransformOpt.has_value() && worldTransformOpt.has_value()) {
								LocalTransform& localTransform = localTransformOpt.value().get();
								WorldTransform& worldTransform = worldTransformOpt.value().get();
								auto camera = scene->GetActiveCamera();

								if (camera) {
									glm::mat4 viewMatrix = camera->view();
									glm::mat4 projectionMatrix = camera->projection();
									glm::mat4 modelMatrix = worldTransform.matrix;

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
									bool useSnap = ImGui::GetIO().KeyCtrl;
									float snapValue = 0.5f;

									if (m_GizmoOperation == ImGuizmo::ROTATE) {
										snapValue = 45.0f;
									}
									else if (m_GizmoOperation == ImGuizmo::TRANSLATE) {
										snapValue = 0.5f;
									}
									else if (m_GizmoOperation == ImGuizmo::SCALE) {
										snapValue = 0.1f;
									}

									float snapValues[3] = { snapValue, snapValue, snapValue };

									// Draw the gizmo (manipulates modelMatrix)
									ImGuizmo::Manipulate(
										glm::value_ptr(viewMatrix),
										glm::value_ptr(projectionMatrix),
										m_GizmoOperation,
										m_GizmoMode,
										glm::value_ptr(modelMatrix),
										nullptr,
										useSnap ? snapValues : nullptr
									);

									// Cache management
									static bool wasUsing = false;
									static glm::vec3 cachedPosition;
									static glm::quat cachedRotation;
									static glm::vec3 cachedScale;
									static entt::entity lastSelectedEntity = entt::null;

									bool isCurrentlyUsing = ImGuizmo::IsUsing();

									// Reset cache if entity changed
									if (selectedEntity != lastSelectedEntity) {
										wasUsing = false;
										lastSelectedEntity = selectedEntity;
									}

									// Just started using - cache original values
									if (isCurrentlyUsing && !wasUsing) {
										cachedPosition = localTransform.position;
										cachedRotation = localTransform.rotation;
										cachedScale = localTransform.scale;
									}

									// Currently manipulating
									if (isCurrentlyUsing) {
										// ===================================================
										// STEP 1: Get parent's world matrix (if exists)
										// ===================================================
										glm::mat4 parentWorldMatrix = glm::mat4(1.0f);

										auto hierarchyOpt = ecs->getEntityComponent<Entity::Hierarchy>(selectedEntity, currentRegistryID);
										if (hierarchyOpt.has_value()) {
											const Entity::Hierarchy& hierarchy = hierarchyOpt.value().get();
											if (hierarchy.parentGUID.IsValid()) {
												entt::entity parentEntity = ecs->resolveGUID(hierarchy.parentGUID, currentRegistryID);
												if (parentEntity != entt::null) {
													auto parentWorldOpt = ecs->getEntityComponent<WorldTransform>(parentEntity, currentRegistryID);
													if (parentWorldOpt.has_value()) {
														parentWorldMatrix = parentWorldOpt.value().get().matrix;
													}
												}
											}
										}

										// ===================================================
										// STEP 2: Convert world matrix to local space
										// ===================================================
										glm::mat4 newLocalMatrix = glm::inverse(parentWorldMatrix) * modelMatrix;

										// ===================================================
										// STEP 3: Decompose local matrix
										// ===================================================
										float localTranslation[3], localRotation[3], localScale[3];
										ImGuizmo::DecomposeMatrixToComponents(
											glm::value_ptr(newLocalMatrix),
											localTranslation,
											localRotation,
											localScale
										);

										// ===================================================
										// STEP 4: Update LocalTransform based on gizmo operation
										// ===================================================
										if (m_GizmoOperation == ImGuizmo::TRANSLATE) {
											localTransform.position = glm::vec3(localTranslation[0], localTranslation[1], localTranslation[2]);
											localTransform.rotation = cachedRotation;
											localTransform.scale = cachedScale;
										}
										else if (m_GizmoOperation == ImGuizmo::ROTATE) {
											localTransform.position = cachedPosition;
											localTransform.rotation = glm::quat(glm::radians(glm::vec3(localRotation[0], localRotation[1], localRotation[2])));
											localTransform.scale = cachedScale;
										}
										else if (m_GizmoOperation == ImGuizmo::SCALE) {
											localTransform.position = cachedPosition;
											localTransform.rotation = cachedRotation;
											localTransform.scale = glm::vec3(localScale[0], localScale[1], localScale[2]);
										}

										// ===================================================
										// STEP 5: Mark transform dirty & sync physics
										// ===================================================
										auto transformSystem = ecs->getSystem<Transform::System>();
										if (transformSystem) {
											transformSystem->markDirty(selectedEntity, ecs->getRegistry(currentRegistryID));
										}

										// Physics sync
										SyncBodyToTransform(selectedEntity, ecs->getRegistry(currentRegistryID), ecs.get(), localTransform, /*dragging=*/true);

										auto rbOpt = ecs->getEntityComponent<Physics::RigidBody3D>(selectedEntity, currentRegistryID);
										if (rbOpt.has_value()) {
											auto& rb = rbOpt.value().get();
											auto physics_system = ecs->getSystem<Physics::System>();

											if (physics_system) {
												JPH::BodyInterface& body_interface = physics_system->GetPhysicsSystem()->GetBodyInterface();

												// Disable physics temporarily during manipulation
												body_interface.DeactivateBody(rb.bodyID);
												body_interface.SetMotionType(rb.bodyID, JPH::EMotionType::Kinematic, JPH::EActivation::DontActivate);

												if (m_GizmoOperation == ImGuizmo::TRANSLATE) {
													JPH::RVec3 pos(localTransform.position.x, localTransform.position.y, localTransform.position.z);
													body_interface.SetPosition(rb.bodyID, pos, JPH::EActivation::DontActivate);
												}
												else if (m_GizmoOperation == ImGuizmo::ROTATE) {
													JPH::Quat rot(localTransform.rotation.x, localTransform.rotation.y, localTransform.rotation.z, localTransform.rotation.w);
													body_interface.SetRotation(rb.bodyID, rot, JPH::EActivation::DontActivate);
												}
											}
										}
									}

									// Just released - reactivate physics
									if (!isCurrentlyUsing && wasUsing) {
										// Final decompose (same logic as above)
										glm::mat4 parentWorldMatrix = glm::mat4(1.0f);

										auto hierarchyOpt = ecs->getEntityComponent<Entity::Hierarchy>(selectedEntity, currentRegistryID);
										if (hierarchyOpt.has_value()) {
											const Entity::Hierarchy& hierarchy = hierarchyOpt.value().get();
											if (hierarchy.parentGUID.IsValid()) {
												entt::entity parentEntity = ecs->resolveGUID(hierarchy.parentGUID, currentRegistryID);
												if (parentEntity != entt::null) {
													auto parentWorldOpt = ecs->getEntityComponent<WorldTransform>(parentEntity, currentRegistryID);
													if (parentWorldOpt.has_value()) {
														parentWorldMatrix = parentWorldOpt.value().get().matrix;
													}
												}
											}
										}

										glm::mat4 newLocalMatrix = glm::inverse(parentWorldMatrix) * modelMatrix;

										float localTranslation[3], localRotation[3], localScale[3];
										ImGuizmo::DecomposeMatrixToComponents(
											glm::value_ptr(newLocalMatrix),
											localTranslation,
											localRotation,
											localScale
										);

										if (m_GizmoOperation == ImGuizmo::TRANSLATE) {
											localTransform.position = glm::vec3(localTranslation[0], localTranslation[1], localTranslation[2]);
										}
										else if (m_GizmoOperation == ImGuizmo::ROTATE) {
											localTransform.rotation = glm::quat(glm::radians(glm::vec3(localRotation[0], localRotation[1], localRotation[2])));
										}
										else if (m_GizmoOperation == ImGuizmo::SCALE) {
											localTransform.scale = glm::vec3(localScale[0], localScale[1], localScale[2]);
										}

										// Mark dirty
										auto transformSystem = ecs->getSystem<Transform::System>();
										if (transformSystem) {
											transformSystem->markDirty(selectedEntity, ecs->getRegistry(currentRegistryID));
										}

										// Reactivate physics
										auto rbOpt = ecs->getEntityComponent<Physics::RigidBody3D>(selectedEntity, currentRegistryID);
										if (rbOpt.has_value()) {
											auto& rb = rbOpt.value().get();
											auto physics_system = ecs->getSystem<Physics::System>();
											if (physics_system) {
												JPH::BodyInterface& body_interface = physics_system->GetPhysicsSystem()->GetBodyInterface();
												body_interface.SetMotionType(rb.bodyID, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
												body_interface.ActivateBody(rb.bodyID);
											}
										}
									}

									wasUsing = isCurrentlyUsing;
								}
							}
						}
					}

					// ========================================
					// === ViewManipulate Camera Cube ===
					// ========================================
					auto camera = scene->GetActiveCamera();
					if (camera) {
						glm::mat4 viewMatrix = camera->view();

						float cubeSize = 128.0f;
						ImVec2 cubePosition(viewportPos.x + size.x - cubeSize - 10.0f, viewportPos.y + 10.0f);

						// Calculate distance from camera to look-at point (origin)
						glm::vec3 lookAtTarget = glm::vec3(0.0f);
						float cameraDistance = glm::length(camera->pos - lookAtTarget);

						// Set these before ViewManipulate
						ImGuizmo::SetDrawlist();
						ImGuizmo::SetRect(viewportPos.x, viewportPos.y, size.x, size.y);

						ImGuizmo::ViewManipulate(
							glm::value_ptr(viewMatrix),
							cameraDistance,
							cubePosition,
							ImVec2(cubeSize, cubeSize),
							0x10101010
						);

						// Check if view is being manipulated and not using imguizmo
						if (ImGuizmo::IsUsingViewManipulate() && !ImGuizmo::IsUsing() && isUsingGizmo) {
							// Extract camera vectors from the modified view matrix
							glm::mat4 inverseView = glm::inverse(viewMatrix);

							// Get the camera position from inverse view matrix
							glm::vec3 newPosition = glm::vec3(inverseView[3]);

							// Get camera basis vectors from inverse view matrix
							glm::vec3 right = glm::vec3(inverseView[0]);
							glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
							glm::vec3 forward = -glm::vec3(inverseView[2]); // Negative because OpenGL looks down -Z

							// Update camera
							camera->pos = newPosition;
							camera->forward = glm::normalize(forward);
							camera->up = glm::normalize(up);
							camera->right = glm::normalize(right);
						}
					}

					// ========================================
					 // === Mouse Picking - AFTER GIZMO ===
					 // ========================================

					// Track if we were using the gizmo in the previous frame
					static bool wasUsingGizmo = false;
					// REMOVE THIS LINE: bool isUsingGizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
					// Reuse the variable declared above

					if (contentHovered) {
						// Only pick on click if gizmo is NOT being used
						bool shouldPick = false;

						// Case 1: Direct click (no drag)
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isUsingGizmo) {
							shouldPick = true;
						}

						// Case 2: Mouse released after dragging (but NOT on gizmo)
						if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)
							&& !isUsingGizmo
							&& !wasUsingGizmo
							&& !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)) {
							shouldPick = true;
						}

						if (shouldPick) {
							ImVec2 mousePos = ImGui::GetMousePos();
							ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);
							performMousePicking(localMousePos, size);
						}
					}

					wasUsingGizmo = isUsingGizmo;
		
                }
                ImGui::End();
            }
        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
