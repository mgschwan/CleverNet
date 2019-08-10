//Please make sure that Wifi is ready before calling the remote_util functions

#include <Particle.h>

using namespace std;


#define MESSAGE_MAX_LEN 512

#define SEND_TIMEOUT 5000

#define RECV_STATE_NEW 0
#define RECV_STATE_ONGOING 1
#define RECV_STATE_ERROR 2
#define RECV_STATE_FINISHED 3

const IPAddress invalid_address (0,0,0,0);

//Calculate a broadcast address from the local IP and netmask
IPAddress clevernet_getBroadcastAddress(); 

void clevernet_setupNetwork();
bool clevernet_setupMDNS();
void clevernet_MDNS_loop();

String clevernet_findNthSubstring(const String &message, const String &delimiter, int n);

const String clevernet_cmd_buttons = String("buttons");
const String clevernet_cmd_shout = String("shout");


typedef struct clevernet_node {
    String name;
    TCPClient connection;
    IPAddress remote;   
    int tcp_recv_buffer_idx = 0;
    int tcp_recv_state = RECV_STATE_NEW;
    char tcp_recv_buffer[MESSAGE_MAX_LEN+1];
} clevernet_node;

#define CLEVERNET_MAX_NODES 16

class CleverNet {
    public:
        CleverNet();
        ~CleverNet();
        bool process(String &message, String &node_name, IPAddress &source);

        bool recvStringUDP(String &message, IPAddress &source);
        bool recvStringTCP(clevernet_node *node, String &message, IPAddress &source);
        bool sendStringTCP(clevernet_node *node, String &message);
        bool sendString(String node_name, String message);
        clevernet_node *getNodeByName(const String &node_name);
        void sendStringUDP(const String &message, IPAddress &remote);
        void announceDevice();
        void connectTCPClient(const String &node_name);
        bool recvString(String &message, String &node_name, IPAddress &source);

        bool addNode(String node_name, IPAddress remote);


    private:
        clevernet_node nodes[CLEVERNET_MAX_NODES];
        int udp_recv_buffer_idx = 0;
        int udp_recv_state = RECV_STATE_NEW;
        char udp_recv_buffer[MESSAGE_MAX_LEN+1];

        UDP clevernet_Udp;
        int clevernet_broadcastPort = 4888;
        IPAddress clevernet_broadcastAddress;    
        bool clevernet_udp_begin = false;

        int device_announce_interval = 1000;
        long last_device_announcement;

        const char *deviceName = "cleverpet";

        bool setup_complete = false;

        int node_count = 0;

};



