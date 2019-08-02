//Please make sure that Wifi is ready before calling the remote_util functions

#include <Particle.h>

using namespace std;

//Calculate a broadcast address from the local IP and netmask
IPAddress clevernet_getBroadcastAddress(); 

void clevernet_setupNetwork();
bool clevernet_recvString(String &message, IPAddress &source);
bool clevernet_recvStringTCP(String &message, IPAddress &source);
bool clevernet_sendStringTCP(String message);
bool clevernet_sendString(String message);
void clevernet_sendStringUDP(String message, IPAddress &remote);
bool clevernet_recvStringUDP(String &message, IPAddress &source);
void clevernet_connectTCPClient(IPAddress &remote);


void clevernet_announceDevice();

bool clevernet_setupMDNS();
void clevernet_MDNS_loop();

String clevernet_findNthSubstring(const String &message, const String &delimiter, int n);



