#include <stdlib.h>
#include "RemoteClient.h"
#include "websocket.h"

SendingPack_t InitSending(int target_sock, int maxdata);
void SendingThread(void *pack);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);
SendingPack_t InitSending(int target_sock, int maxdata)
{
    SendingPack_t pack = {0};
    pack.datanum = 0;
    pack.close = 0;
    InitializeCriticalSection(&(pack.spinlock));
    pack.head = (DataLink_t *)malloc(sizeof(DataLink_t));
    memset(pack.head,0,sizeof(DataLink_t));
    InitializeCriticalSection(&(pack.head->lock));
    EnterCriticalSection(&(pack.head->lock));
    pack.maxdatanum = maxdata;
    pack.target_sock = target_sock;
    return pack;
}
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size)
{

    EnterCriticalSection(&(pack->spinlock));
    if (pack->close == 1)
    {
        LeaveCriticalSection(&(link->lock));
        LeaveCriticalSection(&(pack->spinlock));
        return NULL;
    }
    LeaveCriticalSection(&(pack->spinlock));
    while (pack->datanum > pack->maxdatanum)
    {
        Sleep(10);
    }
    link->data = data;
    link->datasize = size;
    link->next = (DataLink_t *)malloc(sizeof(DataLink_t));
    if (link->next==NULL)
    {
        MessageBox(NULL,NULL,NULL,MB_OK);
    }
    InitializeCriticalSection(&(link->next->lock));
    link->next->data = NULL;
    EnterCriticalSection(&(link->next->lock));
    EnterCriticalSection(&(pack->spinlock));
    pack->datanum += 1;
    DataLink_t*backup=link->next;
    LeaveCriticalSection(&(link->lock));
    LeaveCriticalSection(&(pack->spinlock));
    return backup;
}

void SendingThread(void *input)
{
    SendingPack_t *pack = input;
    DataLink_t *temp = pack->head;
    DataLink_t *freebackup;
    while (1)
    {
        EnterCriticalSection(&(temp->lock));
        if (temp->data == NULL)
        {
            LeaveCriticalSection(&(temp->lock));
            DeleteCriticalSection(&(temp->lock));
            shutdown(pack->target_sock, SD_BOTH);
            free(temp);
            _endthread();
        }
        if (0 >= send(pack->target_sock, temp->data, temp->datasize, 0))
        {
            EnterCriticalSection(&(pack->spinlock));
            pack->close = 1;
            LeaveCriticalSection(&(pack->spinlock));
        }
        EnterCriticalSection(&(pack->spinlock));
        pack->datanum--;
        LeaveCriticalSection(&(pack->spinlock));
        free(temp->data);
        freebackup = temp->next;
        LeaveCriticalSection(&(temp->lock));
        DeleteCriticalSection(&(temp->lock));
        free(temp);
        temp = freebackup;
    }
}
