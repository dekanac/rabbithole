#include "ShaderCompiler.h"

#include "Logger/Logger.h"
#include "Utils/utils.h"
#include <thread>
#include <atomic>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstring>
#include <vector>

static std::atomic<bool> g_QuitRequested = false;
static std::atomic<bool> g_FileChanged = false;
static std::string g_ChangedFilename = "";

ShaderCompiler::ShaderCompiler()
{
    m_ShadersDir = Utils::FindResFolder().string() + "/shaders/";
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
    spSetMatrixLayoutMode(request, SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
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
        return SLANG_STAGE_CLOSEST_HIT;
    case 'M':
        return SLANG_STAGE_MISS;
    default:
        LOG_ERROR("Unrecognized shader stage! Should be CS, VS, FS, HS, MS or RS");
        return SLANG_STAGE_NONE;
    }
}

void MonitorChangesThread(int inotifyFd, int watchDescriptor)
{
    char buffer[1024 * (sizeof(struct inotify_event) + 16)];

    while (!g_QuitRequested)
    {
        int length = read(inotifyFd, buffer, sizeof(buffer));
        if (length < 0 && !g_QuitRequested)
        {
            perror("read");
            break;
        }

        int i = 0;
        while (i < length)
        {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->len)
            {
                if (event->mask & IN_MODIFY || event->mask & IN_CREATE || event->mask & IN_DELETE)
                {
                    g_FileChanged = true;
                    g_ChangedFilename = event->name;
                    LOG_INFO("File changed: {}", g_ChangedFilename);
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
}

bool FileChangeMonitor::CheckForChanges(std::string& inputString)
{
    if (g_FileChanged)
    {
        g_FileChanged = false;
        inputString = g_ChangedFilename;
        return true;
    }
    return false;
}

bool FileChangeMonitor::Init(const std::string& shadersDir)
{
    m_InotifyFd = inotify_init();
    if (m_InotifyFd < 0)
    {
        perror("inotify_init");
        return false;
    }

    m_WatchDescriptor = inotify_add_watch(m_InotifyFd, shadersDir.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    if (m_WatchDescriptor < 0)
    {
        perror("inotify_add_watch");
        close(m_InotifyFd);
        return false;
    }

    m_MonitorThread = new std::thread(MonitorChangesThread, m_InotifyFd, m_WatchDescriptor);
    LOG_INFO("Monitoring changes to {}", shadersDir);

    return true;
}

bool FileChangeMonitor::Shutdown()
{
    g_QuitRequested = true;

    if (m_MonitorThread && m_MonitorThread->joinable())
    {
        m_MonitorThread->join();
        delete m_MonitorThread;
    }

    if (m_WatchDescriptor >= 0)
    {
        inotify_rm_watch(m_InotifyFd, m_WatchDescriptor);
    }

    if (m_InotifyFd >= 0)
    {
        close(m_InotifyFd);
    }

    return true;
}

