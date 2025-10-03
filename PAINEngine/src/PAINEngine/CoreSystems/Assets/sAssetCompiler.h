/*****************************************************************//**
 * \file   sAssetCompiler.h
 * \brief  Declaration of asset compiler service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifdef PN_PLATFORM_WINDOWS
#ifndef S_ASSET_COMPILER_H
#define S_ASSET_COMPILER_H

#include "Applications/AppSystem.h"
//#include <assimp/include/assimp/Importer.hpp>
//#include <assimp/include/assimp/scene.h>
//#include <assimp/include/assimp/postprocess.h>

//#ifdef PN_PLATFORM_ANDROID
//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>
//#endif

namespace PAIN {
	namespace Compiler {

		enum class COMPILE_ASSET_TYPE {
			Texture,
			Object,
			Shader,
			None
		};

		static json readDescFile(const std::string& input_path)
		{

			// Read descriptor JSON
			std::ifstream input_file(input_path);
			if (!input_file.is_open()) {
				PN_CORE_WARN("Cannot open descriptor: {}", input_path);
				return json{};
			}

			json json_file;
			try {
				input_file >> json_file;
			}
			catch (const nlohmann::json::exception& e) {
				PN_CORE_WARN("Failed to parse JSON: {}", e.what());
				return json{};
			}

			return json_file;
		}


		class IAssetCompiler{
		public:
			virtual ~IAssetCompiler() = default;
			virtual void compile(const std::string& desc_path) = 0;
		};

		class TextureCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class ShaderCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class ObjectMeshCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class Service : public AppSystem {
		public:
			~Service() = default;

			void scanAssetDirectory(std::string const& virtual_path, bool b_diretory_tree);

			void compileAsset(COMPILE_ASSET_TYPE type, const std::string& desc_file_path);

			// Helper function
			std::string getIDFromPath(std::string const& path);
			std::string typeToString(COMPILE_ASSET_TYPE type) const;
			COMPILE_ASSET_TYPE getAssetType(std::filesystem::path const& path) const;

			void processAssetFile(std::filesystem::path const& file_path);
			void addValidExtensions(std::string const& ext);

			//Thread safe insertion for file event queue
			void pushFileEvent(std::function<void()> callback);

			void onAttach() override;

			void onFixedUpdate(AppTiming timing) override;
			void onUpdate(AppTiming timing) override;
			void onDetach() override;

			void onEvent(Event::Event& e) override;

#define PN_PATH_SERVICE  services->get<Path::Service>()

		private:
			std::unordered_map<COMPILE_ASSET_TYPE, std::unique_ptr<IAssetCompiler>> compilers;

			//List of valid extension
			std::set<std::string> valid_extensions;

			//File Watching Queue
			std::queue<std::function<void()>> file_event_queue;

			//Mutex for thread safety
			std::mutex file_event_mutex;
		};

	}
}


#endif
#endif