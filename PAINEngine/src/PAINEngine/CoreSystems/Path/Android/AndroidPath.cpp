#include "pch.h"
#include "AndroidPath.h"

#ifdef PN_PLATFORM_ANDROID

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

namespace PAIN {
	namespace Path {

		//Create path service
		Path* Path::create(void* app) {
			return new AndroidPath(app);
		}

		std::string AndroidPath::normalizePath(const std::string& path) const {
			std::string normalized = path;

			// Replace backslashes with forward slashes
			std::replace(normalized.begin(), normalized.end(), '\\', '/');

			// Remove double slashes
			size_t pos = 0;
			while ((pos = normalized.find("//", pos)) != std::string::npos) {
				normalized.replace(pos, 2, "/");
			}

			// Remove trailing slash (except for root)
			if (normalized.length() > 1 && normalized.back() == '/') {
				normalized.pop_back();
			}

			return normalized;
		}

		bool AndroidPath::isValidPath(const std::string& path) const {
			struct stat buffer;
			return (stat(path.c_str(), &buffer) == 0);
		}

		bool AndroidPath::createDirectoryRecursive(const std::string& path) const {
			// Create directories recursively
			std::string currentPath = "";
			std::istringstream stream(path);
			std::string segment;

			while (std::getline(stream, segment, '/')) {
				if (segment.empty()) continue;

				if (currentPath.empty() && path[0] == '/') {
					currentPath = "/";
				}

				currentPath += segment + "/";

				if (mkdir(currentPath.c_str(), 0755) != 0 && errno != EEXIST) {
					PN_CORE_WARN("Failed to create directory: {}", currentPath);
					return false;
				}
			}
			return true;
		}

		void AndroidPath::initWithNativeApp(android_app* app) {
			m_app = app;

			if (!m_app || !m_app->activity) {
				throw std::runtime_error("Invalid android_app pointer");
			}

			ANativeActivity* activity = m_app->activity;

			// Use the direct path members - NO JNI REQUIRED!
			if (activity->internalDataPath) {
				internal_path = normalizePath(activity->internalDataPath);
			}

			if (activity->externalDataPath) {
				external_path = normalizePath(activity->externalDataPath);
			}

			// Generate package name from internal path
			// Internal path format: /data/data/com.package.name/files
			if (!internal_path.empty()) {
				std::string::size_type start = internal_path.find("/data/data/") + 11;
				std::string::size_type end = internal_path.find("/files");
				if (start != std::string::npos && end != std::string::npos && end > start) {
					package_name = internal_path.substr(start, end - start);
				}
			}

			// Convert to paths
			cache_path = internal_path + "/cache";
			assets_path = "file:///android_asset";

			// Log all paths
			PN_CORE_INFO("Internal Path: {}", internal_path);
			PN_CORE_INFO("External Path: {}", external_path);
			PN_CORE_INFO("Cache Path: {}", cache_path);
			PN_CORE_INFO("Assets Path: {}", assets_path);

			// Initialize virtual paths
			init();
		}

		void AndroidPath::init() {
			// Json config
			nlohmann::json config;
			try {
				app_name = "PAINEngine";
			}
			catch (const nlohmann::json::exception& e) {
				app_name = "PAINEngine";
			}

			//Special case: manually register asset path
			virtual_paths["assets"] = assets_path;

			// Register default virtual paths
			registerVirtualPath("internal", internal_path, true);
			registerVirtualPath("external", external_path, true);
			registerVirtualPath("cache", cache_path, true);
			registerVirtualPath("temp", internal_path + "/temp", true);
		}

		void AndroidPath::destroy() {
			virtual_paths.clear();
		}

		void AndroidPath::registerVirtualPath(const std::string& alias, const std::string& path, bool create_new) {

			// Check if path actually exists for non-asset paths
			if (!isValidPath(path)) {
				// Create new path
				if (create_new) {
					createDirectoryRecursive(path);
				}
				else {
					PN_CORE_WARN("Path: {} does not exist. Invalid registering of path.", path);
					return;
				}
			}

			// Check if alias already exists
			if (virtual_paths.find(alias) != virtual_paths.end()) {
				PN_CORE_WARN("Alias: {} already exists. Invalid registering of path.", alias);
				return;
			}

			// Register path...
			virtual_paths[alias] = normalizePath(path);
			PN_CORE_INFO("Registered Virtual Path: {} -> {}", alias, path);
		}

