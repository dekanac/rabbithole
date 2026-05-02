#pragma once

#include <slang.h>

#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#elif __linux__
#include <sys/inotify.h>
#include <unistd.h>
#endif

class Shader;
class VulkanDevice;
class ResourceManager;

class FileChangeMonitor
{
public:
    FileChangeMonitor() {}
    ~FileChangeMonitor() {}

    void MonitorChangesThread();

    bool CheckForChanges(std::string& inputString);
    bool Init(const std::string& shadersDir);
    bool Shutdown();

private:
    std::thread* m_MonitorThread = nullptr;
#if __linux__
    int m_InotifyFd = -1;
    int m_WatchDescriptor = -1;
#elif WIN32
    HANDLE m_ShutdownEvent;
    HANDLE m_DirectoryHandle;
    OVERLAPPED m_Overlapped;
#endif
};

class ShaderCompiler
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    bool Update(std::string& fileChanged);
    bool CompileShader(const std::string& shaderName, const std::string& entryPoint, void** outData,
                       size_t* outDataSize,
                       std::vector<const char*> defines = std::vector<const char*>());

private:
    SlangStage GetStageFromShaderName(const std::string& shaderName);

    SlangSession* m_Session = nullptr;
    FileChangeMonitor m_FileChangeMonitor;
    std::string m_ShadersDir = "";
};
