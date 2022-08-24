#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <WinSock2.h>
#include <errno.h>
#include "cJSON.h"
#include "CheckLogin.h"
#include "websocket.h"
#include <windows.h>
#include <process.h>
#include "log.h"
typedef struct ONLINE_LINK
{
    int sock;
    char ip[32];
    char username[32];
    HANDLE clientpid;
    HANDLE serverpid;
    time_t starttime;
    struct ONLINE_LINK *next;
} OL_L;
typedef void (*FUNC_ONLINE_HOOK)(const char *str);
HANDLE O_lock;
OL_L *head;
FUNC_ONLINE_HOOK List_Remove = NULL, List_Add = NULL;
extern char *Version;
int OnlineNumber = 0;
extern char OnlineLockInited;
void printonline();
void kickplayer(const char *playername);
void kicksock(int sock);
int GetOnlinePlayerNumber();
__declspec(dllexport) void __cdecl ShowPlayerInfo(const char *playername, HWND hwnd);

void __cdecl ShowPlayerInfo(const char *playername, HWND hwnd)
{
    WaitForSingleObject(O_lock, INFINITE);
    OL_L *temp = head;
    char successinfo = 0;
    while (temp != NULL)
    {
        if (!strcmp(playername, temp->username))
        {
            successinfo = 1;
            char *str = (char *)malloc(1024);
            sprintf(str, "用户名:%s\nIP:%s\n登陆时间:%s", temp->username, temp->ip, ctime(&(temp->starttime)));
            MessageBox(hwnd, str, TEXT("用户信息"), MB_OK);
        }
        temp = temp->next;
    }
    if (successinfo == 0)
    {
        MessageBox(hwnd, "该玩家不在线", "错误", MB_OK);
    }
    ReleaseMutex(O_lock);
}
__declspec(dllexport) void __cdecl InitOnline(FUNC_ONLINE_HOOK add, FUNC_ONLINE_HOOK remove)
{
    O_lock = CreateMutex(NULL, 0, NULL);
    OnlineLockInited = 1;
    if (add == NULL || remove == NULL)
    {
        MessageBox(NULL, TEXT("在线列表添加移除函数指针为NULL"), TEXT("ERROR"), MB_ICONERROR);
        exit(1);
    }
    List_Add = add;
    List_Remove = remove;
}
void kicksock(int sock)
{
    WaitForSingleObject(O_lock, INFINITE);
    OL_L *temp = head;
    char successinfo = 0;
    while (temp != NULL)
    {
        if (sock == temp->sock)
        {
            successinfo = 1;

            if (0 == shutdown(temp->sock, SD_BOTH))
            {
                printf("已关闭IP=%s,username=%s玩家的连接\n", temp->ip, temp->username);
            }
            else
            {
                printf("未能成功关闭IP=%s,username=%s玩家的连接:%s\n", temp->ip, temp->username, strerror(errno));
            }
        }
        temp = temp->next;
    }
    if (successinfo == 0)
    {
        printf("未找到%d的连接\n", sock);
    }
    ReleaseMutex(O_lock);
}
__declspec(dllexport) void __cdecl kickplayer(const char *playername)
{
    WaitForSingleObject(O_lock, INFINITE);
    OL_L *temp = head;
    char successinfo = 0;
    while (temp != NULL)
    {
        if (!strcmp(playername, temp->username))
        {
            successinfo = 1;
            if (0 == shutdown(temp->sock, SD_BOTH))
            {
                logout(LOG_PLAYER, "已关闭IP=%s,username=%s玩家的连接\n", temp->ip, temp->username);
            }
            else
            {
                logout(LOG_PLAYER, "未能成功关闭IP=%s,username=%s玩家的连接:%s\n", temp->ip, temp->username, strerror(errno));
            }
        }
        temp = temp->next;
    }
    if (successinfo == 0)
    {
        logout(LOG_PLAYER, "未找到%s的连接\n", playername);
    }
    ReleaseMutex(O_lock);
}

void printonline()
{
    printf("==========开始列出在线玩家==========\n");

    WaitForSingleObject(O_lock, INFINITE);
    OL_L *temp = head;
    char str[20];
    printf("ID\tsock\tIP地址\t用户名\t时间\n");
    int i = 1;
    while (temp)
    {
        printf("%d\t%d\t%s\t%s\t%s", i, temp->sock, temp->ip, temp->username, ctime(&(temp->starttime)));
        temp = temp->next;
        i += 1;
    }
    ReleaseMutex(O_lock);
    printf("==========列出在线玩家完成==========\n");
}

void removeip(int sock)
{
    WaitForSingleObject(O_lock, INFINITE);
    if (head == NULL)
    {
        //当前没有在线玩家
        ReleaseMutex(O_lock);
        // printf("[W] 试图移除非在线玩家\n");
        // log_warn("试图移除非在线玩家");
        logout(LOG_WARN, "试图移除非在线玩家");
        return;
    }
    else
    {
        OL_L *temp = head;
        if (temp->sock == sock)
        {

            if (strcmp(temp->username, "Not-Login") != 0)
            {
                OnlineNumber -= 1;
                logout(LOG_PLAYER, "移除了玩家:%s于IP:%s的连接", temp->username, temp->ip);
                List_Remove(temp->username);
            }
            head = temp->next;
            free(temp);

            ReleaseMutex(O_lock);
            return;
        }
        while (temp->next)
        {
            if (temp->next->sock == sock)
            {

                OL_L *a = temp->next;
                if (strcmp(temp->username, "Not-Login") != 0)
                {
                    OnlineNumber -= 1;
                    logout(LOG_PLAYER, "移除了玩家:%s于IP:%s的连接", a->username, a->ip);
                    List_Remove(a->username);
                }
                temp->next = a->next;
                free(a);
                ReleaseMutex(O_lock);

                return;
            }
            temp = temp->next;
        }
        ReleaseMutex(O_lock);
        return;
    }
}
void adduser(int sock, const char *username)
{
    WaitForSingleObject(O_lock, INFINITE);

    OL_L *temp = head;
    while (temp)
    {
        if (temp->sock == sock)
        {
            strcpy(temp->username, username);
            OnlineNumber += 1;
            logout(LOG_PLAYER, "IP:%s 玩家%s登录到服务器", temp->ip, username);
            List_Add(username);
            ReleaseMutex(O_lock);
            return;
        }
        temp = temp->next;
    }
    ////////////////
    ReleaseMutex(O_lock);
    return;
}
void addip(int sock, const char *IP)
{
    WaitForSingleObject(O_lock, INFINITE);
    if (head == NULL)
    {
        //当前没有在线玩家
        head = (OL_L *)malloc(sizeof(OL_L));
        strcpy(head->ip, IP);
        strcpy(head->username, "Not-Login");
        head->starttime = time(NULL);
        head->sock = sock;
        head->next = NULL;
        ReleaseMutex(O_lock);
        return;
    }
    else
    {
        OL_L *temp = head;
        while (temp)
        {
            if (temp->sock == sock)
            {
                logout(LOG_WARN, "重复的套接字添加,sock=%d,IP=%s\n", sock, IP);
                ReleaseMutex(O_lock);
                return;
            }
            temp = temp->next;
        }
        temp = head;
        temp = (OL_L *)malloc(sizeof(OL_L));
        strcpy(temp->ip, IP);
        strcpy(temp->username, "Not-Login");
        temp->sock = sock;
        temp->starttime = time(NULL);

        temp->next = head;
        head = temp;
        ReleaseMutex(O_lock);
        return;
    }
}

int GetOnlinePlayerNumber()
{
    return OnlineNumber;
}