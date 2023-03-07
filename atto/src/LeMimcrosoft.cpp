#include "AttoLib.h"
#include "AttoAsset.h"
#include <thread>
#include <string>
#include <windows.h>

extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace atto
{
    inline static std::string WcharToAnsi(WCHAR const* str, size_t len) {
        int const size_needed = WideCharToMultiByte(CP_UTF8, 0, str, (int)len, NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, str, (int)len, &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    void LeEngine::Win32WatchDirectory(const char* directory) {
        HANDLE hDirectory = CreateFileA(
            directory,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (hDirectory == INVALID_HANDLE_VALUE) {
            ATTOERROR("Could not open directory");
            return;
        }

        CHAR buffer[4096];
        DWORD bytesReturned;
        BOOL success;
        std::thread([hDirectory, &buffer, &bytesReturned, &success, directory, this]() {
            while (true)
            {
                success = ReadDirectoryChangesW(
                    hDirectory,
                    buffer,
                    sizeof(buffer),
                    TRUE,
                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY,
                    &bytesReturned,
                    NULL,
                    NULL
                );

                if (!success) {
                    ATTOERROR("Error: could not read directory changes")
                    break;
                }

                FILE_NOTIFY_INFORMATION* pNotifyInfo = (FILE_NOTIFY_INFORMATION*)buffer;
                do
                {
                        std::string path = directory;
                        std::string fileName = WcharToAnsi(pNotifyInfo->FileName, pNotifyInfo->FileNameLength);
                        std::string fullpath = path + fileName;

                        LargeString fullpath2 = {};
                        fullpath2.Add(fullpath.c_str());
                        fullpath2.BackSlashesToSlashes();

                        if (pNotifyInfo->Action == FILE_ACTION_ADDED) {
                            Win32OnDirectoryChanged(fullpath2.GetCStr(), DirectoryChangeType::FILE_ADDED);
                        }
                        else if (pNotifyInfo->Action == FILE_ACTION_REMOVED) {
                            Win32OnDirectoryChanged(fullpath2.GetCStr(), DirectoryChangeType::FILE_REMOVED);
                        }
                        else if (pNotifyInfo->Action == FILE_ACTION_MODIFIED) {
                            Win32OnDirectoryChanged(fullpath2.GetCStr(), DirectoryChangeType::FILE_MODIFIED);
                        }

                    pNotifyInfo = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotifyInfo + pNotifyInfo->NextEntryOffset);
                } while (pNotifyInfo->NextEntryOffset != 0);

                ZeroMemory(buffer, sizeof(buffer));
            }

            CloseHandle(hDirectory);
        }).detach();

    }
    void Application::ConsoleWrite(const char* message, u8 colour) {
        HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        // @NOTE: FATAL, ERROR, WARN, INFO, DEBUG, TRACE
        static u8 levels[6] = { 64, 4, 6, 2, 1, 8 };
        SetConsoleTextAttribute(console_handle, levels[colour]);
        OutputDebugStringA(message);
        u64 length = strlen(message);
        LPDWORD number_written = 0;
        WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),
            message,
            (DWORD)length,
            number_written,
            0
        );
        SetConsoleTextAttribute(console_handle, 8);
    }

    void Application::DisplayFatalError(const char* message) {
        MessageBeep(MB_ICONERROR);
        MessageBoxA(NULL, message, "Catastrophic Error !!! (BOOOM) ", MB_ICONERROR);
    }

    b8 Mutex::Create() {
        handle = CreateMutex(0, 0, 0);
        if (handle == nullptr) {
            ATTOERROR("Unable to create mutex.");
            return false;
        }

        return true;
    }

    void Mutex::Destroy() {
        if (handle != nullptr) {
            CloseHandle(handle);
            handle = nullptr;
        }
    }

    b8 Mutex::Lock() {
        if (handle == nullptr) {
            return false;
        }

        DWORD result = WaitForSingleObject(handle, INFINITE);
        switch (result) {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_ABANDONED:
            ATTOERROR("Mutex lock failed.");
            return false;
        }

        return true;
    }

    b8 Mutex::TryLock() {
        if (handle == nullptr) {
            return false;
        }

        DWORD result = WaitForSingleObject(handle, 0);
        switch (result) {
        case WAIT_OBJECT_0:
            return true;
        }

        return false;
    }

    b8 Mutex::Unlock() {
        if (handle == nullptr) {
            return false;
        }

        i32 result = ReleaseMutex(handle);

        return result != 0;
    }
}

