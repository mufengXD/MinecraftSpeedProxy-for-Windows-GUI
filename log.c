#include "log.h"
int DAY = 0;
int LOG_inited=0;
FILE* LogFile = NULL;
TimeStr gettime(void)
{
    time_t rawtime;
    struct tm* info;
    char buffer[80];

    time(&rawtime);

    info = localtime(&rawtime);
    TimeStr surstr;
    strftime(surstr.time, 128, "%Y-%m-%d %H:%M:%S", info);
    //printf("[%s]", buffer );
    return surstr;
}

FUNC_LOG_HOOK log_hook;
void Log_CheckInit();
void Log_CheckInit()
{
    if (LOG_inited==0)
    {
        MessageBox(NULL,TEXT("日志没有初始化\n"),TEXT("ERROR"),MB_ICONERROR);
        exit(1);
    }
}

__declspec(dllexport) int __cdecl InitLog(FUNC_LOG_HOOK func)
{
	log_hook = func;
    LOG_inited=1;
}
void logout(int level, const char* cmd, ...)
{
	char* ptr = (char*)malloc(1024);
	char* temp = ptr + 512;
	va_list args;			  //定义一个va_list类型的变量，用来储存单个参数
	va_start(args, cmd);	  //使args指向可变参数的第一个参数
	vsprintf(ptr, cmd, args); //必须用vprintf等带V的
	va_end(args);			  //结束可变参数的获取
	switch (level)
	{
	case LOG_INFO:
		sprintf(temp, "[%s] [INFO] %s", gettime().time, ptr);
		break;
	case LOG_WARN:
		sprintf(temp, "[%s] [WARN] %s", gettime().time, ptr);
		break;
	case LOG_ERROR:
		sprintf(temp, "[%s] [ERROR] %s", gettime().time, ptr);
		break;
	case LOG_PLAYER:
		sprintf(temp, "[%s] [PLAYER] %s", gettime().time, ptr);
        break;
	default:
		sprintf(temp, "[%s] %s", gettime().time, ptr);
        break;
	}
	log_hook(temp);
	free(ptr);
}

