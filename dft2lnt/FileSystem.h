class File;

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

char *realpath(const char *path, char resolved_path[PATH_MAX]);
char *cwd_realpath(const char *path, char resolved_path[PATH_MAX]);
char* path_basename(const char* path);

#ifdef __cplusplus
} // extern "C"
#endif

class FileSystem {
private:
	std::vector<std::string> dirStack;
public:
	static std::string getRealPath(const std::string& filePath);
	static std::string getDirName(const std::string& filePath);
	static std::string getBaseName(const std::string& filePath);
	static std::string getFileExtension(const std::string& filePath);
	static std::string getFileBase(const std::string& filePath);
	static void getFileBaseAndExtension(std::string& fileBase, std::string& fileExtension, const std::string& filePath);
	static int exists(const File& file);
	static int move(const File& from, const File& to, bool interactive=false, bool safe=false);
	static int copy(const File& from, const File& to, bool interactive=false, bool safe=false);
	static int mkdir(const File& dir, int mode=0755);
	static int chdir(const File& dir);
	static int isDir(const File& file);
	static int remove(const File& file);
	static time_t getLastAccessTime(const File& file);
	static time_t getLastModificationTime(const File& file);
	static time_t getLastStatusChangeTime(const File& file);
	static bool hasAccessTo(const File& file, int mode);
	static int findInPath(std::vector<File>& result, const File& file);
	static int findInPath(std::vector<File>& result, const File& file, const char* path);

	static std::string* load(const File& file);
};

class PushD {
private:
	std::vector<std::string> dirStack;
public:
	PushD() {
	}
	PushD(std::string path) {
		pushd(path);
	}
	
	virtual ~PushD() {
		popd();
	}
	
	int pushd(std::string dir) {
		char buffer[PATH_MAX];
		char* res = getcwd(buffer,PATH_MAX);
		if(res) {
			dirStack.push_back( std::string(buffer) );
			chdir(dir.c_str());
			return 0;
		} else {
			return 1;
		}
	}
	int popd() {
		if(dirStack.size()>0) {
			chdir(dirStack.back().c_str());
			dirStack.pop_back();
			return 0;
		} else {
			return 1;
		}
	}
};

class File {
private:
	std::string pathTo;
	std::string fileBase;
	std::string fileExtension;

	std::string fileName;
	std::string filePath;

	void updateExtra() {
		this->fileName = fileExtension == "" ? fileBase : fileBase + "." + fileExtension;
		this->filePath = pathTo        == "" ? fileName : pathTo + "/" + fileName;
	}
public:
	File() {};
	File(const std::string& filePath);
	File(const std::string& pathTo, const std::string& fileName);
	File(const std::string& pathTo, const std::string& fileBase, const std::string& fileExtension);
	
	const std::string& getPathTo() const { return pathTo; }
	const std::string& getFileName() const { return fileName; }
	const std::string& getFileBase() const { return fileBase; }
	const std::string& getFileExtension() const { return fileExtension; }
	const std::string& getFilePath() const { return filePath; }
	std::string getFileRealPath() const { return FileSystem::getRealPath(filePath); }
	
	void setFileExtension(const std::string& fileExtension );

	File newWithExtension(const std::string& fileExtension) const;
	File newWithName(const std::string& fileBase, const std::string& fileExtension) const;
	File newWithName(const std::string& fileName) const;
	File newWithPathTo(const std::string& filePath) const;
	
	inline bool operator==(const File& other) const {
		return this->getFilePath() == other.getFilePath();
	}
	
	inline bool operator!=(const File& other) const {
		return !(*this == other);
	}
	
	inline bool isModifiedLaterThan(const File& other) const {
		return FileSystem::getLastModificationTime(*this) > FileSystem::getLastModificationTime(other);
	}
	inline bool isAccessedLaterThan(const File& other) const {
		return FileSystem::getLastAccessTime(*this) > FileSystem::getLastAccessTime(other);
	}
	
	inline bool isEmpty() {
		return fileBase.empty() && fileExtension.empty();
	}
};

#endif // FILESYSTEM_H