#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "clevernet_esp32.h"


const char* ssid = "YOURNETWORK";  //Put your SSID here
const char* password = /*secretbegin*/ "*************" /*secretend*/;  //Put your Wifi Password here
const char* system_name = "cleverpet_button";

bool system_ready = false;

int input_pin = 13;

int hub_button_pressed = 0;

void setup(void)
{  
    Serial.begin(115200);

    pinMode(input_pin, INPUT_PULLUP);

    // Connect to WiFi network
    WiFi.begin(ssid, password);
}

const String cmd_check_buttons = String("buttons");

void commandCallback(String &msg)
{
    Serial.println(msg);
    if (msg.indexOf(cmd_check_buttons) == 1)
    {
       clevernet_sendString(String("@buttons:")+String(hub_button_pressed)+String(":;"));
       hub_button_pressed = 0;
    }
}

void loop(void)
{
    static int last_button_state = 0;
    String message;

 
    if ( WiFi.status() == WL_CONNECTED && system_ready == false)
    {
        Serial.println("Wifi Ready");
        Serial.println(WiFi.localIP());

        system_ready = true;

        clevernet_setupNetwork(String(system_name));

        /* Start announcing our presence */
        MDNS.begin(system_name);
    }
    else {
        //Waiting for the Wifi to become ready        
    }

    if (system_ready)
    {
      clevernet_announceLoop();
      
      int button_state = digitalRead(input_pin);
      if (button_state != last_button_state)
      {
        Serial.println("Switch");
        delay ( 100 );         
        last_button_state = button_state;
        if (last_button_state) {
          hub_button_pressed = 1;
        }
      }

      message = "";
      if (clevernet_recvString(message))
      {
          commandCallback(message);
      }      
    }      
}

    
