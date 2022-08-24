#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <windows.h>
typedef struct TIMESTR {
    char time[128];
} TimeStr;
typedef void (*FUNC_LOG_HOOK)(const char* str);
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3
#define LOG_PLAYER 4

TimeStr gettime(void);
void logout(int level, const char* cmd, ...);
__declspec(dllexport) int __cdecl InitLog(FUNC_LOG_HOOK func);
void Log_CheckInit();
