#include "socket.h"
#include "connection.h"
#include "netpack.h"
#include "event.h"
#include "../base/memorystream.h"
#include "../base/logging.h"

using namespace net;

net::ezConnection::ezConnection(ezEventLoop* looper)
	:evLooper_(looper),
	fd_(0),
	uuid_(0),
	gameObj_(nullptr),
	netpackHander_(nullptr)
{
}

net::ezConnection::ezConnection(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
	:evLooper_(looper),
	fd_(fd),
	uuid_(uuid),
  tid_(tid),
	gameObj_(nullptr),
	netpackHander_(nullptr)
{
}

net::ezConnection::~ezConnection()
{
	detachGameObject();
}

void net::ezConnection::attachGameObject(ezGameObject* obj)
{
	gameObj_=obj;
	if(gameObj_)
		gameObj_->setConnection(this);
}

void net::ezConnection::detachGameObject()
{
	if(gameObj_)
		gameObj_->setConnection(nullptr);
	gameObj_=nullptr;
}

void net::ezConnection::onRecvNetPack(ezMsg* pack)
{
	if(netpackHander_)
		netpackHander_->process(this,pack);
}

void net::ezConnection::sendNetPack(ezMsg* pack)
{
	assert(pack);
	assert(pack->size_>0);
	evLooper_->sendMsg(tid_,fd_,*pack);
}

void net::ezConnection::Close()
{
  evLooper_->o2nCloseFd(tid_,fd_,uuid_);
}

void net::ezConnection::setHander(ezNetPackHander* hander)
{
	netpackHander_=hander;
}

net::ezConnection* net::ezConnectionMgr::addConnection(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	assert(defaultHander_);
	ezConnection* conn=new ezConnection(looper,fd,uuid,tid);
	mapConns_[uuid]=conn;
	conn->setHander(defaultHander_);
	return conn;
}

void net::ezConnectionMgr::delConnection(uint64_t uuid)
{
	auto iter=mapConns_.find(uuid);
	if(iter!=mapConns_.end())
	{
		delete iter->second;
		mapConns_.erase(iter);
	}
}

net::ezConnection* net::ezConnectionMgr::findConnection(uint64_t uuid)
{
	auto iter=mapConns_.find(uuid);
	if(iter!=mapConns_.end())
		return iter->second;
	else
		return nullptr;
}

net::ezConnectToInfo* net::ezConnectionMgr::findConnectToInfo(uint64_t uuid)
{
	for(size_t s=0;s<vecConnectTo_.size();++s)
	{
		ezConnectToInfo& info=vecConnectTo_[s];
		if(info.uuid_==uuid)
			return &info;
	}
	return nullptr;
}

void net::ezConnectionMgr::delConnectToInfo(uint64_t uuid)
{
	for(auto iter=vecConnectTo_.begin();iter!=vecConnectTo_.end();++iter)
	{
		ezConnectToInfo& info=*iter;
		if(info.uuid_==uuid)
		{
			vecConnectTo_.erase(iter);
			return;
		}
	}
}

uint64_t net::ezConnectionMgr::connectTo(ezEventLoop* looper,const char* ip,int port)
{
	uint64_t uuid=ezUUID::instance()->uuid();
	looper->o2nConnectTo(uuid,ip,port);
	vecConnectTo_.push_back(ezConnectToInfo(uuid,ip,port));
	return uuid;
}

void net::ezConnectionMgr::reconnectAll()
{
  for(size_t s=0;s<vecConnectTo_.size();++s)
  {
    ezConnectToInfo& info=vecConnectTo_[s];
    if(!info.connectOK_)
      looper_->o2nConnectTo(info.uuid_,info.ip_.c_str(),info.port_);
  }
}

void net::ezServerHander::onOpen(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	ezConnection* conn=looper->getConnectionMgr()->addConnection(looper,fd,uuid,tid);
	if(conn)
	{
		char ipport[128];
		net::ToIpPort(ipport,sizeof(ipport),net::GetPeerAddr(fd));
		conn->setIpAddr(ipport);
		LOG_INFO("new connector from %s,tid=%d,fd=%d",ipport,tid,fd);
	}
}

void net::ezServerHander::onClose(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
	std::string ip=conn->getIpAddr();
	LOG_INFO("disconnect %s",ip.c_str());
	conn=nullptr;
	looper->getConnectionMgr()->delConnection(uuid);
}

void net::ezServerHander::onData(ezEventLoop* looper,int fd,uint64_t uuid,ezMsg* msg)
{
	net::ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
	if(conn)
	{
		conn->onRecvNetPack(msg);
	}
}

int net::ezServerHander::decode(ezEventLoop* looper,int fd,uint64_t uuid,char* buf,size_t s)
{
	base::ezBufferReader reader(buf,s);
	int retlen=0;
// 	while(true)
// 	{
// 		uint16_t msglen=0;
// 		reader.Read<uint16_t>(msglen);
// 		if(reader.Fail())
// 			break;
// 		if(msglen<=0||msglen>maxMsgSize_)
// 			return -1;
// 		if(!reader.CanIncreaseSize(msglen))
// 			break;
// 		ezNetPack* msg=new ezNetPack(msglen);
// 		reader.ReadBuffer(msg->data_,msglen);
// 		msg->size_=msglen;
// 		ezCrossEventData* ev=new ezCrossEventData;
// 		ev->fd_=fd;
// 		ev->uuid_=uuid;
// 		ev->event_=ezCrossData;
// 		ev->msg_=msg;
// 		looper->n2oCrossEvent(ev);
// 		retlen+=(sizeof(uint16_t)+msglen);
// 	}
	return retlen;
}

void net::ezServerHander::onError(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	onClose(looper,fd,uuid,tid);
}

net::ezServerHander::ezServerHander(uint16_t maxMsgSize):maxMsgSize_(maxMsgSize)
{

}

void net::ezGameObject::Close()
{
	if(conn_) conn_->Close();
}

void net::ezGameObject::sendNetpack(ezMsg* pack)
{
  if(conn_)
    conn_->sendNetPack(pack);
  else
    delete pack;
}

net::ezGameObject::ezGameObject()
{

}

net::ezGameObject::~ezGameObject()
{
	if(conn_)
		conn_->detachGameObject();
}

void net::ezConnectToGameObject::Close()
{
	ezGameObject::Close();
	ezConnection* conn=getConnection();
	if(conn)
		conn->getEventLooper()->getConnectionMgr()->delConnectToInfo(conn->uuid());
}

net::ezClientHander::ezClientHander(uint16_t maxMsgSize):maxMsgSize_(maxMsgSize){}

void net::ezClientHander::onOpen(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	ezConnectToInfo* info=looper->getConnectionMgr()->findConnectToInfo(uuid);
	assert(info);
	if(fd>0)
	{
		ezConnection* conn=looper->getConnectionMgr()->addConnection(looper,fd,uuid,tid);
		assert(conn);
		info->connectOK_=true;
		char ipport[128];
		net::ToIpPort(ipport,sizeof(ipport),GetPeerAddr(fd));
		conn->setIpAddr(ipport);
		LOG_INFO("connect to %s ok",ipport);
	}
	else
	{
		info->connectOK_=false;
		LOG_WARN("connect to %s:%d fail ",info->ip_.c_str(),info->port_);
	}
}

void net::ezClientHander::onClose(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
	if(!conn)
		return;
	std::string ip=conn->getIpAddr();
	LOG_INFO("disconnect %s",ip.c_str());
	conn=nullptr;
	ezConnectToInfo* info=looper->getConnectionMgr()->findConnectToInfo(uuid);
	if(info)
		info->connectOK_=false;
	looper->getConnectionMgr()->delConnection(uuid);
}

void net::ezClientHander::onError(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
{
	onClose(looper,fd,uuid,tid);
}

void net::ezClientHander::onData(ezEventLoop* looper,int fd,uint64_t uuid,ezMsg* msg)
{
	net::ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
	if(conn)
	{
		conn->onRecvNetPack(msg);
	}
}

int net::ezClientHander::decode(ezEventLoop* looper,int fd,uint64_t uuid,char* buf,size_t s)
{
	base::ezBufferReader reader(buf,s);
	int retlen=0;
// 	while(true)
// 	{
// 		uint16_t msglen=0;
// 		reader.Read<uint16_t>(msglen);
// 		if(reader.Fail())
// 			break;
// 		if(msglen<=0||msglen>maxMsgSize_)
// 			return -1;
// 		if(!reader.CanIncreaseSize(msglen))
// 			break;
// 		ezMsg* msg=new ezMsg(msglen);
// 		reader.ReadBuffer(msg->data_,msglen);
// 		msg->size_=msglen;
// 		ezCrossEventData* ev=new ezCrossEventData;
// 		ev->fd_=fd;
// 		ev->uuid_=uuid;
// 		ev->event_=ezCrossData;
// 		ev->msg_=msg;
// 		looper->n2oCrossEvent(ev);
// 		retlen+=(sizeof(uint16_t)+msglen);
// 	}
	return retlen;
}

void net::ezReconnectTimerTask::run()
{
	mgr_->reconnectAll();
}
