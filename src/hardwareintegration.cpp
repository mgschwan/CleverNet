/**
    hardwareintegration.cpp
    Description: Demonstrates integration of a remote sensor

    Author: Michael Gschwandtner
    Copyright:  Copyright 2019, Michael Gschwandtner
    License: MIT
    Email: mgschwan@gmail.com
    Version: 1.0 7/24/2019
*/

// This #include statement was automatically added by the Particle IDE.
#include <papertrail.h>

// This #include statement was automatically added by the Particle IDE.
#include "controlpet_util.h"

// This #include statement was automatically added by the Particle IDE.
#include <hackerpet.h>

using namespace std;

IPAddress broadcastAddress;
bool system_ready = false;

String deviceID;

unsigned char hubButtonPressed = 0;
int hubFoodtreatDuration = 5000;
int wait_between_rounds = 2000;

unsigned long reinitialization_interval = 60000;

unsigned long last_hub_action; //Remember when the last interaction has been done
unsigned long last_timestamp;
unsigned long last_dispense;
unsigned long last_button_check;


// Use the IP address and port where the receiver is listening
PapertrailLogHandler papertailHandler("192.168.0.255", 4888, "ControlPet");

SYSTEM_THREAD(ENABLED); 

HubInterface *dli = NULL;

// to avoid quirks define setup() nearly last, and right before loop()
void setup()
{

    Serial.begin(9600);  // for debug text output
    Serial1.begin(38400);  // needed for dli

    last_timestamp = millis();
    last_dispense = last_timestamp;
    last_hub_action = last_timestamp;
    last_button_check = last_timestamp;
    
    deviceID = System.deviceID();
}


void reinitializeHub()
{
    // before starting trial, wait until:
    //  1. device layer is ready (in a good state)
    //  2. foodmachine is "idle", meaning it is not spinning or dispensing food and tray is retracted (see FOODMACHINE_... constants)
    //  3. no button is currently pressed
    
    Log.info("reinitialize hub");
    dli->SetDIResetLock(false);  // allow DI board to reset if needed between trials

    while(true)
    {
        dli->Run(20);
        hubButtonPressed = dli->AnyButtonPressed();    
        if (dli->IsReady() && dli->FoodmachineState() == dli->FOODMACHINE_IDLE && hubButtonPressed == 0)
        {
            // DI reset occurs if, for example, DL detects that touchpads need re-calibration, like if hub was moved to a different surface
            dli->SetDIResetLock(true);  // prevent DI board from resetting during a trial (would cause lights to go blank etc.)
            break;
        }    
    }
    dli->SetLights(dli->LIGHT_BTNS, 0, 0, 0);   
    Log.info("reinitialization complete");
    last_hub_action = millis();

}

const String cmd_buttons = String("buttons");
const String cmd_shout = String("shout");

void commandCallback(String &msg, IPAddress &remote)
{
    if (msg.indexOf(cmd_buttons) == 1)
    {
        String button_bits = clevernet_findNthSubstring(msg, String(":"),0);
        hubButtonPressed = button_bits.toInt();
    } 
    if (msg.indexOf(cmd_shout) == 1)
    {
        String name = clevernet_findNthSubstring(msg, String(":"),0);
        Log.info (name + " is at address " + String(remote));
        clevernet_connectTCPClient(remote);
    }

}

void checkButtons()
{
    if (millis() > last_button_check + 50)
    {
        last_button_check = millis();
        clevernet_sendString(String("@buttons;"));
    }
}

bool dispense()
{
        int max_iterations = 1 + hubFoodtreatDuration/100;   
        bool foodtreat_taken = false;
        bool tray_error = true;

        while (max_iterations > 0)
        {
            unsigned char foodtreat_state = dli->PresentAndCheckFoodtreat(hubFoodtreatDuration);   
            dli->Run(20);
            // if state machine in one of two end states (food eaten or food not eaten), go to next trial
            if (foodtreat_state == dli->PACT_RESPONSE_FOODTREAT_NOT_TAKEN || foodtreat_state == dli->PACT_RESPONSE_FOODTREAT_TAKEN) // state machine finished
            {
                // start a new trial
                reinitializeHub();
                foodtreat_taken = foodtreat_state == dli->PACT_RESPONSE_FOODTREAT_TAKEN;
                tray_error = false;
                break;
            }
        }
    return foodtreat_taken;
}

void gameLoop()
{
    checkButtons();
    if (hubButtonPressed == 1 && millis() > last_dispense + wait_between_rounds)
    {
        Log.info("Button pressed, dispensing");
        dispense();
        last_dispense = millis();
    }
}

// run-loop function required for Particle
void loop()
{
    static String message;
    static IPAddress remote;
    
    // advance the device layer state machine, but with 20 millisecond max time spent per loop cycle
    if (dli)
    {
        dli->Run(20);
        if (millis() - last_hub_action > reinitialization_interval)
        {
            Log.info("Device idle: reinitializing");
            reinitializeHub();
        }
    }
    
    if (WiFi.ready() && system_ready == false)
    {
        broadcastAddress = clevernet_getBroadcastAddress();
        system_ready = true;
    
        Log.info("Wifi Ready");
        
        // initialize device layer interface (from CleverpetLibrary)
        dli = new HubInterface();
        Log.info("Device Layer created");
        
        dli->SetDoPollDiagnostics(true); //start polling the diagnostics
        dli->SetDoPollButtons(true); //start polling the buttons
        dli->PlayTone(0, 5, 10); // turn off sound
        dli->SetLights(dli->LIGHT_BTNS, 0, 0, 0);   

        Log.info ("Hub setup complete");

        reinitializeHub();
    }
    else {
        //Waiting for the Wifi to become ready        
    }

    if (system_ready) 
    {
        clevernet_MDNS_loop();
        gameLoop();
    

        if (clevernet_recvString(message, remote))
        {
            commandCallback(message, remote);
            //Log.info(String("Message Received: ")+message);
        }
    }
}

