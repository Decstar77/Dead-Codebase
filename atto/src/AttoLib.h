#pragma once

#include "AttoDefines.h"
#include "AttoContainers.h"
#include "AttoInput.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

struct GLFWwindow;
struct GLFWmonitor;
struct ALCdevice;
struct ALCcontext;

namespace atto
{
    class LeEngine;
    class GameState;

    enum class LogLevel {
        FATAL = 0,
        ERR = 1,
        WARN = 2,
        INFO = 3,
        DEBUG = 4,
        TRACE = 5,
        COUNT = 6
    };

    class Logger
    {
    public:
        Logger();
        ~Logger();

        static void LogOutput(LogLevel level, const char* message, ...);

    private:
        inline static Logger* instance = nullptr;
        static constexpr u32            LOG_BUFFER_SIZE = 1024;

        char                            logBuffer[LOG_BUFFER_SIZE] = {};
        char                            outputBuffer[LOG_BUFFER_SIZE] = {};
        FixedList<LargeString, 10000>   logs = {};
    };

    struct AppState {
        ALCdevice*                  alDevice = nullptr;
        ALCcontext*                 alContext = nullptr;
        GLFWmonitor*                monitor = nullptr;
        LargeString                 monitorName = LargeString::FromLiteral("");
        f64                         monitorRefreshRate = 0;
        GLFWwindow*                 window = nullptr;
        Logger*                     logger = nullptr;
        FrameInput*                 input = nullptr;
        LeEngine*                   engine = nullptr;
        GameState*                  gameState = nullptr;
        f32                         deltaTime = 0.0f;
        i32                         windowWidth = 1280;
        i32                         windowHeight = 720;
        f32                         windowAspect = (f32)windowWidth / (f32)windowHeight;
        SmallString                 windowTitle = SmallString::FromLiteral("Game");
        bool                        windowCreateCentered = true;
        bool                        windowVsync = true;
        bool                        windowFullscreen = false;
        bool                        shouldClose = false;
        bool                        useLooseAssets = false;
        LargeString                 looseAssetPath = LargeString::FromLiteral("assets/");
    };

    class Clock {
    public:
        void            Start();
        void            End();
        f64             GetElapsedMicroseconds();
        f64             GetElapsedMilliseconds();
        f64             GetElapsedSeconds();
    private:
        f64             startTime = 0;
        f64             endTime = 0;
    };

    class ProfilerClock {
    public:
        ProfilerClock(const char * name);
        ~ProfilerClock();
        
    private:
        Clock       clock;
        SmallString name;
    };

    class GameState {
    public:
        virtual bool Initialize(AppState* app) = 0;
        virtual void Update(AppState* app) = 0;
        virtual void Render(AppState* app) = 0;
        virtual void Shutdown(AppState* app) = 0;
    };

    class Application {
    public:
        inline static AppState* GetAppState() { return appState; }

        static bool         CreateApp(AppState &window);
        static void         DestroyApp(AppState &window);
        static bool         AppIsRunning(AppState& window);
        static void         PresentApp(AppState& window);
        static void         UpdateApp(AppState& app);
        
        static void         ConsoleWrite(const char *output, u8 level);
        static void         DisplayFatalError(const char* output);

    private:
        inline static AppState* appState = nullptr;
    };
}


// Logs a fatal-level message.
#define ATTOFATAL(message, ...) atto::Logger::LogOutput(atto::LogLevel::FATAL, message, ##__VA_ARGS__);

#ifndef ATTOERROR
// Logs an error-level message.
#define ATTOERROR(message, ...) atto::Logger::LogOutput(atto::LogLevel::ERR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Logs a warning-level message.
#define ATTOWARN(message, ...) atto::Logger::LogOutput(atto::LogLevel::WARN, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_WARN_ENABLED != 1
#define ATTOWARN(message)
#endif

#if LOG_INFO_ENABLED == 1
// Logs a info-level message.
#define ATTOINFO(message, ...) atto::Logger::LogOutput(atto::LogLevel::INFO, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define ATTOINFO(message)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug-level message.
#define ATTODEBUG(message, ...) atto::Logger::LogOutput(atto::LogLevel::DEBUG, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define ATTODEBUG(message)
#endif

#if LOG_TRACE_ENABLED == 1
// Logs a trace-level message.
#define ATTOTRACE(message, ...) atto::Logger::LogOutput(atto::LogLevel::TRACE, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_TRACE_ENABLED != 1
#define ATTOTRACE(message)
#endif
