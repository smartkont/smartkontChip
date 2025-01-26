#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <EEPROM.h> 
#include <ESP8266WebServer.h>



/*
Legal statement this code is the property of Financial Minds Consulting Ltd. Canada making any copy of the code or using any art of the code without written approval from the owner of Financial Minds Consulting Ltd is not permitted.

*/


// Server Name path can be replaced by server name and path location
//const char* serverNamePath = "http://70.48.65.225/learnTest/ESP8266_insert.php";
//const char* serverNamePath = "https://device.smartkont.ca/learnTest/ESP8266_insert.php";
// const char* serverNamePath = "https://device.smartkont.ca/";
const char* serverNamePath = "https://deviceapi.pdq2.com/smartKontCode/containerInsert.php";
const char* serverKontSetupPath = "https://deviceapi.pdq2.com/smartKontCode/kontSetup.php";
const char* apiKeyValue = "SmartDabba!ndCan!001";


String macAddress;

// Process controlling variables
int wifiNotConnCntr = 0; // Number of time wifi is not connected for consecutive times.
bool httpRequestFlag = false; // https Send Request Flag, control the number of times http request os sent.
const int sleepTimer=30; // ESP sleep timer
const char *ssid = "SmartKont"; // ESP advertise for Config Portal.
const char *password = "987654321"; // Not used

// Sensor pin definition for distance calculation
#define trigPin D5
#define echoPin D6

// LED pin definition for illumination
const int PIN_RED   = D2; 
const int PIN_GREEN = D1; 
const int PIN_BLUE  = D7; 

// Wifi user name and passwd structure
struct settings {
  char ssid1[30];
  char password1[30];
} user_wifi = {};

ESP8266WebServer server(80);

// Sonic sensor distance and duration measurement
long  duration;
int distance;


// May be useful in container Setup
//AutoConnectConfig acConfig;
//acConfig.apid = "ESP-" + String(ESP.getChipId(), HEX);

// Wifimanager for connection and config Portal
WiFiManager wifiManager;

//Work here
//void 
// Custom portal
void handlePortal() {

  if (server.method() == HTTP_POST) {

    strncpy(user_wifi.ssid,     server.arg("ssid").c_str(),     sizeof(user_wifi.ssid) );
    strncpy(user_wifi.password, server.arg("password").c_str(), sizeof(user_wifi.password) );
    user_wifi.ssid[server.arg("ssid").length()] = user_wifi.password[server.arg("password").length()] = '\0';
    EEPROM.put(0, user_wifi);
    EEPROM.commit();
    

    server.send(200,   "text/html",  "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Wifi Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please restart the device.</p></main></body></html>" );
  } else {

    server.send(200,   "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><main class='form-signin'> <form action='/' method='post'> <h1 class=''>Wifi Setup</h1><br/><div class='form-floating'><label>SSID</label><input type='text' class='form-control' name='ssid'> </div><div class='form-floating'><br/><label>Password</label><input type='password' class='form-control' name='password'></div><br/><br/><button type='submit'>Save</button><p style='text-align: right'><a href='https://www.smartKont.ca' style='color: #32C5FF'>smartKont.ca</a></p></form></main> </body></html>" );
  }
}

// Manual Portal
void eromGetandWifiConnect () {
  EEPROM.begin(sizeof(struct settings) );
  EEPROM.get( 0, user_wifi ); // Get userId and password from EROM

  WiFi.mode(WIFI_STA);
  WiFi.begin(user_wifi.ssid, user_wifi.password);

  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (tries++ > 30) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("Setup Portal", "smartKont.ca");
      break;
    }
  }
  server.on("/",  handlePortal);
  server.begin();

}

// 
bool startConfigPortal_L(){
    WiFi.mode(WIFI_STA); // Setting up ESP8266 at station mode
    // Change color
    wifiManager.setConfigPortalTimeout(120);
    //setColor(255,165,0);// Orange
    setColor(195,82,20);// Orange
    bool res = wifiManager.startConfigPortal(ssid);
    setColor(0,0,0);
    Serial.println("******************RES config Portal :");
    Serial.print(res);
    return res;
}




