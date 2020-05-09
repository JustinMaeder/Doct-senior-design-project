#include "BLEDevice.h"
#include "TinyGPS++.h"
#define HM_MAC "50:51:A9:FE:31:B6"
#include <SoftwareSerial.h>
#include <SPI.h>
TinyGPSPlus gps;
SoftwareSerial mySerial(32,33); //RX,TX

//---------------------------SIM---------------------------//
// Your GPRS credentials (leave empty, if missing)
const char apn[]      = "hologram"; // Your APN
const char gprsUser[] = ""; // User
const char gprsPass[] = ""; // Password
const char simPIN[]   = ""; // SIM card PIN code, if any

// TTGO T-Call pin definitions
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT  Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb

// Define the serial console for debug prints, if needed
//#define TINY_GSM_DEBUG SerialMon
//#define DUMP_AT_COMMANDS

#include <Wire.h>
#include <TinyGsmClient.h>
#include "utilities.h"

#define TINY_GSM_TEST_TCP true

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

const char server[] = "167.99.109.182";

TinyGsmClient client(modem);
const int  port = 8081;
//---------------------------END-SIM---------------------------//

//---------------------------ESP-HUD---------------------------//
static BLEUUID serviceUUID("0000FFE0-0000-1000-8000-00805F9B34FB");
static BLEUUID charUUID("0000FFE1-0000-1000-8000-00805F9B34FB");

static boolean connect = true; 
static boolean connected = false;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;
BLEClient*  pClient;


//    BLE Callbacks

static void notifyCallback 
(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
  String EingangDaten = "";
  for (int i = 0; i < length; i++)EingangDaten += char(*pData++); // Append byte as character to string. Change to the next memory location
  Serial.println(EingangDaten);
}
//Connect to BLE Server

bool connectToServer(BLEAddress pAddress)
{
  Serial.println("Trying to Connect with.... ");
  Serial.println(pAddress.toString().c_str());
  pClient = BLEDevice::createClient();
  pClient->connect(pAddress);
 // Serial.println("CONNECTED");

  // Obtaining a reference to required service
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);

  if (pRemoteService == nullptr)
  {
    Serial.print("Gefunden falsche UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }

  // reference to required characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Gefunden falsche Characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    return false;
  }

  pRemoteCharacteristic->registerForNotify(notifyCallback);
  return true;
}

//---------------------------END-ESP-HUD-----------------------//

//---------------------Hardware Timer-------------------------//
volatile int interruptCounter;
int totalInterruptCounter;
 
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}
//---------------------End-Hardware Timer-------------------------//

//---------------------Vehicle variables-------------------------//
int8_t hr,mn,spd,range, unitID;
int lati, lngi, latii, newlatii, lngii;
String h="", m="",sp="";
int count = 0;
byte one,two,three,four,bat;


int i = 1;
void setup(){
  Serial.begin(115200);
  unitID = random(100);

  //_________________Bluetooth SETTUP_________________
  Serial.println("Start");
  BLEDevice::init("");
  pinMode(22,OUTPUT);
  pinMode(23,OUTPUT);
  //__________________GPS SETTUP_________________
  mySerial.begin(9600);
  
  //__________________SLEEP SETTUP_____________________
  esp_sleep_enable_timer_wakeup(30000000);

  //---------------------------SIM---------------------------//
  // Keep power when running from battery
  Wire.begin(I2C_SDA, I2C_SCL);
  bool   isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  // Set-up modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // Or, use modem.init() if you don't need the complete restart

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }
}

void loop(){
  reconnect:
  
   //---------------------------SIM---------------------------//
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
    SerialMon.println(" fail");
    return;
  }
  SerialMon.println(" OK");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

  SerialMon.print(F("Connecting to APN: "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    return;
  }
  SerialMon.println(" OK");

  SerialMon.print("Connecting to ");
  SerialMon.print(server);
  if (!client.connect(server, port)) {
    SerialMon.println(" fail");
    return;
  }
  SerialMon.println(" OK");

  // Make a HTTP GET request:
  SerialMon.println("Performing HTTP GET request...");
//---------------------------SIM---------------------------//
label:

  while(mySerial.available() > 0){
    gps.encode(mySerial.read());
  }
  if(gps.time.isUpdated()){
    hr = (gps.time.hour());   // Hour   (0-23) (u8)
    mn = (gps.time.minute()); // Minute (0-59) (u8)
    hr = hr + 5;
    hr = hr % 12;
    if (hr < 10){
      h = "0" + String(hr);    
    }else{
      h = String(hr);
    }
    if (mn < 10){
      m = "0" + String(mn);
    }else{
      m = String(mn);
    }
  }
  if(gps.speed.isUpdated()){
    spd = gps.speed.mph();
    sp = String(spd);
  }
  if(gps.location.isUpdated()){
    lati = gps.location.lat(), 6;
    latii= gps.location.rawLat().billionths;
    lngi = gps.location.lng(), 6;
    lngii= gps.location.rawLng().billionths;
  }
//---------------Sending data to HUD-----------------------//
  int batt = 60;
  digitalWrite(22,HIGH);
  if (connect == true) 
  {
    pServerAddress = new BLEAddress(HM_MAC);
    // Serial.println("ServerAddress");
    // Serial.println(pServerAddress);
    if (connectToServer(*pServerAddress)){
      connected = true;
      connect = false;
    }else{
      Serial.println("Connection does not work");
    }
  }

  if (connected){
    String cmp = "s00b00r00t0000\n";
    String HUD_data = "";
    String s = "s" + sp;
    String b = "b60";
    String r = "r10";
    String tim = "t" + h + m + "\n";
    HUD_data = s + b + r + tim;
   
    if (millis() - count > 820){
      pRemoteCharacteristic->writeValue(HUD_data.c_str(), HUD_data.length());
      Serial.print("sent: ");
      Serial.print(HUD_data);
      count = millis();
    }
    digitalWrite(23,HIGH);
    digitalWrite(22,LOW); 
  }
//---------------END DATA TRANSMISSION---------------------//

  if((lati == 0) || (lngi == 0) || (latii == newlatii)){
    goto label;
  }
  newlatii = latii;
  
  Serial.println("-------------------------------------------------------------------");
  Serial.println("Speed = "+String(spd));
  Serial.println("Battery = "+String(batt));
  Serial.println("Latitude = "+String(lati)+"."+String(latii));
  Serial.println("Longitude = "+String(lngi)+"."+String(lngii));
  Serial.println("{'unitID':'"+String(unitID)+"','batt':'"+String(batt)+"','lat':'"+String(lati)+"."+String(latii)+"','long':'"+String(lngi)+"."+String(lngii)+"','spd':'"+String(spd)+"'}");
  client.println("{'unitID':'"+String(unitID)+"','batt':'"+String(batt)+"','lat':'"+String(lati)+"."+String(latii)+"','long':'"+String(lngi)+"."+String(lngii)+"','spd':'"+String(spd)+"'}");
  count++;

  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      SerialMon.print(c);
      timeout = millis();
    }
  }
  SerialMon.println();

  if (!client.connected()) {
    SerialMon.println("Disconnected from server");
    delay(30000); // wait 30 seconds to reconnect
    goto reconnect;
  }

  // Reconnect after 60 seconds
  if(true){   //count < 20
    //delay(49869); // 49.869 seconds + 2G delay (10.131 seconds)
    goto label;
  }
  else{
    // Shutdown
    client.stop();
    SerialMon.println(F("Server disconnected"));
  
    modem.gprsDisconnect();
    SerialMon.println(F("GPRS disconnected"));
  
    esp_deep_sleep_start();
  }
}
