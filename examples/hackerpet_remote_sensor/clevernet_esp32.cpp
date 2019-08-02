#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#include "clevernet_esp32.h"

WiFiUDP clevernet_Udp;
int clevernet_broadcastPort = 4888;
IPAddress clevernet_broadcastAddress;    
bool clevernet_udp_begin = false;

int device_announce_interval = 1000;
long last_device_announcement;

String deviceName;

#define MESSAGE_MAX_LEN 512

#define SEND_TIMEOUT 5000

#define RECV_STATE_NEW 0
#define RECV_STATE_ONGOING 1
#define RECV_STATE_ERROR 2
#define RECV_STATE_FINISHED 3

int recv_buffer_idx = 0;
int recv_state = RECV_STATE_ERROR;
char recv_buffer[MESSAGE_MAX_LEN+1];


int tcp_recv_buffer_idx = 0;
int tcp_recv_state = RECV_STATE_NEW;
char tcp_recv_buffer[MESSAGE_MAX_LEN+1];

WiFiServer server = WiFiServer(clevernet_broadcastPort+1);

WiFiClient client;

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

//Receive on all connected channels
bool clevernet_recvString(String &message)
{

    if (clevernet_recvStringTCP(message))
    {
        //Received TCP Message
        return true;
    } 
    return false;
}

void clevernet_setupNetwork(const String &name)
{
  deviceName = name;
  server.begin();
  clevernet_broadcastAddress = clevernet_getBroadcastAddress();
}

void clevernet_announceLoop()
{
    if (millis() > last_device_announcement + device_announce_interval)
    {
        clevernet_announceDevice();
    }
}

void clevernet_announceDevice(){
    clevernet_sendStringUDP(String("@shout:") + deviceName + String(":;"), clevernet_broadcastAddress);
    last_device_announcement = millis();
}

void clevernet_sendStringUDP(String message, IPAddress &remote) {
    if (!clevernet_udp_begin) {
        //We need to listen at so&me port because begin() requires us to specify a listening port
        clevernet_Udp.begin(clevernet_broadcastPort+1);
        clevernet_udp_begin = true;
    }
    Serial.println("Writing Announce packet");
    Serial.println(remote);

    clevernet_Udp.beginPacket(remote, clevernet_broadcastPort);
    clevernet_Udp.write((const uint8_t *)message.c_str(), message.length());
    clevernet_Udp.endPacket();
}


bool clevernet_sendStringTCP(String message) {
    if (client.connected()) {
	client.setTimeout(SEND_TIMEOUT);
        const uint8_t *msg_str = (const uint8_t *) message.c_str();
        client.write(msg_str, message.length());
        return true;
    }
    return false;
}

bool clevernet_sendString(String message) {
    bool retVal = false;
    Serial.println("Sending message");
    retVal = clevernet_sendStringTCP(message);
    
    return retVal;
}

bool clevernet_recvStringTCP(String &message)
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
               return true;
            }
            if (tcp_recv_state == RECV_STATE_ERROR)
            {
                tcp_recv_state = RECV_STATE_NEW;
            }
        }
    } else {
        client = server.available();
        if (client.connected())
        {
	  //Client connected
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
