#ifndef NET_H
#define NET_H

#include <SDL/SDL_net.h>
#include "sdl.h"

#define NET_MAX_FILENAME 100

class ClientSocket {
private:
    IPaddress ip;
    char sceneFile[NET_MAX_FILENAME];
    TCPsocket sock;
    int remoteThreads;
public:
    ClientSocket(const char* host, const char* sceneFile);
    ~ClientSocket();

    int getRemoteThreads() const;
    bool requestBucket(Rect bucket);
    bool receiveBucket(Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]);
};

class ServerSocket {
private:
    char sceneFile[NET_MAX_FILENAME];
    TCPsocket sock;
    TCPsocket srvSock;
public:
    ServerSocket();
    ~ServerSocket();

    bool acceptConnection();

    const char* getSceneFile() const;

    Rect waitForBucket();
    bool returnBucket(Rect bucket, const Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]);
    void close();
};

#endif // NET_H
