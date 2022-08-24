#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "websocket.h"
#include "RemoteClient.h"
#include "unpack.h"
#include "cJSON.h"
#include "CheckLogin.h"
#include <process.h>
#include <Windows.h>
#include "log.h"
extern char *remoteServerAddress;
extern int LocalPort;
extern int Remote_Port;
extern char *jdata;

void DealClient(void *InputArg);
void adduser(int sock, const char *username);
void DealRemote(void *InputArg);
void removeip(int sock);
int SendResponse(WS_Connection_t client, char *jdata, int pro);
int SendPong(WS_Connection_t client, char *data, int datasize);
// int chick(char *name);
int CL_Check(const char *username);
void SendKick(WS_Connection_t client, const char *reason);

void SendingThread(void *pack);
SendingPack_t InitSending(int target_sock, int maxdata);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);

void DealClient(void *InputArg)
{
    //接收多线程传参
    WS_Connection_t client = *((WS_Connection_t *)InputArg);
    free(InputArg);
    WS_Connection_t remoteserver;

    int opt = 6, err = 0;

    //将双方服务器套接字打包
    Rcpack *pack = (Rcpack *)malloc(sizeof(Rcpack));
    // char remotedata[8192];
    //拉起与服务端连接的线程
    HANDLE pid;
    //进入循环接收客户端数据数据
    int rsnum;       //收发的数据量
    char data[8192]; //数据包
    //首先接收一个包并且看包ID
    char handshaked = 0;
    char logined = 0;
    int statusmode = 0;
    char *handpackdata = (char *)malloc(4096);
    char connected = 0;
    int handpacksize;
    HandPack hp;
    int cancelid;
    void *stackdata;
    SendingPack_t spack;
    HANDLE sendingthread;
    while (1)
    {

        if (logined == 0)
        {
            //还没有登录
            if (handshaked == 0)
            {
                //还没有握手,先握手
                memset(data, 0, 4096);
                int handshakepacksize = RecvFullPack(client, data, 4096);

                if (handshakepacksize <= 0)
                {
                    // printf("[%s] [%lu] [W] 握手包错误，无法完成握手\n", gettime().time, pthread_self());
                    // log_warn("[%lu] 握手包错误，无法完成握手", pthread_self());
                    goto CLOSECONNECT;
                }
                if (GetPackID(data, handshakepacksize) != 0)
                {
                    // printf("[%s] [%lu] [W] 不是握手包，无法完成握手\n", gettime().time, pthread_self());
                    // log_warn("[%lu] 不是握手包，无法完成握手", pthread_self());
                    goto CLOSECONNECT;
                }
                if (0 != ParseHandlePack(&hp, data, handshakepacksize))
                {
                    // printf("[%s] [%lu] [W] 握手包无法解析\n", gettime().time, pthread_self());
                    // log_warn("[%lu] 握手包无法解析", pthread_self());
                    goto CLOSECONNECT;
                }
                // printf("[%s] [I] %s连接到服务器\n", gettime().time, client.addr);
                // log_info("%s连接到服务器", client.addr);
                if (hp.nextstate == 1)
                {
                    statusmode = 1; // status连接
                }
                else
                {
                    statusmode = 0; // login连接
                }
                handshaked = 1; //将连接设置为已握手状态
                continue;
            }
            else
            {
                //已握手，但是并没有完成登录，可能是status或者还没有login
                memset(data, 0, 4096);
                int datasize = RecvFullPack(client, data, 4096);
                if (datasize <= 0)
                {
                    goto CLOSECONNECT;
                }
                switch (GetPackID(data, datasize))
                {
                case -10000:
                    goto CLOSECONNECT;
                    break;
                case 0:
                    //登录或者request包
                    if (statusmode == 1)
                    {
                        // request包
                        int i = SendResponse(client, jdata, hp.protocol);
                        continue;
                    }
                    else
                    {
                        //登录包
                        char vil;
                        varint_decode(data, datasize, &vil);
                        char username[20];
                        ReadString(data + vil + 1, datasize - vil, username, 20);
                        switch (CL_Check(username))
                        {

                        case CHECK_LOGIN_SUCCESS:
                            if (0 != WS_ConnectServer(remoteServerAddress, Remote_Port, &remoteserver))
                            {
                                // printf("[%s] [E] 无法连接远程服务器，原因是%s\n", gettime().time, strerror(errno));
                                // log_error("无法连接远程服务器，原因是%s", strerror(errno));
                                WS_CloseConnection(&client);
                                removeip(client.sock);
                                free(handpackdata);
                                free(pack);
                                _endthread();
                            }
                            pack->client = client;
                            pack->server = remoteserver;
                            pid = (HANDLE)_beginthread(DealRemote, 0, pack);
                            strcpy(hp.Address, remoteServerAddress);
                            hp.port = Remote_Port;
                            hp.nextstate = 2;
                            char *message = (char *)malloc(128);
                            int packsize = BuildHandPack(hp, remoteServerAddress, message, 128);
                            WS_Send(&remoteserver, message, packsize);
                            WS_Send(&remoteserver, data, datasize);
                            free(message);
                            adduser(client.sock, username);
                            goto ACCEPTWHILE;

                        case CHECK_LOGIN_BANNED:
                            logout(LOG_INFO, "%s尝试连接到服务器，但是其位于黑名单中", username);
                            SendKick(client, "You are banned by proxy server");
                            goto CLOSECONNECT;
                        case CHECk_LOGIN_NOT_ON_WHITELIST:
                            logout(LOG_INFO, "%s尝试连接到服务器，但是其不在白名单中", username);
                            SendKick(client, "You are not white-listed on this server");
                            goto CLOSECONNECT;
                        default:
                            logout(LOG_INFO, "%s尝试连接到服务器时出现未知错误", username);
                            SendKick(client, "Unknown Error");
                            goto CLOSECONNECT;
                        }
                    }
                case 1:
                    // ping包
                    SendPong(client, data, datasize);
                    continue;
                default:
                    continue;
                }
            }
        }

    ACCEPTWHILE:
        spack = InitSending(remoteserver.sock, 2000);
        sendingthread = (HANDLE)_beginthread(SendingThread, 0, &spack);
        DataLink_t *temp = spack.head;

        while (1)
        {

            stackdata = malloc(512);
            rsnum = recv(client.sock, stackdata, 512, 0);
            if (rsnum <= 0)
            {
                free(stackdata);
                LeaveCriticalSection(&(temp->lock));
                shutdown(client.sock, SD_BOTH);
                shutdown(remoteserver.sock, SD_BOTH);
                break;
            }
            temp = UpSendingData(&spack, temp, stackdata, rsnum);
            if (temp == NULL)
            {
                //另一侧连接已断开
                free(stackdata);
                shutdown(client.sock, SD_BOTH);
                shutdown(remoteserver.sock, SD_BOTH);
                break;
            }
        }
        WaitForSingleObject(sendingthread, INFINITE);
        WaitForSingleObject(pid, INFINITE); //等待服务线程资源回收
        WS_CloseConnection(&remoteserver);
        DeleteCriticalSection(&(spack.spinlock));
    }
CLOSECONNECT:
    //断开连接并且回收资源
    WS_CloseConnection(&client);
    free(handpackdata);
    free(pack);
    removeip(client.sock);
    // printf("[%s] [I] %s断开连接\n", gettime().time, client.addr);
    // log_info("%s断开连接", client.addr);
    _endthread();
}

