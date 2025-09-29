#pragma once
#include "pch.h"


#include "../CoreSystems/Renderer/Mesh.h"
#include "../CoreSystems/Renderer/RendererLayer.h"

namespace PAIN {

	class Scene {
	public:
		Scene() = default;
		~Scene() = default;

		void Init();
		void OnUpdate();

		//Entity CreateEntity(const std::string name);
		//void DestroyEntity(Entity entity);


	private:
		std::vector<std::unique_ptr<Mesh>> m_Objects;
		//std::unique_ptr<ECS::Controller> ecs;
	};
}

