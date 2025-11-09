#pragma once

#ifndef PATH_HPP
#define PATH_HPP

#include "AssetData.h"
#include "Utility/Log.h"

namespace PAIN {
	namespace Path {

		//Global alias
		static std::string assets_alias = "assets";
		static std::string game_assets_alias = "game_assets";
		static std::string engine_assets_alias = "engine_assets";

#ifdef PN_PLATFORM_WINDOWS
		//Platform specific alias
		static std::string main_assets_alias = "main_assets";
		static std::string main_game_assets_alias = "main_game_assets";
		static std::string main_engine_assets_alias = "main_engine_assets";
		static std::string local_alias = "local";
		static std::string roaming_alias = "roaming";
		static std::string documents_alias = "documents";
		static std::string temp_alias = "temp";
		static std::string config_alias = "config";

		#include "FileWatch.hpp"
#endif

#ifdef PN_PLATFORM_ANDROID
		//Platform specific alias
		static std::string internal_alias = "internal";
		static std::string external_alias = "external";
		static std::string cache_alias = "cache";
		static std::string temp_alias = "temp";
#endif


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

			virtual std::string normalizePath(const std::string& path) const = 0;

			virtual std::string getVirtualParentPath(std::string const& virtual_path) const = 0;

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

#ifdef PN_PLATFORM_WINDOWS

			//Event callback
			using FileWatchEventCallback = std::function<void(std::filesystem::path const&, filewatch::Event)>;

			//Watch directory
			virtual void watchDirectory(std::string const& virtual_path, FileWatchEventCallback callback) = 0;

			//Watch directory & child directories
			virtual void watchDirectoryTree(std::string const& virtual_path, FileWatchEventCallback callback) = 0;

			//Stop watching directory
			virtual void stopWatchingDirectory(std::string const& virtual_path) = 0;

			//Stop watching directory & child directories
			virtual void stopWatchingDirectoryTree(std::string const& virtual_path) = 0;

			//Stop watching all directories
			virtual void stopWatchingAllDirectories() = 0;

			//Log watched directories
			virtual void logWatchedDirectories() const = 0;
#endif

			//Create path service
			static Path* create(void* app);
		};

	}
}

#endif
