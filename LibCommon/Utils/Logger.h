//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//    .--------------------------------------------------.
//    |  This file is part of NTGraphics                 |
//    |  Created 2018 by NT (https://ttnghia.github.io)  |
//    '--------------------------------------------------'
//                            \o/
//                             |
//                            / |
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#pragma once

#include <CommonSetup.h>
#include <Utils/Timer.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <map>

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define WRAPPER           "||"
#define PADDING           ' '
#define INDENT_SIZE       4
#define MAX_BUFFER_LENGTH 128

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class Logger
{
    using Clock = std::chrono::system_clock;
public:
    Logger(const String& loggerName, bool bDefaultLogPolicy = true, bool bPrint2Console = true, bool bWriteLog2File = false, bool bDataLogger = false);
    void setLoglevel(spdlog::level::level_enum level);
    ////////////////////////////////////////////////////////////////////////////////

    void newLine() { printLog(""); }
    void newLineIf(bool bCondition) { if(bCondition) { printLog(""); } }
    void printSeparator();
    void printAligned(const String& s, char padding = PADDING, const String& wrapper = WRAPPER, UInt maxSize = 100);
    void printTextBox(const String& s);
    void printTextBox(const Vector<String>& strs);
    void printWarning(const String& s, UInt maxSize            = 100);
    void printWarningIndent(const String& s,  UInt indentLevel = 1, char trailing = ' ', UInt maxSize = 100);
    void printError(const String& s, UInt maxSize              = 100);
    void printErrorIndent(const String& s, UInt indentLevel    = 1, char trailing = ' ', UInt maxSize = 100);

    ////////////////////////////////////////////////////////////////////////////////
    template<class Function> void printRunTime(const char* caption, const Function& function)
    {
        printLog(Timer::getRunTime<Function>(caption, function));
    }

    template<class Function> void printRunTimeIndent(const char* caption, const Function& function, UInt indentLevel = 1, char trailing = ' ')
    {
        printLogIndent(Timer::getRunTime<Function>(caption, function), indentLevel, trailing);
    }

    template<class Function> void printRunTimeIf(const char* caption, const Function& function)
    {
        Timer timer;
        timer.tick();
        bool bResult = function();
        timer.tock();
        printLogIf(bResult, timer.getRunTime(caption));
    }

    template<class Function> void printRunTimeIndentIf(const char* caption, const Function& function, UInt indentLevel = 1, char trailing = ' ')
    {
        Timer timer;
        timer.tick();
        bool bResult = function();
        timer.tock();
        printLogIndentIf(bResult, timer.getRunTime(caption), indentLevel, trailing);
    }

    ////////////////////////////////////////////////////////////////////////////////
    void printLog(const String& s);
    void printLog(const String& s, spdlog::level::level_enum level);
    void printLogIndent(const String& s, UInt indentLevel = 1, char trailing = ' ');

    void printLogPadding(const String& s, UInt maxSize                                            = 100);
    void printLogPadding(const String& s, spdlog::level::level_enum level, UInt maxSize           = 100);
    void printLogPaddingIndent(const String& s, UInt indentLevel                                  = 1, char trailing = ' ', UInt maxSize = 100);
    void printLogPaddingIndent(const String& s, spdlog::level::level_enum level, UInt indentLevel = 1, char trailing = ' ', UInt maxSize = 100);

    void printLogIf(bool bCondition, const String& s);
    void printLogIf(bool bCondition, const String& s, spdlog::level::level_enum level);
    void printLogIndentIf(bool bCondition, const String& s, UInt indentLevel = 1, char trailing = ' ');

    void printMemoryUsage();

    ////////////////////////////////////////////////////////////////////////////////
    static auto createLogger(const String& loggerName, bool bDefaultLogPolicy = true, bool bPrint2Console = true, bool bWriteLog2File = false)
    {
        return std::make_shared<Logger>(loggerName, bDefaultLogPolicy, bPrint2Console, bWriteLog2File);
    }

    static auto createDataLogger(const String& loggerName, bool bDefaultLogPolicy = true, bool bPrint2Console = true, bool bWriteLog2File = false)
    {
        return std::make_shared<Logger>(loggerName, bDefaultLogPolicy, bPrint2Console, bWriteLog2File, true);
    }

    static void registerDataLogFile(const String& logFile) { __NT_REQUIRE(!s_bInitialized); s_DataLogFiles.push_back(logFile); }
    static void initialize(bool bPrint2Console                         = true, bool bWriteLog2File = false);
    static void initialize(const String& dataPath, bool bPrint2Console = true, bool bWriteLog2File = false);
    static void shutdown();
    static void setDataPath(const String& dataPath) { s_DataPath = dataPath; }
    static auto& mainLogger() noexcept { assert(s_MainLogger != nullptr); return *s_MainLogger; }

    static String getTotalRunTime();
    static void   signalHandler(int signum);

    ////////////////////////////////////////////////////////////////////////////////
    static inline bool s_bPrint2Console = true;
    static inline bool s_bWriteLog2File = false;
private:
    static inline bool s_bInitialized = false;
    static inline bool s_bShutdown    = false;

    static inline String            s_DataPath    = String(".");
    static inline String            s_TimeLogFile = String("time.txt");
    static inline Clock::time_point s_StartupTime {};
    static inline Clock::time_point s_ShutdownTime {};

#ifdef __NT_WINDOWS_OS__
    static inline SharedPtr<spdlog::sinks::wincolor_stdout_sink_mt> s_ConsoleSink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    static inline SharedPtr<spdlog::sinks::ansicolor_stdout_sink_mt> s_ConsoleSink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif

    static inline Vector<String>                                                  s_DataLogFiles {};
    static inline std::map<String, SharedPtr<spdlog::sinks::simple_file_sink_mt>> s_DataLogSinks {};

    static inline SharedPtr<spdlog::sinks::simple_file_sink_mt> s_SystemLogFileSink = nullptr;
    static inline SharedPtr<Logger>                             s_MainLogger        = nullptr;

    ////////////////////////////////////////////////////////////////////////////////
    SharedPtr<spdlog::logger>       m_ConsoleLogger = nullptr;
    SharedPtr<spdlog::async_logger> m_FileLogger    = nullptr;
    bool                            m_bPrint2Console;
    bool                            m_bWriteLog2File;
};