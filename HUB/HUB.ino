#include <WiFi.h>
const char* ssid = "Frontier7600";
const char* password = "9156879178";
const char* host = "167.99.109.182";

//-------------------Keypad-----------------------//
#include <Keypad.h>

short unlock_sig = 21;
String pin = "";
String line = "";
String unlock_pin = "";

const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[rows] = {22, 23, 5, 18}; //connect to the row pinouts of the keypad
byte colPins[cols] = {2, 0, 4}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

}

void loop(){
  
  //-------------------Keypad-----------------------//
  get_pin:
  pinMode(unlock_sig, OUTPUT);
  delay(100);
  WiFiClient client;
  Serial.printf("\n[Connecting to %s ... ", host);
  if (client.connect(host, 3000)){
    Serial.println("[connected]");
    client.println("GET /pin HTTP/1.0\n");
    Serial.println("[Response:]");
    while (client.connected() || client.available()){
      if (client.available()){
        String line = client.readStringUntil('\n');
        if(line.length() == 4){
           Serial.println("gotthepin");
           unlock_pin = line; 
           Serial.println(unlock_pin);
           goto pin_input;
        } //End if
      } // End if
    } // End while
    
    client.stop();
    Serial.println("\n[Disconnected]");
  }
  else{
    Serial.println("connection failed!]");
    client.stop();
  }
  delay(1000);
  goto get_pin;
  
  pin_input:
  //----------------KEYPAD-------------------
  char key = keypad.getKey();
  delay(10);
  if(key != NO_KEY){
      pin = pin + key;
      if (key == '#'){
        pin = pin.substring(0,pin.length() - 1);
        Serial.println(pin);
        if(pin == unlock_pin){
          Serial.println("Unlock");
          digitalWrite(unlock_sig , HIGH);
          delay(3000);
          digitalWrite(unlock_sig , LOW);
          pin = ""; 
          goto get_pin;
        }
        pin = ""; 
      }
  }
  goto pin_input;
}
