/**
 CoolFleks® client software
 */

#include "BLEDevice.h"
//include "BLEScan.h"


// The BLEUUIDs are used in pointers that point to either SERVICES or CHARACTERISTICS in both local and remote BLE devices
// These pointers can be operated on, and some operations include intialising the pointer itself (e.g. creating pointers pointing to local or remote characteristics)
// searching for char_pointers with a certain BLEUUID, etc

//In our project we use one 'umbrella' SERVICE, that contains characteristics COMMAND_CHAR_UUID and USER_CHAR_UUID
// These are the only characteristics needed, and the only necessary for data transfer. UUIDs must be the same for BLE to work. 

// CoolBox service ID.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

// Characteristics for LED Command and UserID. 
static BLEUUID COMMAND_UUID ("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID USER_CHAR_UUID ("2d008b48-e321-4408-b049-7f295a9e9996");

static BLERemoteCharacteristic* PCOMMANDCHARACTERISTIC; 

//Local pointer to point to address of CoolBox 
BLEAdvertisedDevice *myDevice;
std::string value = ""; 
std::string incomingString = "";
std::string OldString = ""; 

//Flag that allows connection between CoolBox and CoolFleks
static boolean doConnect = false;
//Flag that represents connection status 
static boolean connected = false;
//Flag that enables scanning for CoolBoxes by CoolFleks
static boolean doScan = false;
// Weirdass boolean functions sending on and off commands
//const uint8_t notificationOn[] = {0x1, 0x0};
//const uint8_t notificationOff[] = {0x1, 0x0};
static std::string USERID = "13"; 

/////////////////////////////////////////////////////////////////// CALLBACK FUNCTIONS //////////////////////////////////////////////////////////////////////


//Client Callback functions. Sets connected state to false, restarts scanning, resets value to "", and resets
// the parsed variable incomingString back to what it originally was before connection (OldString). 
class MyClientCallback : public BLEClientCallbacks{
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    doScan = true; 
    value = ""; 
    incomingString = OldString; 
    Serial.println("onDisconnect");
  }
};


//Characteristic Callback function for COMMANDCHARACTERISTIC. Handles Server writes to COMMAND char array used to control LEDs. 
//After parsing from Bluetooth Serial, one should update COMMAND's chars with the parsed chars before sending the commands to the LED strip. 

//class CommandCallback : public BLEClientCallbacks{
//void onWrite(BLECharacteristic *customCharacteristic){    
//  }};


/////////////////////////////////////////////////////////////////////////////////////// CONNECT TO SERVER FUNCTION //////////////////////////////////////////////////////////////////////////////////////////////////////////



//Boolean function that stores characteristic IDs and connections between CoolBox and CoolFleks. Returns false and stops the rest of the function immediately when an if-statement fails, returns true if all operations prove successful. 
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str()); 
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created CoolFleks client");
    
    //Set Client Callbacks to the one above (connected state flagging) 
    pClient->setClientCallbacks(new MyClientCallback());  
    
    //After setting callbacks, we can now properly connect to the address of the remote server as specified by the myDevice pointer. 
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to CoolBox");



    // makes a pointer to the service that (now) should be connected to the pClient service. 
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");



    // Obtain a reference to the characteristic in the service of the remote BLE server.
    PCOMMANDCHARACTERISTIC = pRemoteService->getCharacteristic(COMMAND_UUID);
    if (PCOMMANDCHARACTERISTIC == nullptr) {
      Serial.print("Failed to find our COMMAND UUID: ");
      Serial.println(COMMAND_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found COMMAND UUID");


    //Checks if PCOMMANDCHARACTERISTIC can be read. If true, then the previous LED command is stored in OldString. The redlight command is then read and placed into incoming string. 
    //alternatively, there is no need for there to be any data transfer. Instead we can just set value as a permanent 'redlight' command.
     
   if(PCOMMANDCHARACTERISTIC->canRead())
   {
    OldString = incomingString; 
    value = PCOMMANDCHARACTERISTIC->readValue(); //at this point value should be used to replace incomingString in main loop. value is set to "" by default. Can also be done in the next line.  
    Serial.print("The CoolFleks is now red, the command used was: "); // assumption is that the numbers programming the LEDs will be printed out
    Serial.println(value.c_str());
    Serial.print("The old command was: "); 
    Serial.println(incomingString.c_str());  
    }
   //alternate if statement. Uses canRead as trigger. 
   //if(PCOMMANDCHARACTERISTIC->canRead())
   //{
   //incomingString = value; 
   //Serial.println("CoolFleks is now red");    
   // }
    
    

    BLERemoteCharacteristic* PUSERCHARACTERISTIC = pRemoteService->getCharacteristic(USER_CHAR_UUID);
    if (PUSERCHARACTERISTIC == nullptr) {
      Serial.print("Failed to find CoolBox USERID Characteristic container: ");
      Serial.println(COMMAND_UUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found USER UUID");

    if(PUSERCHARACTERISTIC->canWrite())
    {
      PUSERCHARACTERISTIC->writeValue(NULL);
      PUSERCHARACTERISTIC->writeValue(USERID); 
      }
    return true; 

    
    
}




/**
 * CallBack behavior for the Advertised Device. 
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */

  void onResult(BLEAdvertisedDevice pAdvertisedDevice) {
    if (pAdvertisedDevice.getName() == "CoolBox"){
      BLEDevice::getScan()->stop();
      Serial.print("CoolBox has been found: ");
      Serial.println(pAdvertisedDevice.toString().c_str());  
      myDevice = new BLEAdvertisedDevice(pAdvertisedDevice);
      doConnect = true;
      doScan = true;

    }
    // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 4 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1000);
  pBLEScan->setWindow(1000);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(4, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true. Sets notify to On in remote BLE device. 

 if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");  
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect=false;
  }



else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  

  

  
  
  delay(1000); // Delay a second between loops.
} // End of loop
