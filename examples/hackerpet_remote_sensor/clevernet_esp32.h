using namespace std;

//Calculate a broadcast address from the local IP and netmask
IPAddress clevernet_getBroadcastAddress(); 

//Receive a string via UDP enclosed in a @......;
bool clevernet_recvStringUDP(String &message);

void clevernet_setupNetwork(const String &name);
bool clevernet_recvString(String &message);
bool clevernet_recvStringTCP(String &message);
bool clevernet_sendStringTCP(String message);
bool clevernet_sendString(String message);
void clevernet_sendStringUDP(String message, IPAddress &remote);

void clevernet_announceDevice();
void clevernet_announceLoop();

String clevernet_findNthSubstring(const String &message, const String &delimiter, int n);
