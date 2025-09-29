#pragma once
#include "pch.h"


namespace PAIN {
	class Scene {
	public:
		Scene();
		~Scene();

		void Init();
		void OnUpdate();

		//Entity CreateEntity(const std::string name);
		//void DestroyEntity(Entity entity);


	private:
		//std::unique_ptr<ECS::Controller> ecs;
	};
}