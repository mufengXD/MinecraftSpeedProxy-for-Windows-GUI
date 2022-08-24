#ifndef RC
#define RC
#include"websocket.h"
#include<process.h>
typedef struct RCPACK
{
    WS_Connection_t client;
    WS_Connection_t server;
}Rcpack;
typedef struct DATALINK
{
    void* data;
    size_t datasize;
    CRITICAL_SECTION lock;
    struct DATALINK* next;
} DataLink_t;
typedef struct SENDINGPACK
{
    char close;
    DataLink_t* head;
    CRITICAL_SECTION spinlock;
    int maxdatanum;
    int datanum;
    int target_sock;
} SendingPack_t;


struct DATAPACK
{
    void* data;
    size_t datasize;
};
#endif