// Connect to Wifi network
bool wifiConnect() {
  Serial.begin(115200);  
  
  //Serial.println("******************In Wifi!");
  

  int configPortalTryCnt = 0; // Number of times wifi config portal entry done wrong by user if more than 3 give up.
  int wifiConnetTryCnt = 0; // Config portal is correct but not able to connect to wifi weak signal or some technical issue, try 2 times before give up.
  bool wifiConnectedFlag = false; // Wifi connected / Not
  bool configPortalSetupflag = false; // ConfigPortal entry correct/ Not
  
  // No wifi id is setup first time connection
  //Serial.print("******************Saved SSID:");
  //Serial.println(wifiManager.getConfigPortalSSID());
  //Serial.print("******************WIFI Saved:");
  //Serial.println(wifiManager.getWiFiIsSaved());
  if(!wifiManager.getWiFiIsSaved()){// First time no access point is setup
    //Serial.println("******************No Net!");
    if(wifiManager.getConfigPortalActive()){ // Config Portal is active
      //Serial.println("******************Config Portal active!");
      if(wifiManager.stopConfigPortal()) { // Stop the config Portal
        //Serial.println("******************Config Portal stopped!");
        delay(2000); // wait 2 seconds before starting again.
        configPortalSetupflag = startConfigPortal_L(); // Start the config Portal
      }
    } else {
      //Serial.println("******************First setup!!");
      configPortalTryCnt++; 
      configPortalSetupflag = startConfigPortal_L(); // Config Portal started

    } // End for First time access point
  } else { // Access point is already saved.
    configPortalSetupflag = true;
    //Serial.println("******************AP saved config potal true!!");
  }

  uint32_t setupProcessTime=millis(); 

  do{
    uint32_t wifiConnectTime=millis(); // Wait for connection after config portal is setup

    if(configPortalSetupflag){// ConfigPortal setup is successful
      //Serial.println("******************Config portal true before wifi connection!");
      WiFi.begin();
      while (WiFi.status() != WL_CONNECTED && (millis()-wifiConnectTime<90000))  { // Wait for 1.5 mins each time before giving up.
        Serial.print(".");
        //yield();
        delay(100);
      }
      // Set the minimum signal quality default 8%
      //wifiManager.setMinimumSignalQuality();
      if(WiFi.status() == WL_CONNECTED){ // Wifi got connected.
          //Serial.println("******************Wifi connected!!");
          byte mac[6];
          WiFi.macAddress(mac);

         macAddress = String(mac[5], HEX) +(":") + 
                      String(mac[4], HEX) +(":") + 
                      String(mac[3], HEX) +(":") + 
                      String(mac[2], HEX) +(":") + 
                      String(mac[1], HEX) +(":") + 
                      String(mac[0], HEX) +(":") ;
          Serial.println("**macAddress : ");
          Serial.println(macAddress);
          wifiConnectedFlag = true;
      } else { // If not connected with saved accesspoint then start the config portal again.
        wifiConnetTryCnt++;
        //Serial.println("******************Issue with wifi connection Portal is ok");
        configPortalSetupflag = startConfigPortal_L();
        // Check again if wifi is connect or not ??
      }


    } else { // This will be initialted when after setting the initial configPortal sucessfully wifi is not connected and hence config portal started again and user entered wrong credentials or user enter wrong credentials in first setup
        configPortalTryCnt++; // 3 tries
        //Serial.println("******************Portal connection is wrong");
        configPortalSetupflag = startConfigPortal_L();
    }

  }while((!wifiConnectedFlag) && (configPortalTryCnt<3) && (wifiConnetTryCnt<2) && ((millis()-setupProcessTime)<900000));

  // User left the setup running ESP will go to deep sleep and user need to reset using physical reset button
  if((millis()-setupProcessTime)>900000) { // 12 mins
    //Serial.println("******************Lot of time passed going to deep sleep press reset!!!");
    deepSleep(0);
  }

  return wifiConnectedFlag;

}


