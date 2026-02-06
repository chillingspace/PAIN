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

#include "Systems/Collision/sBVHSystem.h"
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

				// [DEBUG] Ensure services exist
				if (!camera || !ecs) {
					PN_CORE_WARN("[ViewportPanel] Cannot raycast: Camera or ECS missing");
					return glm::vec3(0.0f);
				}

				// Get camera matrices
				glm::mat4 viewMatrix = camera->view();
				glm::mat4 projMatrix = camera->projection();

				// Get ray
				glm::vec3 rayOrigin = getCameraPosition(viewMatrix);
				glm::vec3 rayDirection = screenToWorldRay(localMousePos, viewportSize, viewMatrix, projMatrix);

				//PN_CORE_TRACE("[Raycast] Origin: ({}, {}, {}) Dir: ({}, {}, {})",
				//	rayOrigin.x, rayOrigin.y, rayOrigin.z,
				//	rayDirection.x, rayDirection.y, rayDirection.z);

				// ========================================
				// Try Find intersection via BVH
				// ========================================
				auto bvhSystem = services->get<PAIN::sBVHSystem>();
				auto& registry = ecs->getRegistry(currentRegistryID);

				if (bvhSystem) {
					//call raycast
					float maxDist = 1000.0f;
					auto hit = bvhSystem->raycast(rayOrigin, rayDirection, maxDist, registry, -1);

					if (hit.has_value()) {
						// [DEBUG] Log hit
						PN_CORE_INFO("[Raycast] Hit Entity {} at Distance: {}", (uint32_t)hit->entity, hit->distance);


						return hit->point;
					}
				}
				else {
					PN_CORE_WARN("[ViewportPanel] BVH System missing!");
				}

				// ========================================
				// ELSE IF: Try No hit - project onto ground plane (Y=0)
				// ========================================
				glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
				glm::vec3 planePoint(0.0f, 0.0f, 0.0f);

				float denom = glm::dot(rayDirection, planeNormal);

				if (abs(denom) > 0.0001f) {
					float t = glm::dot((planePoint - rayOrigin), planeNormal) / denom;
					if (t >= 0.0f) {
						// [DEBUG] Log plane hit
						// PN_CORE_TRACE("[Raycast] Hit Ground Plane at distance {}", t);
						return rayOrigin + (rayDirection * t);
					}
				}

				// ========================================
				// ELSE: Fixed distance in front of camera
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


			entt::entity ViewportPanel::findEntityAtMousePos(ImVec2 localMousePos, ImVec2 viewportSize) {
				auto scene = services->get<Scene::SceneManager>();
				auto ecs = services->get<ECS::Controller>();
				auto bvhSystem = ecs->getSystem<sBVHSystem>();

				if (!bvhSystem) {
					PN_CORE_WARN("Cannot pick entity: Missing required services!");
					return entt::null;
				}

				auto camera = scene->GetActiveCamera();
				if (!camera) return entt::null;

				// Calculate Ray 
				glm::mat4 viewMatrix = camera->view();
				glm::mat4 projMatrix = camera->projection();

				glm::vec3 rayOrigin = getCameraPosition(viewMatrix);
				glm::vec3 rayDirection = screenToWorldRay(localMousePos, viewportSize, viewMatrix, projMatrix);

				 //PN_CORE_TRACE("Pick Ray - Origin: ({},{},{}) Dir: ({},{},{})", 
				 //   rayOrigin.x, rayOrigin.y, rayOrigin.z,
				 //   rayDirection.x, rayDirection.y, rayDirection.z);

				// Perform Raycast via BVH
				float maxDist = 1000.0f;
				auto& registry = ecs->getRegistry(currentRegistryID);
				int mask = scene->getPickingMask(); 
				auto hitResult = bvhSystem->raycast(rayOrigin, rayDirection, maxDist, registry, mask);

				if (hitResult.has_value()) {
					//PN_CORE_INFO("Raycast Hit Entity: {}", (uint32_t)hitResult->entity);
					return hitResult->entity;
				}

				PN_CORE_TRACE("Raycast Missed");
				return entt::null;
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
						if (ImGui::Button(!scene->isPlaying() ? "Play Scene" : "Stop Scene")) {
							!scene->isPlaying() ? scene->onPlay() : scene->onStop();
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
							cameraController->E_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_E);
							cameraController->Q_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_Q);

							cameraController->LCTRL_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
							cameraController->LSHIFT_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftShift);

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
							cameraController->E_KEYDOWN = false;
							cameraController->Q_KEYDOWN = false;
							cameraController->LCTRL_KEYDOWN = false;
							cameraController->LSHIFT_KEYDOWN = false;
							cameraController->mouseButtonDown = false;
							cameraController->xOffset = 0.0f;
							cameraController->yOffset = 0.0f;
						}

						// Zoom
						if (contentHovered && !gizmoActive && io.MouseWheel != 0.0f) {
							float mouseWheel = io.MouseWheel;
							float zoomSpeed = 0.1f;
							auto activeCamera = scene->GetActiveCamera();

							if (activeCamera) {

								if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
									zoomSpeed *= 2.0f;
								}
								else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
									zoomSpeed /= 2.0f;
								}

								glm::vec3 offset = activeCamera->forward * activeCamera->speed * zoomSpeed * mouseWheel;
								activeCamera->pos += offset;

							}
						}

						// Focus on entity (skip if typing in text field)
						if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F)) {

							entt::entity selectedEntity = m_EntityPanel->getSelectedEntity();

							if (selectedEntity != entt::null) {
								auto localTransformOpt = ecs->getEntityComponent<LocalTransform>(selectedEntity, currentRegistryID);
								auto worldTransformOpt = ecs->getEntityComponent<WorldTransform>(selectedEntity, currentRegistryID);

								if (localTransformOpt.has_value() && worldTransformOpt.has_value()) {
									LocalTransform& localTransform = localTransformOpt.value().get();
									WorldTransform& worldTransform = worldTransformOpt.value().get();
									auto camera = scene->GetActiveCamera();

									if (camera) {

										glm::vec3 targetPos = glm::vec3(worldTransform.matrix[3]);
										camera->pos = targetPos - camera->forward;
									}
								}
								PN_CORE_INFO("FOCUSED ON ENTITY");
							}
							else {
								PN_CORE_INFO("NO ENTITY TO FOCUS ON");
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

									// Hotkeys (skip if typing in text field)
								if (!ImGui::GetIO().WantTextInput) {
									if (ImGui::IsKeyPressed(ImGuiKey_T))
										m_GizmoOperation = ImGuizmo::TRANSLATE;
									if (ImGui::IsKeyPressed(ImGuiKey_R))
										m_GizmoOperation = ImGuizmo::ROTATE;
									if (ImGui::IsKeyPressed(ImGuiKey_Y))
										m_GizmoOperation = ImGuizmo::SCALE;
								}

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
									static LocalTransform startTransform; 

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

										LocalTransform endTransform = localTransform;

										// Only create command if something actually changed

										// Check if actually changed
										if ((cachedPosition != endTransform.position ||
											cachedRotation != endTransform.rotation ||
											cachedScale != endTransform.scale) && command_manager)
										{
											auto guidOpt = ecs->getEntityComponent<Entity::GUID>(selectedEntity, currentRegistryID);

											if (guidOpt.has_value()) {

												// Capture Start/End data by value for the lambda
												Assets::GUID targetGUID = guidOpt.value().get().guid;
												LocalTransform startT = { cachedPosition, cachedRotation, cachedScale };
												LocalTransform endT = endTransform;

												command_manager->executeAction(Action{
													// DO Action (Redo/Apply) using imguizmo
													[this, targetGUID, endT]() {
														auto ecs = services->get<ECS::Controller>();
														entt::entity e = ecs->resolveGUID(targetGUID, currentRegistryID);

														if (ecs->checkEntity(e, currentRegistryID)) {
															auto& registry = ecs->getRegistry(currentRegistryID);

															// Update Transform
															registry.get_or_emplace<LocalTransform>(e) = endT;

															// Mark Dirty
															auto transformSystem = ecs->getSystem<Transform::System>();
															if (transformSystem) {
																transformSystem->markDirty(e, registry);
															}

															// Sync Physics
															SyncBodyToTransform(e, registry, ecs.get(), endT, false);
														}
													},

													// UNDO transform using imguizmo
													[this, targetGUID, startT]() {
														auto ecs = services->get<ECS::Controller>();
														entt::entity e = ecs->resolveGUID(targetGUID, currentRegistryID);

														if (ecs->checkEntity(e, currentRegistryID)) {
															auto& registry = ecs->getRegistry(currentRegistryID);

															// Revert Transform
															registry.get_or_emplace<LocalTransform>(e) = startT;

															// Mark Dirty
															auto transformSystem = ecs->getSystem<Transform::System>();
															if (transformSystem) {
																transformSystem->markDirty(e, registry);
															}

															// Sync Physics
															SyncBodyToTransform(e, registry, ecs.get(), startT, false);
														}
													},

													// 3. Description
													"Transform Entity: " + targetGUID.ToString()
													});
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

						entt::entity selectedEntity = m_EntityPanel->getSelectedEntity();
						float defaultDistance = 10.0f; 
						glm::vec3 lookAtTarget = camera->pos + (camera->forward * defaultDistance);

						if (selectedEntity != entt::null) {
							auto worldTransformOpt = ecs->getEntityComponent<WorldTransform>(selectedEntity, currentRegistryID);

							if (worldTransformOpt.has_value()) {
								WorldTransform& worldTransform = worldTransformOpt.value().get();

							    lookAtTarget = glm::vec3(worldTransform.matrix[3]);

							}
						}
						// Calculate distance from camera to look-at point (origin)
						
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
					// === Camera Collision Visualization ===
					// ========================================
					if (camera->showCollisionGizmo && camera->collisionEnabled) {
						// Project camera position to screen space for visualization
						glm::mat4 viewProj = camera->projection() * camera->view();
						
						// Draw wireframe sphere/capsule around camera
						glm::vec3 camPos = camera->pos;
						float radius = camera->collisionRadius;
						
						// Simple visualization: draw circles in different planes
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImU32 color = IM_COL32(0, 255, 0, 200); // Green for collision
						
						// Project 3D points to 2D screen space
						auto projectToScreen = [&](const glm::vec3& worldPos) -> ImVec2 {
							glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
							if (clip.w > 0.001f) {
								glm::vec3 ndc = glm::vec3(clip) / clip.w;
								return ImVec2(
									viewportPos.x + (ndc.x * 0.5f + 0.5f) * size.x,
									viewportPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * size.y
								);
							}
							return ImVec2(-1000, -1000); // Off-screen
						};
						
						// Draw circle in XY plane
						const int segments = 32;
						for (int i = 0; i < segments; ++i) {
							float angle1 = (float(i) / segments) * 2.0f * 3.14159f;
							float angle2 = (float(i + 1) / segments) * 2.0f * 3.14159f;
							
							glm::vec3 p1 = camPos + glm::vec3(cos(angle1) * radius, sin(angle1) * radius, 0);
							glm::vec3 p2 = camPos + glm::vec3(cos(angle2) * radius, sin(angle2) * radius, 0);
							
							ImVec2 screen1 = projectToScreen(p1);
							ImVec2 screen2 = projectToScreen(p2);
							
							if (screen1.x > -100 && screen2.x > -100) {
								drawList->AddLine(screen1, screen2, color, 2.0f);
							}
						}
						
						// Draw circle in XZ plane
						for (int i = 0; i < segments; ++i) {
							float angle1 = (float(i) / segments) * 2.0f * 3.14159f;
							float angle2 = (float(i + 1) / segments) * 2.0f * 3.14159f;
							
							glm::vec3 p1 = camPos + glm::vec3(cos(angle1) * radius, 0, sin(angle1) * radius);
							glm::vec3 p2 = camPos + glm::vec3(cos(angle2) * radius, 0, sin(angle2) * radius);
							
							ImVec2 screen1 = projectToScreen(p1);
							ImVec2 screen2 = projectToScreen(p2);
							
							if (screen1.x > -100 && screen2.x > -100) {
								drawList->AddLine(screen1, screen2, color, 2.0f);
							}
						}
						
						// Draw circle in YZ plane
						for (int i = 0; i < segments; ++i) {
							float angle1 = (float(i) / segments) * 2.0f * 3.14159f;
							float angle2 = (float(i + 1) / segments) * 2.0f * 3.14159f;
							
							glm::vec3 p1 = camPos + glm::vec3(0, cos(angle1) * radius, sin(angle1) * radius);
							glm::vec3 p2 = camPos + glm::vec3(0, cos(angle2) * radius, sin(angle2) * radius);
							
							ImVec2 screen1 = projectToScreen(p1);
							ImVec2 screen2 = projectToScreen(p2);
							
							if (screen1.x > -100 && screen2.x > -100) {
								drawList->AddLine(screen1, screen2, color, 2.0f);
							}
						}
						
						// For capsule, draw lines showing height
						if (camera->useCapsuleCollision) {
							float halfHeight = (camera->capsuleHeight - 2.0f * radius) * 0.5f;
							if (halfHeight > 0) {
								glm::vec3 top = camPos + glm::vec3(0, halfHeight, 0);
								glm::vec3 bottom = camPos - glm::vec3(0, halfHeight, 0);
								
								// Draw vertical lines to show capsule extent
								for (int i = 0; i < 8; ++i) {
									float angle = (float(i) / 8) * 2.0f * 3.14159f;
									glm::vec3 offset(cos(angle) * radius, 0, sin(angle) * radius);
									
									ImVec2 screenTop = projectToScreen(top + offset);
									ImVec2 screenBottom = projectToScreen(bottom + offset);
									
									if (screenTop.x > -100 && screenBottom.x > -100) {
										drawList->AddLine(screenTop, screenBottom, color, 1.0f);
									}
								}
							}
						}
					}

					// ========================================
					 // === Mouse Picking - AFTER GIZMO ===
					 // ========================================


					bool isHoveringViewCube = false;
					{
						ImVec2 mousePos = ImGui::GetMousePos();
						// Use the exact same coordinates you used for ViewManipulate
						float cubeSize = 128.0f;
						ImVec2 cubePos(viewportPos.x + size.x - cubeSize - 10.0f, viewportPos.y + 10.0f);

						if (mousePos.x >= cubePos.x && mousePos.x <= cubePos.x + cubeSize &&
							mousePos.y >= cubePos.y && mousePos.y <= cubePos.y + cubeSize) {
							isHoveringViewCube = true;
						}
					}


					bool isAnyGizmoActive = isUsingGizmo || ImGuizmo::IsUsingViewManipulate() || isHoveringViewCube;

					static bool wasAnyGizmoActive = false;

					if (contentHovered) {
						bool shouldPick = false;

						// Case 1: Direct click (no drag)
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isAnyGizmoActive) {
							shouldPick = true;
						}

						// Case 2: Mouse released after dragging (but NOT on gizmo)
						// We add '!wasAnyGizmoActive' to ensure we didn't just finish using the cube
						if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)
							&& !isAnyGizmoActive
							&& !wasAnyGizmoActive
							&& !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)) {
							shouldPick = true;
						}

						if (shouldPick) {
							ImVec2 mousePos = ImGui::GetMousePos();
							ImVec2 localMousePos = ImVec2(mousePos.x - viewportPos.x, mousePos.y - viewportPos.y);
							performMousePicking(localMousePos, size);
						}
					}

					// Update state for next frame
					wasAnyGizmoActive = isAnyGizmoActive;
		
                }
                ImGui::End();
            }
        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
