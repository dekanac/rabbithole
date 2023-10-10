#pragma once

#include <slang.h>
#include <string>

#include <windows.h>
#include <thread>
#include <vector>

class Shader;
class VulkanDevice;
class ResourceManager;

class FileChangeMonitor
{
public:
	FileChangeMonitor() {}
	~FileChangeMonitor() {}

	bool CheckForChanges(std::string& inputString);
	bool Init(const std::string& shadersDir);
	bool Shutdown();

private:
	std::thread* m_MonitorThread = nullptr;
	HANDLE m_hChangeEvent = nullptr;
	HANDLE m_hChange = nullptr;
};

class ShaderCompiler
{
public:
	ShaderCompiler();
	~ShaderCompiler();

	bool Update(std::string& fileChanged);
	bool CompileShader(const std::string& shaderName, const std::string& entryPoint, void** outData, size_t* outDataSize, std::vector<const char*> defines = std::vector<const char*>());

private:
	SlangStage GetStageFromShaderName(const std::string& shaderName);

	SlangSession* m_Session = nullptr;
	FileChangeMonitor m_FileChangeMonitor;
	std::string m_ShadersDir = "";
};