int SendPong(WS_Connection_t client, char *data, int datasize)
{
    return WS_Send(&client, data, datasize);
}

int SendResponse(WS_Connection_t client, char *jdata, int pro)
{

    cJSON *json = cJSON_Parse(jdata);
    if (json == NULL)
    {
        //数据格式错误!
        // printf("[%s] [W] motd.json数据格式错误\n", gettime().time);
        // log_warn("motd.json数据格式错误");
        return -3;
    }
    cJSON *version = cJSON_GetObjectItem(json, "version");
    if (version == NULL)
    {
        cJSON_Delete(json);
        //数据格式错误!
        // printf("[%s] [W] motd.json数据格式错误\n", gettime().time);
        // log_warn("motd.json数据格式错误");
        return -3;
    }
    cJSON *temp = cJSON_CreateNumber(pro);
    cJSON_ReplaceItemInObject(version, "protocol", temp);
    char *jjdata = cJSON_Print(json);
    cJSON_Delete(json);
    char *data = (char *)malloc(strlen(jjdata) + 32);
    int datapacksize = BuildString(jjdata, data, strlen(jjdata) + 32);
    if (datapacksize <= 0)
    {
        free(jjdata);
        return -1;
    }
    char vil;
    char vi[10];
    varint_encode(datapacksize + 1, vi, 10, &vil);
    char *message = (char *)malloc(strlen(jjdata) + 32);
    free(jjdata);
    char *p = message;
    memcpy(message, vi, vil);
    p += vil;
    *p = 0;
    p += 1;
    memcpy(p, data, datapacksize);
    free(data);
    int sendnum = WS_Send(&client, message, datapacksize + vil + 1);
    if (sendnum <= 0)
    {
        free(message);
        return -2;
    }
    free(message);
    return 0;
}
void SendKick(WS_Connection_t client, const char *reason)
{
    //创建json包

    char *str = (char *)malloc(strlen(reason) + 10);
    sprintf(str, "\"%s\"", reason);

    char *data = (char *)malloc(strlen(str) + 20);
    int strl = BuildString(str, data, strlen(str) + 20);
    char *message = (char *)malloc(strlen(str) + 30);
    char vil;
    char vi[10];
    varint_encode(strl + 1, vi, 10, &vil);
    memcpy(message, vi, vil);
    char *p = message + vil;
    *p = 0;
    p += 1;
    memcpy(p, data, strl);
    WS_Send(&client, message, strl + vil + 1);
    free(data);
    free(str);
    free(message);
}
