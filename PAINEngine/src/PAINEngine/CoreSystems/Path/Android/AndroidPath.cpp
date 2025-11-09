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
			// Replace all backslashes with forward slashes (for APK/content and filesystem)
			std::replace(normalized.begin(), normalized.end(), '\\', '/');
			// Remove double slashes
			size_t pos = 0; while ((pos = normalized.find("//", pos)) != std::string::npos) normalized.replace(pos, 2, "/");
			// Remove trailing slash except for root
			if (normalized.length() > 1 && normalized.back() == '/') normalized.pop_back();
			// Remove leading slashes (optional, based on your virtual path convention)
			while (normalized.length() > 1 && normalized.front() == '/') normalized.erase(normalized.begin());
			return normalized;
		}

		std::string AndroidPath::normalizeFileIOPath(const std::string& path) const {
			std::string normalized = path;
			// Replace all backslashes with forward slashes
			std::replace(normalized.begin(), normalized.end(), '\\', '/');

			// Remove double slashes but preserve single leading/trailing slashes (important for root or absolute paths)
			size_t pos = 0;
			while ((pos = normalized.find("//", pos)) != std::string::npos) {
				normalized.replace(pos, 2, "/");
			}

			// Optionally, remove '/./' to further canonicalize (resolves "a/./b" -> "a/b")
			pos = 0;
			while ((pos = normalized.find("/./", pos)) != std::string::npos) {
				normalized.replace(pos, 3, "/");
			}
			// Optionally, collapse trailing "/." (E.g., "dir/." -> "dir")
			if (normalized.size() > 2 && normalized.compare(normalized.size() - 2, 2, "/.") == 0)
				normalized.resize(normalized.size() - 2);

			// Do NOT strip leading or trailing slashes for file I/O (preserve absolute/relative nature)
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
				internal_path = normalizeFileIOPath(activity->internalDataPath);
			}

			if (activity->externalDataPath) {
				external_path = normalizeFileIOPath(activity->externalDataPath);
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

			// Log all paths
			PN_CORE_INFO("Internal Path: {}", internal_path);
			PN_CORE_INFO("External Path: {}", external_path);
			PN_CORE_INFO("Cache Path: {}", cache_path);

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

			//Register assets path
			virtual_paths[assets_alias] = "";
			virtual_paths[game_assets_alias] = relative_game_folder;
			virtual_paths[engine_assets_alias] = relative_engine_folder;

			//Register default virtual paths
			registerVirtualPath(internal_alias, internal_path, true);
			registerVirtualPath(external_alias, external_path, true);
			registerVirtualPath(cache_alias, cache_path, true);
			registerVirtualPath(temp_alias, internal_path + "/temp", true);
		}

		void AndroidPath::destroy() {
			virtual_paths.clear();
		}

		std::string AndroidPath::getVirtualParentPath(std::string const& virtual_path) const {
			//Get alias
			auto alias = getAlias(virtual_path);

			//Get alias full path
			std::filesystem::path alias_path = normalizePath(virtual_paths.at(alias));

			//Get actual path
			std::filesystem::path actual_path = resolvePath(virtual_path);

			//Get parent path
			std::filesystem::path parent_path = normalizePath(actual_path.parent_path().string());

			//Return parent path
			if (parent_path != alias_path) {

				//Check if iteration of parent path exceeds alias
				if (parent_path.string().find(alias_path.string()) == std::string::npos) {
					return alias + getVirtualSymbol();
				}

				//Get relative path from parent path
				std::string relative_path = parent_path.string().substr(alias_path.string().size() + 1);

				//Convert parent path back to virtual path
				return alias + getVirtualSymbol() + relative_path;
			}
			else {

				//Return alias
				return alias + getVirtualSymbol();
			}
		}

		void AndroidPath::registerVirtualPath(const std::string& alias, const std::string& raw_path, bool create_new) {
			// Always normalize file system paths BEFORE any queries, existence checks, or storage.
			std::string path = isAssetPath(alias + getVirtualSymbol()) ? normalizePath(raw_path) : normalizeFileIOPath(raw_path);

			// Only perform disk/FS existence check for non-asset paths.
			if (!isAssetPath(alias + getVirtualSymbol())) {
				if (!isValidPath(path)) {
					if (create_new) {
						createDirectoryRecursive(path);
					}
					else {
						PN_CORE_WARN("Path: {} does not exist. Invalid registering of path.", path);
						return;
					}
				}
			}

			// Prevent duplicate alias registration.
			if (virtual_paths.find(alias) != virtual_paths.end()) {
				PN_CORE_WARN("Alias: {} already mesh_id. Invalid registering of path.", alias);
				return;
			}

			virtual_paths[alias] = path; // Store normalized!
			PN_CORE_INFO("Registered Virtual Path: {} -> {}", alias, path);
		}

		void AndroidPath::updateVirtualPath(const std::string& alias, const std::string& raw_path) {
			std::string path = isAssetPath(alias + getVirtualSymbol()) ? normalizePath(raw_path) : normalizeFileIOPath(raw_path);

			if (!isAssetPath(alias + getVirtualSymbol())) {
				if (!isValidPath(path)) {
					PN_CORE_WARN("Path: {} does not exist. Invalid registering of path.", path);
					return;
				}
			}

			auto it = virtual_paths.find(alias);
			if (it == virtual_paths.end()) {
				PN_CORE_WARN("Alias: {} does not mesh_id. Invalid updating of path.", alias);
				return;
			}

			it->second = path;
			PN_CORE_INFO("Updated: {} -> {}", alias, path);
		}

		std::string AndroidPath::resolvePath(const std::string& virtualPath) const {
			auto [alias, relativePath] = parseVirtualPath(virtualPath); // Normalize any input!

			auto it = virtual_paths.find(alias);
			if (it == virtual_paths.end()) {
				throw std::runtime_error("Unknown virtual path alias: " + alias);
			}

			std::string fullPath = it->second;
			if (!relativePath.empty()) {
				fullPath = fullPath.empty() ? relativePath : (fullPath + "/" + relativePath);
			}

			// Always return normalized path.
			return isAssetPath(alias + getVirtualSymbol()) ? normalizePath(fullPath) : normalizeFileIOPath(fullPath);
		}

		std::string AndroidPath::resolvePath(const std::string& alias, std::string const& relative) const {
			auto it = virtual_paths.find(alias);
			if (it == virtual_paths.end()) {
				throw std::runtime_error("Unknown virtual path alias: " + alias);
			}
			std::string fullPath = it->second;
			if (!relative.empty()) {
				fullPath = fullPath.empty() ? relative : (fullPath + "/" + relative);
			}
			return isAssetPath(alias + getVirtualSymbol()) ? normalizePath(fullPath) : normalizeFileIOPath(fullPath);
		}

		std::vector<std::string> AndroidPath::listFiles(const std::string& virtualPath, const std::string& filter, const std::string& extension) const {

			// Asset (APK) directory listing
			if (isAssetPath(virtualPath)) {
				std::vector<std::string> files;
				std::string assetDir = resolvePath(virtualPath);

				auto* mgr = m_app ? m_app->activity->assetManager : nullptr;
				if (!mgr) {
					PN_CORE_WARN("AAssetManager unavailable! Cannot list asset directory: {}", assetDir);
					return files;
				}
				AAssetDir* dir = AAssetManager_openDir(mgr, assetDir.c_str());
				if (!dir) {
					PN_CORE_WARN("Cannot open asset directory: {}", assetDir);
					return files;
				}
				const char* filename = nullptr;
				while ((filename = AAssetDir_getNextFileName(dir)) != nullptr) {
					std::string fname = filename;
					bool matchesFilter = filter.empty() || fname.find(filter) != std::string::npos;
					bool matchesExt = extension.empty() ||
						(fname.size() >= extension.size() &&
							fname.compare(fname.size() - extension.size(), extension.size(), extension) == 0);
					if (matchesFilter && matchesExt)
						files.push_back(assetDir + "/" + fname);
				}
				AAssetDir_close(dir);
				return files;
			}

			// Filesystem listing
			std::string actualPath = resolvePath(virtualPath);

			if (!isValidPath(actualPath)) {
				throw std::runtime_error("Path does not exist or is not a directory: " + actualPath);
			}
			std::vector<std::string> files;
			DIR* dir = opendir(actualPath.c_str());
			if (!dir) {
				PN_CORE_WARN("Cannot open directory: {}", actualPath);
				return files;
			}
			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr) {
				if (entry->d_type == DT_REG) {
					std::string filename = entry->d_name;
					bool matchesFilter = filter.empty() || filename.find(filter) != std::string::npos;
					bool matchesExt = extension.empty() ||
						(filename.size() >= extension.size() &&
							filename.compare(filename.size() - extension.size(), extension.size(), extension) == 0);
					if (matchesFilter && matchesExt)
						files.push_back(actualPath + "/" + filename);
				}
			}
			closedir(dir);
			return files;
		}

		std::vector<std::string> AndroidPath::listDirectories(const std::string& virtualPath, const std::string& filter) const {

			// Asset (APK) directory listing
			if (isAssetPath(virtualPath)) {
				std::vector<std::string> directories;
				std::string assetDir = resolvePath(virtualPath);

				auto* mgr = m_app ? m_app->activity->assetManager : nullptr;
				if (!mgr) {
					PN_CORE_WARN("AAssetManager unavailable! Cannot list asset directory: {}", assetDir);
					return directories;
				}
				AAssetDir* dir = AAssetManager_openDir(mgr, assetDir.c_str());
				if (!dir) {
					PN_CORE_WARN("Cannot open asset directory: {}", assetDir);
					return directories;
				}
				const char* filename = nullptr;
				while ((filename = AAssetDir_getNextFileName(dir)) != nullptr) {
					// AAssetDir does not distinguish between files and directories. (Docnote)
					std::string dname = filename;
					bool matchesFilter = filter.empty() || dname.find(filter) != std::string::npos;
					// You may want to filter only known directories if you have a directory manifest, else list all.
					if (matchesFilter)
						directories.push_back(assetDir + "/" + dname);
				}
				AAssetDir_close(dir);
				return directories;
			}

			// Filesystem listing
			std::string actualPath = resolvePath(virtualPath);
			if (!isValidPath(actualPath)) {
				throw std::runtime_error("Path does not exist or is not a directory: " + actualPath);
			}
			std::vector<std::string> directories;
			DIR* dir = opendir(actualPath.c_str());
			if (!dir) {
				PN_CORE_WARN("Cannot open directory: {}", actualPath);
				return directories;
			}
			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr) {
				if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
					std::string dirname = entry->d_name;
					bool matchesFilter = filter.empty() || dirname.find(filter) != std::string::npos;
					if (matchesFilter)
						directories.push_back(actualPath + "/" + dirname);
				}
			}
			closedir(dir);
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
			return parseVirtualPath(virtualPath).first == assets_alias || parseVirtualPath(virtualPath).first == engine_assets_alias || parseVirtualPath(virtualPath).first == game_assets_alias;
		}

		std::unique_ptr<IFileStream> AndroidPath::createFileStream(const std::string& virtualPath, FileMode mode) {

			//Resolve path and create stream
			auto path = resolvePath(virtualPath);

			//Check if virtual path is asset
			if (isAssetPath(virtualPath) && mode == FileMode::Read) {
				AAssetManager* asset_mgr = m_app->activity->assetManager;
				return std::make_unique<AndroidAssetStream>(asset_mgr, path, FileMode::Read);
			}
			else {

				//Ensure path mesh_id
				if (!std::filesystem::mesh_id(path)) {
					throw std::runtime_error("File does not exist! Unable to create file stream");
				}

				return std::make_unique<AndroidAssetStream>(nullptr, path, mode);
			}
		}
	}
}

#endif
