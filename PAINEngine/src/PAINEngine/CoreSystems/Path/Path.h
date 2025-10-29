#pragma once

#include "AssetData.h"

#ifndef PATH_HPP
#define PATH_HPP

namespace PAIN {
	namespace Path {

		//Path class
		class Path {
		protected:

			//Virtual paths
			std::unordered_map<std::string, std::string> virtual_paths;

			//Game folder relative to asset
			std::string relative_game_folder = Assets::game_assets_folder.string();
			std::string relative_engine_folder = Assets::engine_assets_folder.string();

			//Parsing virtual functions
			std::pair<std::string, std::string> parseVirtualPath(const std::string& virtualPath) const {
				auto separatorPos = virtualPath.find(getVirtualSymbol());
				if (separatorPos == std::string::npos) {
					throw std::runtime_error("Invalid virtual path format: " + virtualPath);
				}

				std::string alias = virtualPath.substr(0, separatorPos);
				std::string relativePath = virtualPath.substr(separatorPos + 3);

				// Remove leading slashes
				size_t firstValidChar = relativePath.find_first_not_of("/\\");
				if (firstValidChar != std::string::npos) {
					relativePath = relativePath.substr(firstValidChar);
				}
				else {
					relativePath.clear();
				}

				return { alias, relativePath };
			}

		public:
			Path() = default;
			virtual ~Path() = default;

			std::string getVirtualSymbol() const { return "://"; }

			std::string aliasCombineRelative(std::string const& alias, std::string const& relative) const {
				return alias + getVirtualSymbol() + relative;
			}

			virtual void registerVirtualPath(const std::string& alias, const std::string& path, bool create_new = false) = 0;
			virtual void updateVirtualPath(const std::string& alias, const std::string& path) = 0;
			virtual std::string resolvePath(const std::string& virtualPath) const = 0;
			virtual std::string resolvePath(const std::string& alias, std::string const& relative) const = 0;
			virtual std::vector<std::string> listFiles(const std::string& virtualPath, const std::string& filter = "", const std::string& extension = "") const = 0;
			virtual std::vector<std::string> listDirectories(const std::string& virtualPath, const std::string& filter = "") const = 0;
			virtual bool pathExists(const std::string& virtualPath) const = 0;
			virtual bool createDirectory(const std::string& virtualPath) const = 0;

			//Read & write files
			virtual std::vector<uint8_t> readFile(const std::string& virtualPath) const = 0;
			virtual bool writeFile(const std::string& virtualPath, const std::vector<uint8_t>& data) const = 0;

			//Overloads provided for json
			virtual nlohmann::json readJsonFile(const std::string& virtualPath) const = 0;
			virtual bool writeJsonFile(const std::string& virtualPath, const nlohmann::json& data) const = 0;

			std::string getAlias(const std::string& virtualPath) const {
				auto [alias, relativePath] = parseVirtualPath(virtualPath);
				return alias;
			}

			void logVirtualPaths() const {
				PN_CORE_INFO("=== Windows Virtual Paths ===");
				for (const auto& [alias, path] : virtual_paths) {
					PN_CORE_INFO("{} -> {}", alias, path);
				}
			}

			//Gettors
			std::unordered_map<std::string, std::string> getAllVirtualPaths() const { return virtual_paths; }

			//Create path service
			static Path* create(void* app);
		};

	}
}

#endif
