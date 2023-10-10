#include "ShaderCompiler.h"

#include "Logger/Logger.h"
#include "Render/Shader.h"
#include "Render/ResourceManager.h"
#include "Render/Renderer.h"
#include "Utils/Utils.h"
#include <iostream>

static std::atomic<bool> g_QuitRequested = false;
static std::atomic<bool> g_FileChanged = false;
static std::string g_ChangedFilename = "";

ShaderCompiler::ShaderCompiler()
{
	m_ShadersDir = Utils::FindResFolder().string() + "\\shaders\\";
	m_Session = spCreateSession();
	m_FileChangeMonitor.Init(m_ShadersDir);
	m_Session->setDefaultDownstreamCompiler(SLANG_SOURCE_LANGUAGE_HLSL, SLANG_PASS_THROUGH_DXC);
}

ShaderCompiler::~ShaderCompiler()
{
	spDestroySession(m_Session);
	m_FileChangeMonitor.Shutdown();
}

bool ShaderCompiler::Update(std::string& fileChanged)
{
	return m_FileChangeMonitor.CheckForChanges(fileChanged);
}

bool ShaderCompiler::CompileShader(const std::string& shaderName, const std::string& entryPoint, void** outData, size_t* outDataSize, std::vector<const char*> defines)
{
	SlangCompileRequest* request = spCreateCompileRequest(m_Session);
	std::string shaderPath = m_ShadersDir + shaderName;
	std::string shaderExt = shaderName.substr(shaderName.find_last_of('.') + 1);

	request->addCodeGenTarget(SLANG_SPIRV);
	request->addSearchPath(m_ShadersDir.c_str());
	request->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_STANDARD);

	SlangSourceLanguage srcLanguage = SLANG_SOURCE_LANGUAGE_SLANG;

	auto translationUnitIdx = request->addTranslationUnit(srcLanguage, "");
	request->addTranslationUnitSourceFile(translationUnitIdx, shaderPath.c_str());
	auto entryPointIndex = request->addEntryPoint(translationUnitIdx, entryPoint.c_str(), GetStageFromShaderName(shaderName));
	
	for (auto define : defines)
	{
		request->addTranslationUnitPreprocessorDefine(translationUnitIdx, define, "1");
	}

	auto anyErrors = request->compile();
	if (anyErrors)
	{
		auto diagnostics = request->getDiagnosticOutput();
		LOG_CRITICAL("Compilation of %s shader failed with: {}: {}", shaderName.c_str(), diagnostics);
		return false;
	}

	const void* dataLocal = request->getEntryPointCode(entryPointIndex, outDataSize);
	
	*outData = malloc(*outDataSize);
	if (!*outData)
	{
		LOG_CRITICAL("Failed to allocate data for shader creation! Shader: %s", shaderName.c_str());
		return false;
	}

	memcpy(*outData, dataLocal, *outDataSize);

	spDestroyCompileRequest(request);

	return true;
}

SlangStage ShaderCompiler::GetStageFromShaderName(const std::string& shaderName)
{
	switch (shaderName[0])
	{
	case 'C':
		return SLANG_STAGE_COMPUTE;
	case 'V':
		return SLANG_STAGE_VERTEX;
	case 'F':
		return SLANG_STAGE_FRAGMENT;
	case 'R':
		return SLANG_STAGE_RAY_GENERATION;
	case 'H':
		return SLANG_STAGE_ANY_HIT;
	case 'M':
		return SLANG_STAGE_MISS;
	default:
		LOG_ERROR("Unrecognized shader stage! Should be CS, VS, FS, HS, MS or RS");
		return SLANG_STAGE_NONE;
	}
}
// Function that the separate thread will execute
void MonitorChangesThread(HANDLE change, HANDLE changeEvent)
{
	while (!g_QuitRequested)
	{
		DWORD waitResult = WaitForSingleObject(change, INFINITE);

		if (waitResult == WAIT_OBJECT_0)
		{
			g_FileChanged = true;

			// Retrieve the change information
			char buffer[1024];
			DWORD bytesRead;
			if (ReadDirectoryChangesW(change, buffer, sizeof(buffer), FALSE,
				FILE_NOTIFY_CHANGE_FILE_NAME, &bytesRead, NULL, NULL))
			{
				FILE_NOTIFY_INFORMATION* fileInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
				WCHAR wideFilename[MAX_PATH];
				wcsncpy_s(wideFilename, fileInfo->FileName, fileInfo->FileNameLength / sizeof(WCHAR));
				wideFilename[fileInfo->FileNameLength / sizeof(WCHAR)] = L'\0';
				char filename[MAX_PATH];
				WideCharToMultiByte(CP_ACP, 0, wideFilename, -1, filename, MAX_PATH, NULL, NULL);
				g_ChangedFilename = filename;
			}

			SetEvent(changeEvent);
		}
		else
		{
			LOG_CRITICAL("Error finding next change notification");
			break;
		}
	}
}

bool FileChangeMonitor::CheckForChanges(std::string& inputString)
{
	if (g_FileChanged) 
	{
		g_FileChanged = false;
		LOG_INFO("File changed: {}", g_ChangedFilename);

		inputString = g_ChangedFilename;
		return true;
	}

	return false;
}

bool FileChangeMonitor::Init(const std::string& shadersDir)
{
	// Specify the directory to monitor and the type of changes to track
	LPCSTR directoryPath = shadersDir.c_str();
	DWORD notifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE;  // Track changes to file names

	// Start watching for changes
	m_hChange = FindFirstChangeNotificationA(directoryPath, FALSE, notifyFilter);
	if (m_hChange == INVALID_HANDLE_VALUE) {
		LOG_CRITICAL("Error finding change notification handle!");
		return false;
	}

	LOG_INFO("Monitoring changes to {}", directoryPath);

	// Create an event for synchronization
	m_hChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!m_hChangeEvent) {
		std::cerr << "Error creating event: " << GetLastError() << std::endl;
		return false;
	}

	// Create a thread for monitoring changes
	m_MonitorThread = new std::thread(MonitorChangesThread, m_hChange, m_hChangeEvent);

	return true;
}

bool FileChangeMonitor::Shutdown()
{
	// Clean up
	g_QuitRequested = true;
	FindCloseChangeNotification(m_hChange);
	CloseHandle(m_hChangeEvent);
	m_MonitorThread->join();
	delete(m_MonitorThread);

	return true;
}
