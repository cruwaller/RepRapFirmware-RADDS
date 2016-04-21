#ifndef MASSSTORAGE_H

#define MASSSTORAGE_H

#include "Pins.h"

enum SDCardState
{
	JUST_REMOVED = 0,
	REMOVED = 1,
	READY = 2
};

class Platform;
class FileInfo;

class MassStorage
{
public:

	bool FindFirst(const char *directory, FileInfo &file_info);
	bool FindNext(FileInfo &file_info);
	const char* GetMonthName(const uint8_t month);
	const char* CombineName(const char* directory, const char* fileName);
	bool Delete(const char* directory, const char* fileName);
	bool MakeDirectory(const char *parentDir, const char *dirName);
	bool MakeDirectory(const char *directory);
	bool Rename(const char *oldFilename, const char *newFilename);
	bool FileExists(const char *file) const;
	bool FileExists(const char* directory, const char *fileName) const;
	bool DirectoryExists(const char *path) const;
	bool DirectoryExists(const char* directory, const char* subDirectory);
	bool FileSystemAvailable();

friend class Platform;

protected:

	MassStorage(Platform* p);
	void Init();
	void Reset();

private:

#if defined(SD_DETECT_PIN) && defined(SD_DETECT_VAL)
	bool fileSystemInit();
#endif

	Platform* platform;
	FATFS fileSystem;
	DIR findDir;
	char combinedName[FILENAME_LENGTH + 1];
};

#endif
