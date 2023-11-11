#pragma once

#include <string>

typedef void* ResManFile;

class ResourceManager
{
public:

    ResourceManager();
    ~ResourceManager();

    void AddArchive(const char* physicalFilePath, const char* virtualDirectoryPath, bool recursive);
    void AddDirectory(const char* physicalDirectoryPath, const char* virtualDirectoryPath, bool recursive);

    //void AddPath(const char* path, bool recurse);
    //RES_EXPORT int    ResCreatePath(char* path, int recurse);

    //ResManFile fopen(const char* name, const char* mode);
    //int fclose(ResManFile file);
    //long ftell(ResManFile file);
    //size_t fread(void* buffer, size_t size, size_t num, ResManFile file);
    //size_t fwrite(void* buffer, size_t size, size_t num, ResManFile file);
    //int fseek(ResManFile file, long offset, int whence);
    //int fprintf(ResManFile file, const char* fmt, ...);
    //int fscanf(ResManFile file, const char* fmt, ...);
    //char* fgets(char* buffer, int max_count, ResManFile file);
    //int feof(ResManFile file);

private:

};