// Connect to Wifi manual portal
bool wifiConnectManualPortal() {
  Serial.begin(115200);  
  
  //Serial.println("******************In Wifi!");
  

  int configPortalTryCnt = 0; // Number of times wifi config portal entry done wrong by user if more than 3 give up.
  int wifiConnetTryCnt = 0; // Config portal is correct but not able to connect to wifi weak signal or some technical issue, try 2 times before give up.
  bool wifiConnectedFlag = false; // Wifi connected / Not
  bool configPortalSetupflag = false; // ConfigPortal entry correct/ Not
  
  // No wifi id is setup first time connection
  //Serial.print("******************Saved SSID:");
  //Serial.println(wifiManager.getConfigPortalSSID());
  //Serial.print("******************WIFI Saved:");
  //Serial.println(wifiManager.getWiFiIsSaved());
  if(!wifiManager.getWiFiIsSaved()){// First time no access point is setup
    //Serial.println("******************No Net!");
    if(wifiManager.getConfigPortalActive()){ // Config Portal is active
      //Serial.println("******************Config Portal active!");
      if(wifiManager.stopConfigPortal()) { // Stop the config Portal
        //Serial.println("******************Config Portal stopped!");
        delay(2000); // wait 2 seconds before starting again.
        configPortalSetupflag = startConfigPortal_L(); // Start the config Portal
      }
    } else {
      //Serial.println("******************First setup!!");
      configPortalTryCnt++; 
      configPortalSetupflag = startConfigPortal_L(); // Config Portal started

    } // End for First time access point
  } else { // Access point is already saved.
    configPortalSetupflag = true;
    //Serial.println("******************AP saved config potal true!!");
  }

  uint32_t setupProcessTime=millis(); 

  do{
    uint32_t wifiConnectTime=millis(); // Wait for connection after config portal is setup

    if(configPortalSetupflag){// ConfigPortal setup is successful
      //Serial.println("******************Config portal true before wifi connection!");
      WiFi.begin();
      while (WiFi.status() != WL_CONNECTED && (millis()-wifiConnectTime<90000))  { // Wait for 1.5 mins each time before giving up.
        Serial.print(".");
        //yield();
        delay(100);
      }
      // Set the minimum signal quality default 8%
      //wifiManager.setMinimumSignalQuality();
      if(WiFi.status() == WL_CONNECTED){ // Wifi got connected.
          //Serial.println("******************Wifi connected!!");
          byte mac[6];
          WiFi.macAddress(mac);

         macAddress = String(mac[5], HEX) +(":") + 
                      String(mac[4], HEX) +(":") + 
                      String(mac[3], HEX) +(":") + 
                      String(mac[2], HEX) +(":") + 
                      String(mac[1], HEX) +(":") + 
                      String(mac[0], HEX) +(":") ;
          Serial.println("**macAddress : ");
          Serial.println(macAddress);
          wifiConnectedFlag = true;
      } else { // If not connected with saved accesspoint then start the config portal again.
        wifiConnetTryCnt++;
        //Serial.println("******************Issue with wifi connection Portal is ok");
        configPortalSetupflag = startConfigPortal_L();
        // Check again if wifi is connect or not ??
      }


    } else { // This will be initialted when after setting the initial configPortal sucessfully wifi is not connected and hence config portal started again and user entered wrong credentials or user enter wrong credentials in first setup
        configPortalTryCnt++; // 3 tries
        //Serial.println("******************Portal connection is wrong");
        configPortalSetupflag = startConfigPortal_L();
    }

  }while((!wifiConnectedFlag) && (configPortalTryCnt<3) && (wifiConnetTryCnt<2) && ((millis()-setupProcessTime)<900000));

  // User left the setup running ESP will go to deep sleep and user need to reset using physical reset button
  if((millis()-setupProcessTime)>900000) { // 12 mins
    //Serial.println("******************Lot of time passed going to deep sleep press reset!!!");
    deepSleep(0);
  }

  return wifiConnectedFlag;

}



// Distance measurement
float distanceMeasure() {
  //Serial.println("Insite distance measurement");
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  distance = duration/2*.0343; // As of now cosidering speed of sound in dry air (humidity factor not considered)

  if(distance>=400 || distance <=2) {
    Serial.println("Out of range");
    distance=-1000; // -1000 distance when out of scope
  }
return distance;
}

// Create http request for container data
String createHttpRequestData (String apikeyValue_f, float distanceValue_f, String macAddress_f) {
  //******** String class uses lot of memory try to replace option SafeString
  String postApiKey="api_key=";
  String postDistance="&distance=";
  String postMacAddress = "&mac=";

  String httpRequestData_f = postApiKey + apikeyValue_f + postDistance + distanceValue_f + postMacAddress + macAddress_f ;
return httpRequestData_f;
}

