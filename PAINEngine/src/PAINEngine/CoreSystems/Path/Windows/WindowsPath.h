#pragma once

#ifdef PN_PLATFORM_WINDOWS
#ifndef WINDOWS_PATH_HPP
#define WINDOWS_PATH_HPP

#include "../Path.h"

namespace PAIN {
	namespace Path {

		// Windows Implementation
		class WinFileStream : public IFileStream {
			std::fstream file;
			size_t fileSize = 0;
		public:
			WinFileStream(const std::string& path, FileMode mode) {
				std::ios_base::openmode fmode = std::ios::binary;
				if (mode == FileMode::Read) fmode |= std::ios::in;
				else if (mode == FileMode::Write) fmode |= std::ios::out | std::ios::trunc;
				else if (mode == FileMode::ReadWrite) fmode |= std::ios::in | std::ios::out;
				file.open(path, fmode);
				if (file) {
					file.seekg(0, std::ios::end);
					fileSize = static_cast<size_t>(file.tellg());
					file.seekg(0, std::ios::beg);
				}
			}
			~WinFileStream() override {
				if (file.is_open()) {
					file.close();
				}
			}
			size_t read(void* buffer, size_t size) override {
				file.read(static_cast<char*>(buffer), size);
				return static_cast<size_t>(file.gcount());
			}
			size_t write(const void* buffer, size_t size) override {
				file.write(static_cast<const char*>(buffer), size);
				return file ? size : 0;
			}
			void flush() override { file.flush(); }
			void seek(size_t pos) override { file.clear(); file.seekg(pos, std::ios::beg); file.seekp(pos, std::ios::beg); }
			size_t tell() override { return static_cast<size_t>(file.tellg()); }
			bool eof() const override { return file.eof(); }
			size_t size() const override { return fileSize; }
			bool good() const override { return file.good(); }
		};

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

			//List of watchers
			std::unordered_map<std::filesystem::path, std::unique_ptr<filewatch::FileWatch<std::string>>> dir_watchers;

			//Private initialize and destroy
			void init();
			void destroy();
		public:

			WindowsPath() { init(); };
			virtual ~WindowsPath() { destroy(); }

			std::string normalizePath(const std::string& path) const override;

			std::string getVirtualParentPath(std::string const& virtual_path) const override;

			//Override functions from path interface
			void registerVirtualPath(const std::string& alias, const std::string& path, bool create_new = false) override;
			void updateVirtualPath(const std::string& alias, const std::string& path) override;
			std::string resolvePath(const std::string& virtualPath) const override;
			std::string resolvePath(const std::string& alias, std::string const& relative) const override;
			std::vector<std::string> listFiles(const std::string& virtualPath, const std::string& filter = "", const std::string& extension = "") const override;
			std::vector<std::string> listDirectories(const std::string& virtualPath, const std::string& filter = "") const override;
			bool pathExists(const std::string& virtualPath) const override;
			bool createDirectory(const std::string& virtualPath) const override;

			std::unique_ptr<IFileStream> createFileStream(const std::string& virtualPath, FileMode mode) override;

			//Watch directory
			void watchDirectory(std::string const& virtual_path, FileWatchEventCallback callback) override;

			//Watch directory & child directories
			void watchDirectoryTree(std::string const& virtual_path, FileWatchEventCallback callback) override;

			//Stop watching directory
			void stopWatchingDirectory(std::string const& virtual_path) override;

			//Stop watching directory & child directories
			void stopWatchingDirectoryTree(std::string const& virtual_path) override;

			//Stop watching all directories
			void stopWatchingAllDirectories() override;

			//Log watched directories
			void logWatchedDirectories() const override;
		};

	}
}

#endif
#endif
