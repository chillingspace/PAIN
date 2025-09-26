/*****************************************************************//**
 * \file   sPath.h
 * \brief  Path manager declarations
 *
 * \author Ho Shu Hng, 2301339, shuhng.ho@digipen.edu (100%)
 * \date   September 2024
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once

#ifdef PN_PLATFORM_WINDOWS
#ifndef PATH_SERVICE_HPP
#define PATH_SERVICE_HPP

#include "pch.h"
#include "Applications/Appsystem.h"

namespace PAIN {
	 namespace Path {
		
	 	//Path global service
	 	class Service : public AppSystem {
	 	private:

	 		//Storage of virtual paths
	 		std::unordered_map<std::string, std::filesystem::path> virtual_paths;

	 		//Event call back for file watchess
	 		using FileWatchEventCallback = std::function<void(std::filesystem::path const&, filewatch::Event)>;

	 		//Map of file watchers
	 		std::unordered_map<std::filesystem::path, std::unique_ptr<filewatch::FileWatch<std::string>>> dir_watchers;

	 		//Map of last write times
	 		std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> file_write_times;

	 		//Root directory path
	 		std::filesystem::path root_path;
	 	public:

	 		//Default constructor and destructor
			Service() = default;

			//Optional virtual functions
			void onAttach() override {};
			void onDetach() override {};
			void onUpdate(float dt) override {};

			//Event handler for app layer
			void onEvent(Event::Event& e) override {};

	 		//Init
	 		void init(std::string const& config_path);

	 		//Register new virtual path
	 		void registerVirtualPath(std::string const& path_alias, std::filesystem::path const& path, bool b_full_path = false);

	 		//Update virtual path
	 		void updateVirtualPath(std::string const& path_alias, std::filesystem::path const& path, bool b_full_path = false);

	 		//Resolve virtual path
	 		std::filesystem::path resolvePath(std::string const& virtual_path) const;

	 		//Normalise path
	 		std::filesystem::path normalizePath(std::filesystem::path const& path) const;

	 		//Get virtual path alias
	 		std::string getAlias(std::string const& virtual_path) const;

	 		//Get virtual parent path
	 		std::string getVirtualParentPath(std::string const& virtual_path) const;

	 		//Convert to virtual path
	 		std::string convertToVirtualPath(std::string const& path_alias, std::string const& path) const;

	 		//Log all registered virtual paths
	 		void logVirtualPaths() const;

	 		//Find root path
	 		std::filesystem::path findRootDirectory();

	 		//List directories
	 		std::vector<std::filesystem::path> listDirectories(std::string const& virtual_path, std::string const& char_filter = "") const;

	 		//List files
	 		std::vector<std::filesystem::path> listFiles(std::string const& virtual_path, std::string const& char_filter = "", std::string const& ext_filter = "") const;

	 		//Watch directory
	 		void watchDirectory(std::string const& virtual_path, FileWatchEventCallback callback);

	 		//Watch directory & child directories
	 		void watchDirectoryTree(std::string const& virtual_path, FileWatchEventCallback callback);

	 		//Stop watching directory
	 		void stopWatchingDirectory(std::string const& virtual_path);

	 		//Stop watching directory & child directories
	 		void stopWatchingDirectoryTree(std::string const& virtual_path);

	 		//Stop watching all directories
	 		void stopWatchingAllDirectories();

	 		//Log watched directories
	 		void logWatchedDirectories() const;
	 	};
	 }
}
#endif // !PATH_SERVICE_HPP
#endif
