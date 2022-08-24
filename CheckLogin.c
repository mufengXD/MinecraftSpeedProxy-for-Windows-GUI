#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <process.h>
#include <windows.h>
#include "CheckLogin.h"
#include "log.h"
#define ON 1
#define OFF 0
Link_t *Whitelist_head = NULL;
Link_t *Ban_head = NULL;
HANDLE lock;
const char *Whitelist_file = "whitelist.txt";
const char *Banlist_file = "banned-players.txt";
char Whitelist_switch = OFF;
char CL_Inited = 0;
int CL_EnableWhiteList();
int CL_Check(const char *username);
int CL_DisabledWhiteList();
int CL_ReloadWhiteList();
int CL_LoadBanList();
int CL_WhiteListAdd(const char *playername);
int Save(const char *filename, Link_t *link);
int CL_WhiteListRemove(const char *playername);
int CL_BanListAdd(const char *playername);
int CL_BanListRemove(const char *playername);
__declspec(dllexport) void __cdecl CL_LockInit();
void CL_CheckInited();
void CL_CheckInited()
{
    if (CL_Inited == 0)
    {
        MessageBox(NULL, TEXT("没有初始化CheckOnline"), TEXT("ERROR"), MB_ICONERROR);
        exit(1);
    }
}

__declspec(dllexport) int CL_List(int w, FUNC_WHITELIST_HOOK func)
{
    if (func == NULL)
    {
        return -1; //传入回调函数为空
    }
    WaitForSingleObject(lock, INFINITE);
    if (w == 1) //读取白名单
    {

        if (Whitelist_switch == ON)
        {

            Link_t *temp = Whitelist_head;
            int i = 0;
            char username[512];
            char t[128];
            sprintf(t,"%d",username);
            while (temp != NULL)
            {
                i += 1;
                strcpy(username,temp->username);
                MessageBox(NULL,t,NULL,MB_OK);
                func(w,username);
                temp = temp->next;
            }
            ReleaseMutex(lock);
            return i;
        }
        else
        {
            ReleaseMutex(lock);
            return -2;
        }
    }
    else if (w == 2)
    {
        int i = 0;
        Link_t *temp = Ban_head;
        while (temp != NULL)
        {
            i += 1;
            func(w,temp->username);
            temp = temp->next;
        }
        ReleaseMutex(lock);
        return i;
    }
    else
    {
        return -3;
    }
}
__declspec(dllexport) void __cdecl CL_LockInit()
{
    lock = CreateMutex(NULL, 0, NULL);
    CL_Inited = 1;
}
int CL_Check(const char *username)
{

    WaitForSingleObject(lock, INFINITE);
    //检查是否被ban
    Link_t *temp = Ban_head;
    while (temp != NULL)
    {
        if (!strcmp(temp->username, username))
        {
            ReleaseMutex(lock);
            return CHECK_LOGIN_BANNED;
        }
        temp = temp->next;
    }
    //检查是否在白名单
    if (Whitelist_switch == ON)
    {
        temp = Whitelist_head;
        while (temp != NULL)
        {
            if (!strcmp(temp->username, username))
            {
                ReleaseMutex(lock);
                return CHECK_LOGIN_SUCCESS;
            }
            temp = temp->next;
        }
    }
    else
    {
        ReleaseMutex(lock);
        return CHECK_LOGIN_SUCCESS;
    }
    ReleaseMutex(lock);
    return CHECk_LOGIN_NOT_ON_WHITELIST;
}
int CL_EnableWhiteList()
{
    if (Whitelist_switch == ON)
        return -2;
    WaitForSingleObject(lock, INFINITE);
    //读取白名单文件
    FILE *fp = fopen(Whitelist_file, "r");
    if (fp == NULL)
        fp = fopen(Whitelist_file, "w");
    if (fp == NULL)
    {
        // printf("无法读取白名单文件,原因是:%s\n", strerror(errno));
        logout(LOG_WARN, "无法读取白名单文件,原因是:%s", strerror(errno));
        ReleaseMutex(lock);
        return -1;
    }
    Whitelist_switch = ON; //启用白名单
    char username[1024];
    while (fscanf(fp, "%s", username) > 0)
    {
        if (Whitelist_head == NULL)
        {
            Whitelist_head = (Link_t *)malloc(sizeof(Link_t));
            Whitelist_head->username = (char *)malloc(strlen(username) + 1);
            strcpy(Whitelist_head->username, username);
            Whitelist_head->next = NULL;
        }
        else
        {
            Link_t *temp = (Link_t *)malloc(sizeof(Link_t));
            temp->username = (char *)malloc(strlen(username) + 1);
            strcpy(temp->username, username);
            temp->next = Whitelist_head;
            Whitelist_head = temp;
        }
    }
    fclose(fp);
    ReleaseMutex(lock);
    return 0;
}
int CL_DisabledWhiteList()
{
    if (Whitelist_switch == OFF)
        return -2;
    WaitForSingleObject(lock, INFINITE);
    Link_t *temp = Whitelist_head;
    while (temp != NULL)
    {
        Whitelist_head = temp->next;
        free(temp->username);
        free(temp);
        temp = Whitelist_head;
    }
    Whitelist_switch = OFF;
    ReleaseMutex(lock);
    return 0;
}
int CL_ReloadWhiteList()
{
    if (Whitelist_switch == OFF)
        return -1;
    WaitForSingleObject(lock, INFINITE);
    Link_t *temp = Whitelist_head;
    while (temp != NULL)
    {
        Whitelist_head = temp->next;
        free(temp->username);
        free(temp);
        temp = Whitelist_head;
    }
    FILE *fp = fopen(Whitelist_file, "r");
    if (fp == NULL)
        fp = fopen(Whitelist_file, "w");
    if (fp == NULL)
    {
        fp = fopen(Whitelist_file, "w");
        // printf("无法读取白名单文件,原因是:%s\n", strerror(errno));
        logout(LOG_ERROR,"无法读取白名单文件,原因是:%s", strerror(errno));
        ReleaseMutex(lock);
        return -1;
    }
    char username[1024];
    while (fscanf(fp, "%s", username) > 0)
    {
        if (Whitelist_head == NULL)
        {
            Whitelist_head = (Link_t *)malloc(sizeof(Link_t));
            Whitelist_head->username = (char *)malloc(strlen(username) + 1);
            strcpy(Whitelist_head->username, username);
            Whitelist_head->next = NULL;
        }
        else
        {
            Link_t *temp = (Link_t *)malloc(sizeof(Link_t));
            temp->username = (char *)malloc(strlen(username) + 1);
            strcpy(temp->username, username);
            temp->next = Whitelist_head;
            Whitelist_head = temp;
        }
    }
    fclose(fp);
    ReleaseMutex(lock);
    return 0;
}

