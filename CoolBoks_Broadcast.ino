/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
*/


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define _GLIBCXX_USE_C99 1
BLEServer* pServer = NULL;
BLECharacteristic* PCOMMANDCHARACTERISTIC = NULL; //value will be set later 
BLECharacteristic* PUSERCHARACTERISTIC = NULL;
BLECharacteristic* PENTRYCHARACTERISTIC = NULL; 
std::string newUser = ""; 
int BOX_ID = 3; // Singsaker, according to Django code 


bool deviceConnected = false;
bool oldDeviceConnected = false;






// See the following for generating UUIDs:
// https://www.uuidgenerator.net/


// The BLEUUIDs are used in pointers that point to either SERVICES or CHARACTERISTICS in both local and remote BLE devices
// These pointers can be operated on, and some operations include intialising the pointer itself (e.g. creating pointers pointing to local or remote characteristics)
// searching for char_pointers with a certain BLEUUID, etc

//In our project we use one 'umbrella' SERVICE, that contains characteristics COMMAND_CHAR_UUID and USER_CHAR_UUID
// These are the only characteristics needed, and the only necessary for data transfer. UUIDs must be the same for BLE to work. 

static BLEUUID SERVICE_UUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID COMMAND_CHAR_UUID ("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID USER_CHAR_UUID ("2d008b48-e321-4408-b049-7f295a9e9996");






//////////////////////////////////////////////////////////////////////////////////////////// CALLBACK BODY FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////////////////////

class NewUserCallback: public BLECharacteristicCallbacks  //New User Callback writes out a newly received USERID from a CoolFleks.
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    newUser = pCharacteristic->getValue();
    Serial.print("The User ID is: "); 
    Serial.println(newUser.c_str()); 
    }
  
  };

class MyServerCallbacks: public BLEServerCallbacks {    //Server callback set device connected states to either true or false on Connect/Disconnect
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


//Basic setup procedure. The steps below MUST be done in this exact sequence for the whole thing to work. 
void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("CoolBox");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Intialises the BLE Service, but setup code for the service will follow below. 
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Sets up characteristics for Command (red light), UserIDCharacteristic (retrieves value from CoolFleks ESP32). Addition of
  // characteristics take place here, in the same setup stage
  PCOMMANDCHARACTERISTIC = pService->createCharacteristic(
                      COMMAND_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  PUSERCHARACTERISTIC = pService->createCharacteristic(
                      USER_CHAR_UUID, 
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE               
                    );                                           



  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // BLE2902 is a 'descriptor' for BLE Characteristics that enables notification functions between BLE devices connected to each other. 
  PCOMMANDCHARACTERISTIC->addDescriptor(new BLE2902());
  PUSERCHARACTERISTIC->addDescriptor(new BLE2902()); 


// Characteristic callback setup. I've made one callback function (using classes) for Command and UserID messages passed between CoolFleks and CoolBox. Box ID and Date Time have their own code. 

  PUSERCHARACTERISTIC->setCallbacks(new NewUserCallback());  //reads characteristic of Client. Callback function coming up. 
  //Setting values for testing. 
  PCOMMANDCHARACTERISTIC->setValue("thingy"); 
  PUSERCHARACTERISTIC->setValue("No User");

  
  // When  the service is set up as done above, we formally start the service with its own function
  pService->start();

  // To make sure our service and characteristics are found, we initialise Advertising.
  // The following code is run such that: 
  //1) the BLE Device's advertising function is set up as pAdvertising, 
  //2) The Service (which acts as an umbrella over our characteristics
  // is added to the advertising function, 
  //3-4) Default settings, no need to think about these, 
  //5) Starts up the advertising 
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting for connection to CoolFleks to obtain data...");
}

// As seen in the set up above, this can seem quite involved for what should be a simple process. 
// The best way to think about this is that a BLE Device
// must simply be set up IN SEQUENCE from the TOP to the BOTTOM of its system hierarchy. 
// The TOP being the BLEDevice itself (in our case, the ESP32), and the lowest being the BLE Characteristic. 
// In between we have the Service function and Advertising function.
// The Service function envelops the Characteristics function. The Advertising function is on the same
// level as Service function, but cannot exist independently before Service is initialised, therefore, Service function is the big daddy here after BLEDevice


void loop() { //very simple code
    // Read off PUSERCHARACTERISTIC
    if(deviceConnected)
    { 
      Serial.println(PUSERCHARACTERISTIC->getValue().c_str());
      delay(1000);
      }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(700); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        Serial.println("The device is trying to connect...");
        oldDeviceConnected = deviceConnected;
    }
}