		void AndroidPath::updateVirtualPath(const std::string& alias, const std::string& path) {

			// Check if path actually exists for non-asset paths
			if (!isValidPath(path)) {
				PN_CORE_WARN("Path: {} does not exist. Invalid registering of path.", path);
				return;
			}

			auto it = virtual_paths.find(alias);
			if (it == virtual_paths.end()) {
				PN_CORE_WARN("Alias: {} does not exists. Invalid registering of path.", alias);
				return;
			}

			// Update path
			it->second = normalizePath(path);
			PN_CORE_INFO("Updated: {} -> {}", alias, path);
		}

		std::string AndroidPath::resolvePath(const std::string& virtualPath) const {

			auto [alias, relativePath] = parseVirtualPath(virtualPath);

			auto it = virtual_paths.find(alias);
			if (it == virtual_paths.end()) {
				throw std::runtime_error("Unknown virtual path alias: " + alias);
			}

			if (relativePath.empty()) {
				return it->second;
			}

			std::string fullPath = it->second + "/" + relativePath;

			if (isAssetPath(virtualPath)) {
				return fullPath;
			}
			else {
				return normalizePath(fullPath);
			}
		}

		std::vector<std::string> AndroidPath::listFiles(const std::string& virtualPath, const std::string& filter, const std::string& extension) const {
			// Assets can't be listed through filesystem
			if (isAssetPath(virtualPath)) {
				PN_CORE_WARN("Asset file listing not supported without AAssetManager");
				return {};
			}

			// Get actual path
			auto actualPath = resolvePath(virtualPath);

			// Check if path exists
			if (!isValidPath(actualPath)) {
				throw std::runtime_error("Path does not exist or is not a directory: " + actualPath);
			}

			// Store list of files in a virtual path
			std::vector<std::string> files;

			DIR* dir = opendir(actualPath.c_str());
			if (!dir) {
				PN_CORE_WARN("Cannot open directory: %s", actualPath.c_str());
				return files;
			}

			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr) {
				if (entry->d_type == DT_REG) {
					std::string filename = entry->d_name;
					bool matchesFilter = filter.empty() || filename.find(filter) != std::string::npos;

					// Android NDK compatible ends_with implementation
					bool matchesExt = extension.empty() ||
						(filename.size() >= extension.size() &&
							filename.compare(filename.size() - extension.size(), extension.size(), extension) == 0);

					if (matchesFilter && matchesExt) {
						files.push_back(actualPath + "/" + filename);
					}
				}
			}
			closedir(dir);

			// Return all files
			return files;
		}

		std::vector<std::string> AndroidPath::listDirectories(const std::string& virtualPath, const std::string& filter) const {
			// Assets can't be listed through filesystem
			if (isAssetPath(virtualPath)) {
				PN_CORE_WARN("Asset file listing not supported without AAssetManager");
				return {};
			}

			// Get actual path
			auto actualPath = resolvePath(virtualPath);

			// Check if path exists
			if (!isValidPath(actualPath)) {
				throw std::runtime_error("Path does not exist or is not a directory: " + actualPath);
			}

			// Store list of directories in a virtual path
			std::vector<std::string> directories;

			DIR* dir = opendir(actualPath.c_str());
			if (!dir) {
				PN_CORE_WARN("Cannot open directory: %s", actualPath.c_str());
				return directories;
			}

			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr) {
				if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
					std::string dirname = entry->d_name;
					bool matchesFilter = filter.empty() || dirname.find(filter) != std::string::npos;

					if (matchesFilter) {
						directories.push_back(actualPath + "/" + dirname);
					}
				}
			}
			closedir(dir);

			// Return all directories
			return directories;
		}

		bool AndroidPath::pathExists(const std::string& virtualPath) const {
			// Handle asset paths
			if (isAssetPath(virtualPath)) {
				// Assets always "exist" in the APK, but we can't verify without AAssetManager
				return true;  // Assume assets exist
			}

			try {
				std::string actualPath = resolvePath(virtualPath);
				return isValidPath(actualPath);
			}
			catch (const std::exception&) {
				return false;
			}
		}

		bool AndroidPath::createDirectory(const std::string& virtualPath) const {
			// Can't create asset directories
			if (isAssetPath(virtualPath)) {
				PN_CORE_WARN("Cannot create directories in assets");
				return false;
			}

			try {
				std::string actualPath = resolvePath(virtualPath);
				return createDirectoryRecursive(actualPath);
			}
			catch (const std::exception&) {
				return false;
			}
		}

		bool AndroidPath::isAssetPath(const std::string& virtualPath) const {
			return parseVirtualPath(virtualPath).first == "assets";
		}
	}
}

#endif
