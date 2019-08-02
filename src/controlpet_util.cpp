#include <MDNS.h>
#include "controlpet_util.h"

MDNS clevernet_mdns;

UDP clevernet_Udp;
int clevernet_broadcastPort = 4888;
IPAddress clevernet_broadcastAddress;    
bool clevernet_udp_begin = false;

int device_announce_interval = 1000;
long last_device_announcement;

const char *deviceName = "cleverpet";

#define MESSAGE_MAX_LEN 512

#define SEND_TIMEOUT 5000

#define RECV_STATE_NEW 0
#define RECV_STATE_ONGOING 1
#define RECV_STATE_ERROR 2
#define RECV_STATE_FINISHED 3

int udp_recv_buffer_idx = 0;
int udp_recv_state = RECV_STATE_NEW;
char udp_recv_buffer[MESSAGE_MAX_LEN+1];


int tcp_recv_buffer_idx = 0;
int tcp_recv_state = RECV_STATE_NEW;
char tcp_recv_buffer[MESSAGE_MAX_LEN+1];

TCPClient client;

String clevernet_findNthSubstring(const String &message, const String &delimiter, int n)
{
    int i = 0;
    int npos = 0;

    for (i = 0; i < n+1; i++)
    {
        npos = message.indexOf(delimiter, npos);
        if (npos < 0) break;
        npos++;
    }
    if (npos >= 0)
    {
        int endOfSubstring = message.indexOf(delimiter, npos);
        if (endOfSubstring >= 0)
        {
            return message.substring(npos, endOfSubstring);
        }
        return message.substring(npos);
    }
    return String("");
}

void clevernet_announceDevice(){
    clevernet_sendStringUDP(String("@shout:") + String(deviceName) + String(":;"), clevernet_broadcastAddress);
    last_device_announcement = millis();
}

void clevernet_sendStringUDP(String message, IPAddress &remote) {
    if (!clevernet_udp_begin) {
        //We need to listen at so&me port because begin() requires us to specify a listening port
        clevernet_Udp.begin(clevernet_broadcastPort);
        clevernet_udp_begin = true;
    }
    clevernet_Udp.sendPacket(message, message.length(), remote, clevernet_broadcastPort);
}

void clevernet_MDNS_loop() {
    clevernet_mdns.processQueries();
    if (millis() > last_device_announcement + device_announce_interval)
    {
        clevernet_announceDevice();
    }
}

bool clevernet_setupMDNS() {
    bool success = false;

    success = clevernet_mdns.setHostname(deviceName);
    Log.info("MDNS: Set hostname %d",success);

    if (success) {
        success = clevernet_mdns.begin(true);
    }

    Log.info("MDNS: Begin %d",success);

    return success;
}


//Receive on all connected channels
bool clevernet_recvString(String &message, IPAddress &source)
{
    if (clevernet_recvStringTCP(message, source))
    {
        //Received TCP Message
        return true;
    } else if (clevernet_recvStringUDP(message, source))
    {
        //Received TCP Message
        return true;
    }
    return false;
}

void clevernet_setupNetwork()
{
  clevernet_setupMDNS();
  clevernet_broadcastAddress = clevernet_getBroadcastAddress();
}

void clevernet_connectTCPClient(IPAddress &remote)
{
    if (!client.connected())
    {
        Log.info(String("Connecting to ")+String(remote));
        client.connect(remote, clevernet_broadcastPort+1);
    }
}


bool clevernet_sendStringTCP(String message) {
    static long last_log = 0;
    if (client.connected()) {
        const uint8_t *msg_str = (const uint8_t *) message.c_str();
        client.write(msg_str, message.length(), SEND_TIMEOUT );
        return true;
    } else {
        if (millis() > last_log + 500)
        {
            Log.info("Not connected, can't send");
            last_log = millis();
        }
    }
    return false;
}

bool clevernet_sendString(String message) {
    bool retVal = false;

    retVal = clevernet_sendStringTCP(message);
   
    return retVal;
}

bool clevernet_recvStringTCP(String &message, IPAddress &source)
{
    int c;
    if (client.connected()) {
        while (client.available())
        {
          c = client.read();
          if (tcp_recv_state == RECV_STATE_NEW || tcp_recv_state == RECV_STATE_ONGOING )
          {
            if (tcp_recv_state == RECV_STATE_NEW && c == '@') 
            {
                //Log.info("Start marker found");
                tcp_recv_state = RECV_STATE_ONGOING;        
                tcp_recv_buffer_idx = 0;
            }
                
            if (c >= 0 && tcp_recv_state == RECV_STATE_ONGOING)
            {
                    if (tcp_recv_buffer_idx > MESSAGE_MAX_LEN)
                    {
                        tcp_recv_state == RECV_STATE_ERROR;
                    }
                    else {
                        tcp_recv_buffer[tcp_recv_buffer_idx] = (char)c;
                        tcp_recv_buffer_idx ++;
                        if (c == ';') {
                            //Log.info("End marker found");
                            //Message end marker found
                            tcp_recv_buffer[tcp_recv_buffer_idx] = 0;
                            tcp_recv_state = RECV_STATE_FINISHED;
                        }
                    }
                }
            }
            
            if (tcp_recv_state == RECV_STATE_FINISHED) {
               tcp_recv_state = RECV_STATE_NEW;
               message.remove(0);
               message += String(tcp_recv_buffer);
               source = client.remoteIP();
               return true;
            }
            if (tcp_recv_state == RECV_STATE_ERROR)
            {
                tcp_recv_state = RECV_STATE_NEW;
            }
        }
    }
    return false;
}


bool clevernet_recvStringUDP(String &message, IPAddress &source)
{
    int c;
    if (clevernet_Udp.parsePacket() > 0) {
        while (clevernet_Udp.available())
        {
          c = clevernet_Udp.read();
          if (udp_recv_state == RECV_STATE_NEW || udp_recv_state == RECV_STATE_ONGOING )
          {
            if (udp_recv_state == RECV_STATE_NEW && c == '@') 
            {
                Log.info("Start marker found");
                udp_recv_state = RECV_STATE_ONGOING;        
                udp_recv_buffer_idx = 0;
            }
                
            if (c >= 0 && udp_recv_state == RECV_STATE_ONGOING)
            {
                    if (udp_recv_buffer_idx > MESSAGE_MAX_LEN)
                    {
                        udp_recv_state == RECV_STATE_ERROR;
                    }
                    else {
                        udp_recv_buffer[udp_recv_buffer_idx] = (char)c;
                        udp_recv_buffer_idx ++;
                        if (c == ';') {
                            Log.info("End marker found");
                            //Message end marker found
                            udp_recv_buffer[udp_recv_buffer_idx] = 0;
                            udp_recv_state = RECV_STATE_FINISHED;
                        }
                    }
                }
            }
            
            if (udp_recv_state == RECV_STATE_FINISHED) {
               udp_recv_state = RECV_STATE_NEW;
               message.remove(0);
               message += String(udp_recv_buffer);
               source = clevernet_Udp.remoteIP();
               return true;
            }
            if (udp_recv_state == RECV_STATE_ERROR)
            {
                udp_recv_state = RECV_STATE_NEW;
            }
        }
    } 
    return false;
}




IPAddress clevernet_getBroadcastAddress() {
 
        IPAddress localIP;
        IPAddress broadcastIP;
        IPAddress netmask;

        localIP = WiFi.localIP();
        netmask = WiFi.subnetMask();

        for (int idx = 0; idx < 4; idx++) {
            broadcastIP[idx] = localIP[idx] | ~netmask[idx];
        }

        return broadcastIP;
}