// Send http request to web server for container data
bool sendHttpRequestData () {
    HTTPClient http;    //Declare object of class HTTPClient
    WiFiClient client;  // Object for WiFi Client

    http.begin(client, serverNamePath); //Specify request destination
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //Specify content-type header

    // Get from sensor
    float measuredDistance = distanceMeasure();
	
    //Serial.println("macAddress:");
    //Serial.print(macAddress);

    String httpRequestData = createHttpRequestData(apiKeyValue,measuredDistance, macAddress);
    //Serial.println(httpRequestData);
    //Serial.println(httpRequestData.c_str());
    //Serial.print("Wifi Status");
    //Serial.println(WiFi.status());
    
    // Try sending the data 3 time of error out for any issues.
    int httpSendCounter=0;
    int httpCode;
    bool httpReturnCode = false;
    while(!httpRequestFlag) { // Try multiple time mainely in case of Serve issue.

      //Serial.print("httpRequestFlag:");
      //Serial.println(httpRequestFlag);
      httpCode = http.POST(httpRequestData.c_str());   //Send the request after changing the String object to char*
      //Serial.print("HTTP Code");
      //Serial.println(httpCode);
      if(httpCode == 200) {
        httpRequestFlag=true;
        // Blink for .5 seconds when request is sent successfully 
       // blink(0, 0 , 255 , 1);
        Serial.print("HTTP Response code: ");
        Serial.println(http.getString());    //Print request response payload
        //http.end();  //Close connection
        httpReturnCode = true;
      }
      else { // If response not received program will try three times and then print the error of last error.
          httpSendCounter++;
          if (httpSendCounter>2) {
              httpRequestFlag=true; // Try three time after that giveup.
              //blink(255, 0 , 0 , 3); // 3 RED Blink meaning some issue with the webserver.
              Serial.print("Error code: ");
              Serial.println(httpCode);
              Serial.print("HTTP Response code: ");
              Serial.println(http.getString());    
              http.end();  //Close connection
              httpReturnCode = true; //false; Hack to have it deep sleep mode
           }
       }
    }
    http.end();  //Close connection
    return httpReturnCode;
}

void blink(int b_RED, int b_GREEN, int b_BLUE, int b_times){
  int b_time = 0;
  while (b_time < b_times) {
    setColor(b_RED,b_GREEN,b_BLUE); 
	  delay(250);
    setColor(0,0,0);
    delay(250);
    b_time++;
  }

} 

// ESP restart
void restartESP(){
    ESP.restart();
}

// ESP restart
void resetESP(){
    ESP.reset();
}

void setupSensorPinModes() {
  // Define Sensor Pin modes
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
}

// Make the board in deep sleep mode to conserve the battery
void deepSleep(const int sleepTimeSecs) {
  //Serial.begin(115200);
  // Code to sleep mode.
  Serial.println("Going to deep sleep mode");
  int sleepTimeSecsCal = sleepTimeSecs * 1000000;
  setColor(0,0,0);
  ESP.deepSleep(sleepTimeSecsCal);
  Serial.println("Wake up!");
}

// Define LED Pin modes
void setupRGBLEDPinModes() {
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
}


//Set the color of LED
void setColor(int RED, int GREEN, int BLUE) {
  analogWrite(PIN_RED, RED);
  analogWrite(PIN_GREEN, GREEN);
  analogWrite(PIN_BLUE, BLUE);
 /* Serial.println("**************************************");
  Serial.println("************* Set Color called");
  Serial.print("************* Color code RED:");
  Serial.println(RED);
  Serial.print("************* Color code GREEN:");
  Serial.println(GREEN);
  Serial.print("************* Color code BLUE:");
  Serial.println(BLUE);
  Serial.println("**************************************");
  */
}

// Create http request for container setp
String createHttpRequestDataKontSetup (String apikeyValueKontSetup, String userNameKontSetup_s, String macAddressKontSetup) {
  //******** String class uses lot of memory try to replace option SafeString
  String postApiKeyKontSetup="api_key=";
  String postUserName="&userName="; // in case @ have any issues
  String postMacAddressKontSetup = "&mac=";

  String httpRequestData_s = postApiKey + apikeyValueKontSetup + postUserName + userNameKontSetup_s + postMacAddress + macAddressKontSetup ;
return httpRequestData_s;
}

