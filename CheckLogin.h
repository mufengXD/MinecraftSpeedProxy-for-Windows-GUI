#ifndef CHECKLOGIN
#define CHECKLOGIN
#define CHECK_LOGIN_SUCCESS 0
#define CHECk_LOGIN_NOT_ON_WHITELIST 1
#define CHECK_LOGIN_BANNED 2
typedef struct LINK {
    char* username;
    struct LINK* next;
} Link_t;
typedef void (*FUNC_WHITELIST_HOOK)(int sign, const char* str);
int CL_Check(const char* username);
__declspec(dllexport) int __cdecl CL_EnableWhiteList();
__declspec(dllexport) int __cdecl CL_DisabledWhiteList();
__declspec(dllexport) int __cdecl CL_ReloadWhiteList();
__declspec(dllexport)int CL_LoadBanList();
__declspec(dllexport) int CL_WhiteListAdd(const char* playername);
__declspec(dllexport) int CL_WhiteListRemove(const char* playername);
__declspec(dllexport) int CL_BanListAdd(const char* playername);
__declspec(dllexport) int CL_BanListRemove(const char* playername);
__declspec(dllexport) int CL_List(int w,FUNC_WHITELIST_HOOK func);
void CL_CheckInited();
__declspec(dllexport) void __cdecl CL_LockInit();
#endif