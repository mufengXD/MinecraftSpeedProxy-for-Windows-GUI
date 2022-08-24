#include <stdio.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "websocket.h"
#include "cJSON.h"
#include "log.h"
#include "CheckLogin.h"
#include <Windows.h>

// char *Version;
char *remoteServerAddress;
void Server(void *a);
// const char *Defalut_Configfile_Path = "/etc/minecraftspeedproxy/config.json";
// const char *Config_Script_Command = "bash <(curl -fsSL https://fastly.jsdelivr.net/gh/AllesUgo/Minecraft-Speed-Proxy@master/scripts/config.sh )";
int LocalPort;
int Remote_Port;
char *jdata;
int Run;
char Motd_Loaded = 0;
HANDLE Main_Pid;
WS_ServerPort_t server;

char OnlineLockInited = 0;
void OnlineControl_Init();
void DealClient(void *InputArg);
void addip(int sock, const char *IP);
void sighandle(int sig);
void printhelp(void);
int ReadConfig(const char *filepath, char **sremoteserveraddress, int *remoteport, int *localport, char *noinput_sign);
//开始
void logout(int level, const char *cmd, ...);
__declspec(dllexport) int __cdecl StartServer(const char *serveraddr, int remoteport, int localport);
__declspec(dllexport) int __cdecl StopServer();
__declspec(dllexport) int __cdecl StopServer()
{
	if (Run == 1)
		Run = 0;
	else
		return -1;
	WS_CloseServer(server);
	WaitForSingleObject(Main_Pid, INFINITE);
	return 0;
}

__declspec(dllexport) int __cdecl StartServer(const char *serveraddr, int remoteport, int localport)
{
	if (Run == 1)
		return 1;
	if (OnlineLockInited == 0)
	{
		MessageBox(NULL, TEXT("没有初始化OnlineLock"), TEXT("ERROR"), MB_ICONERROR);
		exit(1);
	}
	CL_CheckInited();
	Log_CheckInit();
	remoteServerAddress = (char *)malloc(strlen(serveraddr) + 1);
	strcpy(remoteServerAddress, serveraddr);
	Remote_Port = remoteport;
	LocalPort = localport;
	fflush(stdout);
	Main_Pid = (HANDLE)_beginthread(Server, 0, NULL);
	return 0;
}

void Server(void *a)
{

	if (Run == 1)
	{
		_endthread();
	}

	Run = 1;
	// log_info("PID:%d 远程服务器:%s:%d 本地监听端口%d", getpid(), remoteServerAddress, Remote_Port, LocalPort);
	//读取motd
	//  printf("[%s] [I] 加载motd数据\n", gettime().time);

	if (!Motd_Loaded)
	{
		logout(LOG_INFO, "加载motd数据");
		FILE *fp = fopen("motd.json", "r");
		if (fp == NULL)
		{
			// printf("[%s] [W] 没有找到motd.json\n", gettime().time);
			logout(LOG_WARN, "没有找到motd.json");
			jdata = (char *)malloc(141);
			strcpy(jdata, "{\"version\": {\"name\": \"1.8.7\",\"protocol\": 47},\"players\": {\"max\": 0,\"online\": 0,\"sample\": []},\"description\": {\"text\": \"Minecraft Speed Plus\"}}");
			fp = fopen("motd.json", "w");
			if (fp == NULL)
			{
				// printf("[%s] [W] 无法保存motd.json文件，原因是:%s\n", gettime().time, strerror(errno));
				logout(LOG_WARN, "无法保存motd.json文件，原因是:%s", strerror(errno));
			}
			else
			{
				fputs(jdata, fp);
				fclose(fp);
				// printf("[%s] [I] 已生成默认的motd.json\n", gettime().time);
				logout(LOG_INFO, "已生成默认的motd.json\n");
			}
		}
		else
		{
			int filesize;
			fseek(fp, 0L, SEEK_END);
			filesize = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
			jdata = (char *)malloc(filesize);
			fread(jdata, filesize, 1, fp);
			fclose(fp);
		}
		Motd_Loaded = 1;
	}
	else
	{
		logout(LOG_INFO, "motd已加载");
	}
	//创建监听端口
	// printf("[%s] [I] 初始化服务端口\n", gettime().time);
	logout(LOG_INFO,"初始化本地服务端口%d",LocalPort);
	server = WS_CreateServerPort(LocalPort, 5);
	if (server == 0)
	{

		// printf("[%s] [E] 绑定端口%d失败，端口已被占用\n", gettime().time, LocalPort);
		logout(LOG_ERROR, "绑定端口%d失败，端口可能已被占用", LocalPort);
		_endthread();
	}
	//循环等待用户连接
	WS_Connection_t *client;
	// printf("[%s] [I] 加载完成，等待连接，输入help获取帮助\n", gettime().time);
	logout(LOG_INFO, "加载完成，等待连接");
	while (Run)
	{
		client = (WS_Connection_t *)malloc(sizeof(WS_Connection_t));
		if (0 > WS_WaitClient(server, client))
		{
			// MessageBox(NULL,TEXT("到if里面了"),NULL,MB_OK);

			free(client);
			if (Run == 0)
			{
				break;
			}
			//建立连接失败
			// printf("[%s] [W] 连接建立失败:%s\n", gettime().time, strerror(errno));
			logout(LOG_WARN, "连接建立失败:%s", strerror(errno));
			continue;
		}
		// MessageBox(NULL,TEXT("到if外面了"),NULL,MB_OK);
		addip(client->sock, client->addr);
		_beginthread(DealClient, 0, client);
	}

	_endthread();
}