// Send http request to web server for container data
bool sendHttpRequestDataKontSetup () {
    HTTPClient httpKontSetup;    //Declare object of class HTTPClient
    WiFiClient clientKontSetup;  // Object for WiFi Client

    httpKontSetup.begin(clientKontSetup, serverKontSetupPath); //Specify request destination
    httpKontSetup.addHeader("Content-Type", "application/x-www-form-urlencoded"); //Specify content-type header

    // Get from ESP8266 server
    String userNameKontSetup =''; //Get from EROM
	
    //Serial.println("macAddress:");
    //Serial.print(macAddress);

    String httpRequestDataKontSetup = createHttpRequestDataKontSetup(apiKeyValue,userNameKontSetup, macAddress);
    //Serial.println(httpRequestData);
    //Serial.println(httpRequestData.c_str());
    //Serial.print("Wifi Status");
    //Serial.println(WiFi.status());
    
    // Try sending the data 3 time of error out for any issues.
    int httpSendCounter=0;
    int httpCode;
    bool httpReturnCode = false;
    while(!httpRequestFlag) { // Try multiple time mainely in case of Serve issue.

      //Serial.print("httpRequestFlag:");
      //Serial.println(httpRequestFlag);
      httpCode = httpKontSetup.POST(httpRequestDataKontSetup.c_str());   //Send the request after changing the String object to char*
      //Serial.print("HTTP Code");
      //Serial.println(httpCode);
      if(httpCode == 200) {
        httpRequestFlag=true;
        // Blink for .5 seconds when request is sent successfully 
       // blink(0, 0 , 255 , 1);
        Serial.print("HTTP Response code: ");
        Serial.println(httpKontSetup.getString());    //Print request response payload
        //http.end();  //Close connection
        httpReturnCode = true;
      }
      else { // If response not received program will try three times and then print the error of last error.
          httpSendCounter++;
          if (httpSendCounter>2) {
              httpRequestFlag=true; // Try three time after that giveup.
              //blink(255, 0 , 0 , 3); // 3 RED Blink meaning some issue with the webserver.
              Serial.print("Error code: ");
              Serial.println(httpCode);
              Serial.print("HTTP Response code: ");
              Serial.println(httpKontSetup.getString());    
              http.end();  //Close connection
              httpReturnCode = true; //false; Hack to have it deep sleep mode
           }
       }
    }
    httpKontSetup.end();  //Close connection
    return httpReturnCode;
}


// Send Container setup request
void containerSetup() {
  // Wifi id password saved
  // Create PHP to setup container
  // Send the request
  // Request will have mac address, client id, container name
  // EROM to be used for saving user id/
 bool requestKontSetupStat = sendHttpRequestDataKontSetup();
  

}



void setup() {
  Serial.begin(115200);
  //Serial.println("Inside 8266");

  // Set Input output pins
  setupSensorPinModes();
  // LED pin setup
  setupRGBLEDPinModes();
 // WiFi.persistent(true);
  //WiFi.setAutoConnect(true);
  //WiFi.setAutoReconnect(true);
}




void loop() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    bool requestStat = sendHttpRequestData();
    if (requestStat) { // If request is sent then go to deepSleep mode.
      deepSleep(sleepTimer); //Send a request every x seconds
    }
    // This is just a test to see how it wakeup.
    //Serial.begin(115200);

    //Serial.println("Wake UP **");
    //delay(10000);
    //Serial.println("Wake UP *****************");
 
  } else {
    // Try to reconnect the wifi
    //WiFi.disconnect(); // Disconnect
    //Serial.println("Wifi reconnecting");
    //delay(1000); // 1 Second delay
    if(!wifiConnect()){// Connecting to wifi, if not connected
      wifiNotConnCntr++; // increase how much time wifi is not connected since chip started
    } 
  }
 Serial.print("wifiNotConnCntr :");
 Serial.println(wifiNotConnCntr);
 if(wifiNotConnCntr>=2) { // Restart ESP if wifiConnect is not working for 2 time in a sequence.
    Serial.println("Restarting ESP");
    restartESP();
  } // Saving something to permanent memory so it doesn't go in infinite loop after number of retiries to save batteries.

 
}