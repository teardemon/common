#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "../base/portable.h"
#include "../base/singleton.h"
#include "../base/eztimer.h"
#include "event.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace net
{
  class ezEventLoop;
  struct ezMsg;
  class ezConnection;
  class ezIMessagePusher;
  class ezIMessagePuller;
  class ezIoThread;
  class ezClientFd;
  class ezBuffer;

  class ezIDecoder
  {
  public:
    virtual ~ezIDecoder(){}
    virtual int Decode(ezIMessagePusher* pusher,char* buf,size_t s)=0;
  };

  class ezIEncoder
  {
  public:
    virtual ~ezIEncoder(){}
    virtual bool Encode(ezIMessagePuller* puller,ezBuffer* buff)=0;
  };

  class ezIConnnectionHander
  {
  public:
    virtual void OnOpen(ezConnection* conn)=0;
    virtual void OnClose(ezConnection* conn)=0;
    virtual void OnData(ezConnection* conn,ezMsg* msg)=0;
  };

  class ezServerHander:public ezIConnnectionHander
  {
  public:
    virtual void OnOpen(ezConnection* conn);
    virtual void OnClose(ezConnection* conn);
    virtual void OnData(ezConnection* conn,ezMsg* msg);
  };

  class ezClientHander:public ezIConnnectionHander
  {
  public:
    virtual void OnOpen(ezConnection* conn);
    virtual void OnClose(ezConnection* conn);
    virtual void OnData(ezConnection* conn,ezMsg* msg);
  };

  class ezMsgDecoder:public ezIDecoder
  {
  public:
    explicit ezMsgDecoder(uint16_t maxsize):maxMsgSize_(maxsize){}
    virtual int Decode(ezIMessagePusher* pusher,char* buf,size_t s);
  private:
    uint16_t maxMsgSize_;
  };

  class ezMsgEncoder:public ezIEncoder
  {
  public:
    virtual bool Encode(ezIMessagePuller* puller,ezBuffer* buffer);
  };

  class ezGameObject
  {
  public:
    ezGameObject();
    virtual ~ezGameObject();
    void SetConnection(ezConnection* conn) {conn_=conn;}
    ezConnection* GetConnection(){return conn_;}
    void SendNetpack(ezMsg& msg);
    virtual void Close();
  private:
    ezConnection* conn_;
  };

  class ezConnection:public ezThreadEventHander
  {
  public:
    ezConnection(ezEventLoop* looper,ezClientFd* client,int tid,int64_t userdata);
    virtual ~ezConnection();
    void AttachGameObject(ezGameObject* obj);
    void DetachGameObject();
    ezGameObject* GetGameObject(){return gameObj_;}
    void OnRecvNetPack(ezMsg* pack);
    void ActiveClose();
    void SetIpAddr(const char* ip){ip_=ip;}
    const std::string& GetIpAddr() {return ip_;}
    int64_t GetUserdata();
    void SendMsg(ezMsg& msg);
    bool RecvMsg(ezMsg& msg);
    virtual void ProcessEvent(ezThreadEvent& ev);
  private:
    void CloseClient();
  private:
    ezClientFd* client_;
    std::string ip_;
    ezGameObject* gameObj_;
    int64_t userdata_;
  };
}
#endif