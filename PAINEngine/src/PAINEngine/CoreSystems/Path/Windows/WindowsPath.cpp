#include "pch.h"
#include "WindowsPath.h"

#ifdef PN_PLATFORM_WINDOWS

#include <ShlObj.h>
#include <KnownFolders.h>
#include <combaseapi.h>

namespace PAIN {
	namespace Path {

		//Create path service
		Path* Path::create(void* app) {
			return new WindowsPath();
		}

        std::string WindowsPath::getKnownFolderPath(const GUID& folderId) const {
            PWSTR path;
            if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &path))) {
                // Convert wide string to UTF-8 using Windows API
                int utf8Length = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                if (utf8Length == 0) {
                    CoTaskMemFree(path);
                    throw std::runtime_error("Failed to convert wide string to UTF-8");
                }

                std::string result(utf8Length - 1, 0); // -1 to exclude null terminator
                WideCharToMultiByte(CP_UTF8, 0, path, -1, &result[0], utf8Length, nullptr, nullptr);

                CoTaskMemFree(path);
                return normalizePath(result);
            }
            throw std::runtime_error("Failed to get Windows known folder path");
        }

        std::string WindowsPath::normalizePath(const std::string& path) const {
            std::filesystem::path p(path);
            return p.lexically_normal().string();
        }

        void WindowsPath::init() {

            //Default engine name
            app_name = ENGINE_NAME;

            //Get windows paths
            out_path = normalizePath(std::filesystem::current_path().string());
            localdata_path = getKnownFolderPath(FOLDERID_LocalAppData);
            roamingdata_path = getKnownFolderPath(FOLDERID_RoamingAppData);
            documents_path = getKnownFolderPath(FOLDERID_Documents);

            // Calculate project root
            std::filesystem::path project_root = Assets::findProjectRoot();

            std::string project_root_str = normalizePath(project_root.string());

            //Register default virtual paths
            registerVirtualPath("out", out_path);

            //Temp paths to be changed
            registerVirtualPath(main_assets_alias, project_root_str + "/assets", false);
            registerVirtualPath(main_game_assets_alias, project_root_str + "/assets/" + relative_game_folder, false);
            registerVirtualPath(main_engine_assets_alias, project_root_str + "/assets/" + relative_engine_folder, false);
            registerVirtualPath(assets_alias, out_path + "/assets", true);
            registerVirtualPath(game_assets_alias, out_path + "/assets/" + relative_game_folder, true);
            registerVirtualPath(engine_assets_alias, out_path + "/assets/" + relative_engine_folder, true);
            registerVirtualPath(local_alias, localdata_path + "/" + app_name, true);
            registerVirtualPath(roaming_alias, roamingdata_path + "/" + app_name, true);
            registerVirtualPath(documents_alias, documents_path + "/" + app_name, true);
            registerVirtualPath(temp_alias, localdata_path + "/" + app_name + "/temp", true);
            registerVirtualPath(game_folder_alias, project_root_str + "/Game", false);
        }

        void WindowsPath::destroy() {
            virtual_paths.clear();
        }

        std::string WindowsPath::getVirtualParentPath(std::string const& virtual_path) const {

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

        void WindowsPath::registerVirtualPath(const std::string& alias, const std::string& path, bool create_new) {

            //check if path actually mesh_id
            if (!std::filesystem::exists(path)) {

                //Create new path
                if (create_new) {
                    std::filesystem::create_directories(path);
                }
                else {
                    PN_CORE_WARN("Path: {} does not exist. Invalid registering of path.", path);
                    return;
                }
            }

            //check if path actually mesh_id
            if (virtual_paths.find(alias) != virtual_paths.end()) {
                PN_CORE_WARN("Alias: {} already mesh_id. Invalid registering of path.", alias);
                return;
            }

            //Register path
            virtual_paths[alias] = normalizePath(path);
            PN_CORE_INFO("Registered Virtual Path: {} -> {}", alias, path);
        }

        void WindowsPath::updateVirtualPath(const std::string& alias, const std::string& path) {
            //check if path actually mesh_id
            if (!std::filesystem::exists(path)) {
                PN_CORE_WARN("Path: {} does not exist. Invalid registering of path.", path);
                return;
            }

            //check if path actually mesh_id
            auto it = virtual_paths.find(alias);
            if (it == virtual_paths.end()) {
                PN_CORE_WARN("Alias: {} does not mesh_id. Invalid registering of path.", alias);
                return;
            }

            //Update path
            it->second = normalizePath(path);
            PN_CORE_INFO("Updated: {} -> {}", alias, path);
        }

        std::string WindowsPath::resolvePath(const std::string& virtualPath) const {
            auto [alias, relative_path] = parseVirtualPath(virtualPath);

            auto it = virtual_paths.find(alias);
            if (it == virtual_paths.end()) {
                throw std::runtime_error("Unknown virtual path alias: " + alias);
            }

            if (relative_path.empty()) {
                return it->second;
            }

            std::filesystem::path fullPath = std::filesystem::path(it->second) / relative_path;
            return normalizePath(fullPath.string());
        }

        std::string WindowsPath::resolvePath(const std::string& alias, std::string const& relative) const {

            auto it = virtual_paths.find(alias);
            if (it == virtual_paths.end()) {
                throw std::runtime_error("Unknown virtual path alias: " + alias);
            }

            if (relative.empty()) {
                return it->second;
            }

            std::filesystem::path fullPath = std::filesystem::path(it->second) / relative;
            return normalizePath(fullPath.string());
        }

        std::vector<std::string> WindowsPath::listFiles(const std::string& virtualPath, const std::string& filter, const std::string& extension) const {
            //Get actual path
            auto actual_path = resolvePath(virtualPath);

            //Check if path mesh_id
            if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
                throw std::runtime_error("Path does not exist or is not a directory: " + actual_path);
            }

            //Store of files in a virtual path
            std::vector<std::string> files;
            for (const auto& file : std::filesystem::directory_iterator(actual_path)) {

                //Check for files with extension or if no extension filter present, extract all files
                if (file.is_regular_file() && (filter.empty() || (file.path().string().find(filter) != file.path().string().npos)) && (extension.empty() || file.path().extension() == extension)) {
                    files.push_back(file.path().string());
                }
            }

            //Return all files
            return files;
        }

        std::vector<std::string> WindowsPath::listDirectories(const std::string& virtualPath, const std::string& filter) const {

            //Get actual path
            auto actual_path = resolvePath(virtualPath);

            //Check if path mesh_id
            if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
                throw std::runtime_error("Path does not exist or is not a directory: " + actual_path);
            }

            //Store of directories in a virtual path
            std::vector<std::string> directories;
            for (const auto& directory : std::filesystem::directory_iterator(actual_path)) {

                //Check for directory
                if (directory.is_directory() && (filter.empty() || (directory.path().string().find(filter) != directory.path().string().npos))) {
                    directories.push_back(directory.path().string());
                }
            }

            //Return all directories
            return directories;
        }

        bool WindowsPath::pathExists(const std::string& virtualPath) const {
            try {
                std::string actualPath = resolvePath(virtualPath);
                return std::filesystem::exists(actualPath);
            }
            catch (const std::exception&) {
                return false;
            }
        }

        bool WindowsPath::createDirectory(const std::string& virtualPath) const {
            try {
                std::string actualPath = resolvePath(virtualPath);
                return std::filesystem::create_directories(actualPath);
            }
            catch (const std::exception&) {
                return false;
            }
        }

        std::unique_ptr<IFileStream> WindowsPath::createFileStream(const std::string& virtualPath, FileMode mode, bool bin) {

            auto path = resolvePath(virtualPath);

            // Only assert for reads, not writes.
            if (mode == FileMode::Read) {
                assert(std::filesystem::exists(path) && "Scene file does not exist or path is invalid!");
                if (!std::filesystem::exists(path)) {
                    PN_CORE_WARN("File does not exist! Unable to create file stream for reading");
                    return std::unique_ptr<IFileStream>();
                }
            }

#ifdef PN_PLATFORM_WINDOWS
            // Only windows have writing
            // For writing: if the parent directory doesn't exist, create it
            else if (mode == FileMode::Write || mode == FileMode::ReadWrite) {
                auto p = std::filesystem::path(path);
                if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
                    std::filesystem::create_directories(p.parent_path());
                }
            }
