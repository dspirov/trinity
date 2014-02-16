#include "net.h"
#include "cxxptl_sdl.h"
#include <iostream>
using namespace std;

#define RT_PORT 9000

inline void writeToBuffer(const Color &col, char* buff)
{
    for(int i = 0 ; i < 3 ; i++)
    {
        // Encoding floats is hard. This is an ugly solution, but it works.
        Sint32 c = col.components[i] * 10000;
        SDLNet_Write32(c, buff + i*4);
    }
}

inline void readFromBuffer(char* buff, Color& col)
{
    for(int i = 0 ; i < 3 ; i++)
    {
        Sint32 c = SDLNet_Read32(buff + 4*i);
        col.components[i] = (float)c / 10000;
    }
}

ClientSocket::ClientSocket(const char* host, const char* sceneFile)
{
    sock = NULL; remoteThreads = 0;
    strncpy(this->sceneFile, sceneFile, NET_MAX_FILENAME);
    if(SDLNet_ResolveHost(&ip, host, RT_PORT) != 0)
    { cerr<<SDLNet_GetError()<<endl; return; }
    sock = SDLNet_TCP_Open(&ip);
    if(!sock)
    { cerr<<SDLNet_GetError()<<endl; return; }
    if(SDLNet_TCP_Send(sock, this->sceneFile, NET_MAX_FILENAME) < NET_MAX_FILENAME)
    { cerr<<SDLNet_GetError()<<endl; return; }
    Sint32 tempThreads = 0;
    if(SDLNet_TCP_Recv(sock, &tempThreads, 4) <= 0)
    { cerr<<SDLNet_GetError()<<endl; return; }
    remoteThreads = SDLNet_Read32(&tempThreads);
}

ClientSocket::~ClientSocket()
{
	SDLNet_TCP_Close(sock);
}

int ClientSocket::getRemoteThreads() const
{
    return remoteThreads;
}

bool sendRect(Rect r, TCPsocket sock)
{
    Sint32 data[4];
    SDLNet_Write32(r.x0, &data[0]);
    SDLNet_Write32(r.y0, &data[1]);
    SDLNet_Write32(r.x1, &data[2]);
    SDLNet_Write32(r.y1, &data[3]);
    if(SDLNet_TCP_Send(sock, data, 16) < 16)
    {
        cerr<<"Failed to send Rect"<<endl;
        return false;
    }
    return true;
}

bool ClientSocket::requestBucket(Rect bucket)
{
    return sendRect(bucket, sock);
}

Rect receiveRect(TCPsocket& sock)
{
    Sint32 data[4];
    if(SDLNet_TCP_Recv(sock, data, 16) < 16)
    {
        cerr<<"Failed to read Rect: "<<SDLNet_GetError()<<endl;
        return Rect(0,0,0,0);
    }
    Rect r(SDLNet_Read32(&data[0]),
            SDLNet_Read32(&data[1]),
            SDLNet_Read32(&data[2]),
            SDLNet_Read32(&data[3]));
    if(r.x0 > VFB_MAX_SIZE || r.y0 > VFB_MAX_SIZE)
    {
        cerr<<"Received bad Rect! "<<r.x0<<", "<<r.y0<<endl;
        exit(1);
    }
    return r;
}

bool ClientSocket::receiveBucket(Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE])
{
    Rect bucket = receiveRect(sock);
    if(bucket.w == 0)
        return false;
    // receive the pixels
    char *result = new char[bucket.w * bucket.h * 12];
    char *pRead = (char*)result;
    int status = 0, totalRead = 0;
    while(totalRead < (int)(bucket.w * bucket.h * 12))
    {
        status = SDLNet_TCP_Recv(sock, pRead, bucket.w * bucket.h * 12 - totalRead);
        //printf("Received %i bytes\n", status);
        if(status <= 0)
        {
            cout<<"Failed to receive bucket: "<<status<<SDLNet_GetError()<<endl;
            return false;
        }
        totalRead += status;
        pRead += status;
    }
    for(int y = 0 ; y < bucket.h ; y++)
    {
        for(int x = 0 ; x < bucket.w ; x++)
        {
            readFromBuffer(&result[(bucket.w * y + x)*12], vfb[bucket.y0 + y][bucket.x0 + x]);
        }
    }
    delete[] result;
    displayVFBRect(bucket, vfb);
    return true;
}

ServerSocket::ServerSocket()
{
    IPaddress ip;
    if(SDLNet_ResolveHost(&ip, NULL, RT_PORT) != 0)
        cerr<<SDLNet_GetError()<<endl;
    srvSock = SDLNet_TCP_Open(&ip);
    if(!srvSock)
        cerr<<SDLNet_GetError();
    sock = NULL;
}

ServerSocket::~ServerSocket()
{
    SDLNet_TCP_Close(srvSock);
    if(sock)
        SDLNet_TCP_Close(sock);
}

bool ServerSocket::acceptConnection()
{
    sock = SDLNet_TCP_Accept(srvSock);
    if(!sock)
        return false;
    if(SDLNet_TCP_Recv(sock, this->sceneFile, NET_MAX_FILENAME) <= NET_MAX_FILENAME)
        cerr<<SDLNet_GetError();
    Sint32 numThreads = 0;
    SDLNet_Write32(get_processor_count(), &numThreads);
    if(SDLNet_TCP_Send(sock, &numThreads, 4) < 4)
        cerr<<SDLNet_GetError();
    return true;
}

const char* ServerSocket::getSceneFile() const
{
    return sceneFile;
}

Rect ServerSocket::waitForBucket()
{
    return receiveRect(sock);
}

bool ServerSocket::returnBucket(Rect bucket, const Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE])
{
    if(!sendRect(bucket, sock))
        return false;
    char *buffer = new char[bucket.w * bucket.h * 12];
    char *p = buffer;
    for(int y = bucket.y0 ; y < bucket.y1 ; y++)
    {
        for(int x = bucket.x0 ; x < bucket.x1 ; x++)
        {
            writeToBuffer(vfb[y][x], p);
            p+= 12;
        }
    }
    int status = SDLNet_TCP_Send(sock, buffer, bucket.w * bucket.h * 12);
    delete[] buffer;
    if(status == (int)(bucket.w * bucket.h * 12))
        return true;
    cerr<<status;
    return false;
}
