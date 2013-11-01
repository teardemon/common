#include "../net/event.h"
#include "../net/poller.h"
#include "../net/connection.h"
#include "../net/netpack.h"
#include "../base/memorystream.h"

#include <limits>
using namespace net;
using namespace std;

static bool initNetwork(int major_version = 2) 
{
	WSADATA wsa_data;
	int minor_version = 0;
	if(major_version == 1) minor_version = 1;
	if(WSAStartup(MAKEWORD(major_version, minor_version), &wsa_data) != 0)
		return false;
	else
		return true;
}

int main()
{
	initNetwork();
	ezEventLoop* ev1=new ezEventLoop;
	ev1->init(new ezSelectPoller(ev1),new ezServerHander((numeric_limits<uint16_t>::max)()));
	ev1->serveOnPort(10010);
	Sleep(1000);

	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezSelectPoller(ev),new ezClientHander((numeric_limits<uint16_t>::max)()));
	ev->getConnectionMgr()->connectTo(ev,"127.0.0.1",10010);
	Sleep(1000);

	while(true)
	{
		ev1->netEventLoop();
		ev->netEventLoop();
		ev1->crossEventLoop();
		ev->crossEventLoop();
		Sleep(10);
	}
	delete ev;
	return 1;
}