#endif

            return std::make_unique<WinFileStream>(path, mode, bin);
        }

        void WindowsPath::watchDirectory(std::string const& virtual_path, FileWatchEventCallback callback) {

            //Get actual path
            std::filesystem::path actual_path = resolvePath(virtual_path);

            //Check if path mesh_id
            if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
                throw std::runtime_error("Path does not exist or is not a directory: " + actual_path.string());
            }

            //Add watcher for the directory
            dir_watchers.emplace(actual_path,
                std::make_unique<filewatch::FileWatch<std::string>>(actual_path.string(),
                    [actual_path, callback, this](std::string const& file, filewatch::Event event) {

                        //Call callback
                        callback(actual_path / file, event);
                    }));
        }

        void WindowsPath::watchDirectoryTree(std::string const& virtual_path, FileWatchEventCallback callback) {

            //Get alias
            auto alias = getAlias(virtual_path);

            //Get actual path
            std::filesystem::path actual_path = resolvePath(virtual_path);

            //Check if path mesh_id
            if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
                throw std::runtime_error("Path does not exist or is not a directory: " + actual_path.string());
            }

            //Function to add watcher for a directory
            auto addWatcher = [this, alias, callback](std::filesystem::path const& dir_path) {
                watchDirectory(aliasCombineRelative(alias, dir_path.string()), callback);
                PN_CORE_INFO("New Directory Watched: " + dir_path.string());
                };

            //Add watcher for child directories
            for (const auto& dir : std::filesystem::recursive_directory_iterator(actual_path)) {

                //Register directory
                if (dir.is_directory()) {
                    addWatcher(dir.path());
                }
            }

            //Watch the root directory and dynamically add watchers for new directories
            watchDirectory(virtual_path, [this, callback, addWatcher](std::filesystem::path const& path, filewatch::Event event) {

                //Add a watcher for the new directory
                if (event == filewatch::Event::added && std::filesystem::is_directory(path)) {
                    addWatcher(path);
                    return;
                }

                //Pass the event to the callback
                callback(path, event);
                });
        }

        void WindowsPath::stopWatchingDirectory(std::string const& virtual_path) {
            //Get actual path
            std::filesystem::path actual_path = resolvePath(virtual_path);

            //Check if path mesh_id
            if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
                throw std::runtime_error("Path does not exist or is not a directory: " + actual_path.string());
            }

            //Erase directory from watchers
            auto it = dir_watchers.find(actual_path);
            if (it != dir_watchers.end()) {
                dir_watchers.erase(it);
            }
        }

        void WindowsPath::stopWatchingDirectoryTree(std::string const& virtual_path) {
            //Get alias
            auto alias = getAlias(virtual_path);

            //Get actual path
            std::filesystem::path actual_path = resolvePath(virtual_path);

            //Check if path mesh_id
            if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
                throw std::runtime_error("Path does not exist or is not a directory: " + actual_path.string());
            }

            //Add watcher for child directories
            for (const auto& dir : std::filesystem::recursive_directory_iterator(actual_path)) {
                if (dir.is_directory()) {
                    auto path = aliasCombineRelative(alias, dir.path().string());

                    //Erase directory from watchers
                    auto it = dir_watchers.find(path);
                    if (it != dir_watchers.end()) {
                        dir_watchers.erase(it);
                    }
                }
            }

            //Erase directory from watchers
            auto it = dir_watchers.find(virtual_path);
            if (it != dir_watchers.end()) {
                dir_watchers.erase(it);
            }
        }

        void WindowsPath::stopWatchingAllDirectories() {
            dir_watchers.clear();
        }

        void WindowsPath::logWatchedDirectories() const {
            PN_CORE_INFO("Currently watched directories:");
            for (const auto& [path, watcher] : dir_watchers) {
                PN_CORE_INFO(path.string());
            }
        }
	}
}

#endif
