#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "websocket.h"
#include "RemoteClient.h"
#include <Windows.h>

extern char *remoteServerAddress;
extern int LocalPort;
extern int Remote_Port;

void SendingThread(void *pack);
SendingPack_t InitSending(int target_sock, int maxdata);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);

void DealRemote(void *InputArg);
void DealRemote(void *InputArg)
{
    //接收多线程传参
    Rcpack *pack = ((Rcpack *)InputArg);
    WS_Connection_t server = pack->server;
    WS_Connection_t client = pack->client;
    char data[8192];

    //直接进入接收循环
    register int rsnum; //收发的数据量
    //数据包
    SendingPack_t spack = InitSending(client.sock, 2000);
    HANDLE pid;
    pid = (HANDLE)_beginthread(SendingThread, 0, &spack);

    void *stackdata;
    DataLink_t *temp = spack.head;
    while (1)
    {
        stackdata =malloc(8192 * 4);
        rsnum = recv(server.sock, stackdata, 8192 * 4, 0);

        if (rsnum <= 0)
        {
            free(stackdata);
            LeaveCriticalSection(&(temp->lock));
            shutdown(client.sock, SD_BOTH);
            shutdown(server.sock, SD_BOTH);
            break;
        }
        temp = UpSendingData(&spack, temp, stackdata, rsnum);
        if (temp == NULL)
        {
            //另一侧连接已断开
            free(stackdata);
            shutdown(client.sock, SD_BOTH);
            shutdown(server.sock, SD_BOTH);
            break;
        }
    }
    WaitForSingleObject(pid,INFINITE);
    DeleteCriticalSection(&(spack.spinlock));
    _endthread();
}
