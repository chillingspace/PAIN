#pragma once

#include "AssetData.h"

#ifndef PATH_HPP
#define PATH_HPP

namespace PAIN {
	namespace Path {

		//Different file modes
		enum class FileMode {
			Read,
			Write,
			ReadWrite
		};

		//Custom file streaming
		class IFileStream {
		public:
			virtual ~IFileStream() = default;
			virtual size_t read(void* buffer, size_t size) = 0;
			virtual size_t write(const void* buffer, size_t size) = 0;
			virtual void flush() = 0;
			virtual void seek(size_t pos) = 0;
			virtual size_t tell() = 0;
			virtual bool eof() const = 0;
			virtual size_t size() const = 0;
			virtual bool good() const = 0;
		};

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

			//Create custom file stream
			virtual std::unique_ptr<IFileStream> createFileStream(const std::string& virtualPath, FileMode mode) = 0;

			std::string getAlias(const std::string& virtualPath) const {
				auto [alias, relativePath] = parseVirtualPath(virtualPath);
				return alias;
			}

			std::string getRelative(const std::string& virtualPath) const {
				auto [alias, relativePath] = parseVirtualPath(virtualPath);
				return relativePath;
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