int CL_LoadBanList()
{
    WaitForSingleObject(lock, INFINITE);
    //卸载现有列表
    Link_t *temp = Ban_head;
    while (temp != NULL)
    {
        Ban_head = temp->next;
        free(temp->username);
        free(temp);
        temp = Ban_head;
    }
    //读取封禁列表
    FILE *fp = fopen(Banlist_file, "r");
    if (fp == NULL)
        fp = fopen(Banlist_file, "w");
    if (fp == NULL)
    {
        // printf("无法读取封禁列表,原因是:%s\n", strerror(errno));
        logout(LOG_ERROR,"无法读取封禁列表,原因是:%s", strerror(errno));
        log()
        ReleaseMutex(lock);
        return -1;
    }
    char username[1024];
    while (fscanf(fp, "%s", username) > 0)
    {
        if (Ban_head == NULL)
        {
            Ban_head = (Link_t *)malloc(sizeof(Link_t));
            Ban_head->username = (char *)malloc(strlen(username) + 1);
            strcpy(Ban_head->username, username);
            Ban_head->next = NULL;
        }
        else
        {
            Link_t *temp = (Link_t *)malloc(sizeof(Link_t));
            temp->username = (char *)malloc(strlen(username) + 1);
            strcpy(temp->username, username);
            temp->next = Ban_head;
            Ban_head = temp;
        }
    }
    fclose(fp);
    ReleaseMutex(lock);
    return 0;
}
int Save(const char *filename, Link_t *link)
{
    char name[_MAX_PATH];
    sprintf(name, ".%s", filename);
    FILE *fp = fopen(name, "w");
    if (fp == NULL)
    {
        return -1;
    }
    WaitForSingleObject(lock, INFINITE);
    Link_t *temp = link;
    while (temp != NULL)
    {
        fprintf(fp, "%s\n", temp->username);
        temp = temp->next;
    }
    fclose(fp);
    remove(filename);
    rename(name, filename);
    ReleaseMutex(lock);
    return 0;
}
int CL_WhiteListAdd(const char *playername)
{
    if (Whitelist_switch == OFF)
        return -2;
    WaitForSingleObject(lock, INFINITE);
    //检查玩家是否已经在白名单
    Link_t *temp = Whitelist_head;
    while (temp != NULL)
    {
        if (!strcmp(temp->username, playername))
        {
            ReleaseMutex(lock);
            return -1;
        }
        temp = temp->next;
    }
    temp = (Link_t *)malloc(sizeof(Link_t));
    temp->username = (char *)malloc(strlen(playername) + 1);
    strcpy(temp->username, playername);

    temp->next = Whitelist_head;
    Whitelist_head = temp;
    ReleaseMutex(lock);
    //写入文件
    if (-1 == Save(Whitelist_file, Whitelist_head))
    {
        // printf("写入白名单文件失败，原因是%s,本次更改将在程序重启后失效\n", strerror(errno));
        logout(LOG_ERROR,"写入白名单文件失败，原因是%s,本次更改将在程序重启后失效", strerror(errno));
    }
    return 0;
}
int CL_WhiteListRemove(const char *playername)
{
    if (Whitelist_switch == OFF)
        return -2;
    char successinfo = 0;
    WaitForSingleObject(lock, INFINITE);
    Link_t *temp = Whitelist_head;
    //先检查第一项
    if (!strcmp(temp->username, playername))
    {
        Whitelist_head = temp->next;
        free(temp->username);
        free(temp);
        successinfo = 1;
    }
    else
    {

        while (temp != NULL && temp->next != NULL)
        {
            if (!strcmp(temp->next->username, playername))
            {
                successinfo = 1;
                Link_t *del = temp->next;
                temp->next = del->next;
                free(del->username);
                free(del);
            }
            temp = temp->next;
        }
    }
    ReleaseMutex(lock);
    if (successinfo == 0)
    {
        return -1;
    }
    //写入文件
    if (-1 == Save(Whitelist_file, Whitelist_head))
    {
        // printf("写入白名单文件失败，原因是%s,本次更改将在程序重启后失效\n", strerror(errno));
        logout(LOG_ERROR,"写入白名单文件失败，原因是%s,本次更改将在程序重启后失效", strerror(errno));
    }
    return 0;
}
int CL_BanListAdd(const char *playername)
{
    WaitForSingleObject(lock, INFINITE);
    //检查玩家是否已经被封禁
    Link_t *temp = Ban_head;
    while (temp != NULL)
    {
        if (!strcmp(temp->username, playername))
        {
            ReleaseMutex(lock);
            return -1;
        }
        temp = temp->next;
    }
    temp = (Link_t *)malloc(sizeof(Link_t));
    temp->username = (char *)malloc(strlen(playername) + 1);
    strcpy(temp->username, playername);

    temp->next = Ban_head;
    Ban_head = temp;
    ReleaseMutex(lock);
    //写入文件
    if (-1 == Save(Banlist_file, Ban_head))
    {
        // printf("写入封禁列表文件失败，原因是%s,本次更改将在程序重启后失效\n", strerror(errno));
        logout(LOG_ERROR,"写入封禁列表文件失败，原因是%s,本次更改将在程序重启后失效", strerror(errno));
    }
    return 0;
}
int CL_BanListRemove(const char *playername)
{
    char successinfo = 0;
    WaitForSingleObject(lock, INFINITE);
    Link_t *temp = Ban_head;
    //先检查第一项
    if (!strcmp(temp->username, playername))
    {
        Ban_head = temp->next;
        free(temp->username);
        free(temp);
        successinfo = 1;
    }
    else
    {
        while (temp->next != NULL)
        {
            if (!strcmp(temp->next->username, playername))
            {
                successinfo = 1;
                Link_t *del = temp->next;
                temp->next = del->next;
                free(del->username);
                free(del);
            }
            temp = temp->next;
        }
    }
    ReleaseMutex(lock);
    if (successinfo == 0)
    {
        return -1;
    }
    //写入文件
    if (-1 == Save(Banlist_file, Ban_head))
    {
        // printf("写入封禁列表文件失败，原因是%s,本次更改将在程序重启后失效\n", strerror(errno));
        logout(LOG_ERROR,"写入封禁列表文件失败，原因是%s,本次更改将在程序重启后失效", strerror(errno));
    }
    return 0;
}