#pragma once

#ifdef PN_PLATFORM_WINDOWS
#ifndef WINDOWS_PATH_HPP
#define WINDOWS_PATH_HPP

#include "../Path.h"

namespace PAIN {
	namespace Path {

		class WindowsPath : public Path {
		private:

			//Path variables
			std::string app_name;
			std::string out_path;
			std::string localdata_path;
			std::string roamingdata_path;
			std::string documents_path;

			//Internal path functions
			std::string getKnownFolderPath(const GUID& folderId) const;
			std::string normalizePath(const std::string& path) const;


			//Private initialize and destroy
			void init();
			void destroy();
		public:

			WindowsPath() { init(); };
			virtual ~WindowsPath() { destroy(); }

			//Override functions from path interface
			void registerVirtualPath(const std::string& alias, const std::string& path, bool create_new = false) override;
			void updateVirtualPath(const std::string& alias, const std::string& path) override;
			std::string resolvePath(const std::string& virtualPath) const override;
			std::vector<std::string> listFiles(const std::string& virtualPath, const std::string& filter = "", const std::string& extension = "") const override;
			std::vector<std::string> listDirectories(const std::string& virtualPath, const std::string& filter = "") const override;
			bool pathExists(const std::string& virtualPath) const override;
			bool createDirectory(const std::string& virtualPath) const override;
		};

	}
}

#endif
#endif
