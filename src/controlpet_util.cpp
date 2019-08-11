#include <MDNS.h>
#include "controlpet_util.h"

MDNS clevernet_mdns;

const char *deviceName = "cleverpet";

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


void clevernet_MDNS_loop() {
    clevernet_mdns.processQueries();
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

void clevernet_setupNetwork()
{
  clevernet_setupMDNS();
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



CleverNet::CleverNet()
{
    for (int i=0; i < CLEVERNET_MAX_NODES; i++)
    {
        nodes[i].remote = invalid_address;
    }
}

bool CleverNet::process(String &message, String &node_name, IPAddress &source)
{
    if (!setup_complete)
    {
      clevernet_broadcastAddress = clevernet_getBroadcastAddress();
      setup_complete = true;
    }

    if (millis() > last_device_announcement + device_announce_interval)
    {
        announceDevice();
    }

    if (recvString(message, node_name, source))
    {
        if (message.indexOf(clevernet_cmd_shout) == 1)
        {
            node_name = clevernet_findNthSubstring(message, String(":"),0);
            Log.info (node_name + " is at address " + String(source));
            addNode(node_name, source);
        }
        return true;
    }
    return false;
}

CleverNet::~CleverNet()
{
    /* Disconnect from all nodes */
    for (int i=0; i < CLEVERNET_MAX_NODES; i++)
    {
        if (nodes[i].connection.connected())
        {
            nodes[i].connection.stop();
        }
    }
}


bool CleverNet::recvStringUDP(String &message, IPAddress &source)
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



bool CleverNet::recvStringTCP(clevernet_node *node, String &message, IPAddress &source)
{
    int c;
    if (node->connection.connected()) {
        while (node->connection.available())
        {
          c = node->connection.read();
          if (node->tcp_recv_state == RECV_STATE_NEW || node->tcp_recv_state == RECV_STATE_ONGOING )
          {
            if (node->tcp_recv_state == RECV_STATE_NEW && c == '@') 
            {
                node->tcp_recv_state = RECV_STATE_ONGOING;        
                node->tcp_recv_buffer_idx = 0;
            }
                
            if (c >= 0 && node->tcp_recv_state == RECV_STATE_ONGOING)
            {
                    if (node->tcp_recv_buffer_idx > MESSAGE_MAX_LEN)
                    {
                        node->tcp_recv_state == RECV_STATE_ERROR;
                    }
                    else {
                        node->tcp_recv_buffer[node->tcp_recv_buffer_idx] = (char)c;
                        node->tcp_recv_buffer_idx ++;
                        if (c == ';') {
                            //Message end marker found
                            node->tcp_recv_buffer[node->tcp_recv_buffer_idx] = 0;
                            node->tcp_recv_state = RECV_STATE_FINISHED;
                        }
                    }
                }
            }
            
            if (node->tcp_recv_state == RECV_STATE_FINISHED) {
               node->tcp_recv_state = RECV_STATE_NEW;
               message.remove(0);
               message += String(node->tcp_recv_buffer);
               source = node->connection.remoteIP();
               return true;
            }
            if (node->tcp_recv_state == RECV_STATE_ERROR)
            {
                node->tcp_recv_state = RECV_STATE_NEW;
            }
        }
    }
    return false;
}

bool CleverNet::sendStringTCP(clevernet_node *node, String &message) {
    static long last_log = 0;
    if (!node->connection.connected()) {
        connectTCPClient(node->name);
    }

    if (node->connection.connected()) {
        const uint8_t *msg_str = (const uint8_t *) message.c_str();
        node->connection.write(msg_str, message.length(), SEND_TIMEOUT );
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

bool CleverNet::sendString(String node_name, String message) {
    bool retVal = false;

    clevernet_node *node = getNodeByName(node_name);
    if (node)
    {
        retVal = sendStringTCP(node, message);
    } else {
        Log.info(String("Can't send message, node ") + node_name + String ( " not found"));
    }
   
    return retVal;
}

clevernet_node *CleverNet::getNodeByName(const String &name )
{
    clevernet_node *retVal = nullptr;
    for (int i=0; i < node_count; i++)
    {
        if (nodes[i].name.compareTo(name) == 0)
        {
            retVal = &nodes[i];
            break;
        }
    }
    return retVal;
}

void CleverNet::sendStringUDP(const String &message, IPAddress &remote) {
    if (!clevernet_udp_begin) {
        //We need to listen at so&me port because begin() requires us to specify a listening port
        clevernet_Udp.begin(clevernet_broadcastPort);
        clevernet_udp_begin = true;
    }
    clevernet_Udp.sendPacket(message, message.length(), remote, clevernet_broadcastPort);
}

void CleverNet::announceDevice(){
    if (setup_complete)
    {
        sendStringUDP(String("@shout:") + String(deviceName) + String(":;"), clevernet_broadcastAddress);
        last_device_announcement = millis();
    }
}

void CleverNet::connectTCPClient(const String &node_name)
{
    clevernet_node *node = getNodeByName(node_name);
    if (node)
    {
        if (!node->connection.connected() && !(node->remote == invalid_address))
        {
            Log.info(String("Connecting to ")+String(node->remote));
            node->connection.connect(node->remote, clevernet_broadcastPort+1);
        }
    }
}

//Receive on all connected channels
bool CleverNet::recvString(String &message, String &node_name, IPAddress &source)
{
    for (int i=0; i < node_count; i++)
    {
        if (recvStringTCP(&nodes[i], message, source))
        {
            node_name = nodes[i].name;
            //Received TCP Message
            return true;
        }
    }
    
    //If no node has a message for us, check for broadcasts
    if (recvStringUDP(message, source))
    {
        node_name = "";
        //Received TCP Message
        return true;
    }

    return false;
}


bool CleverNet::addNode(String node_name, IPAddress remote) {
    bool retVal = false;
    clevernet_node *node = getNodeByName(node_name);
    if (node)
    {
        //Log.info("Node exists");
    }
    else if (node_count < CLEVERNET_MAX_NODES) {
        Log.info(String("Add node ") + node_name);
        nodes[node_count].name = node_name;
        nodes[node_count].remote = remote;
        node_count ++;
        retVal = true;
    } else {
        Log.info("Max nodes reached");
    }
    return retVal;
}

