#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>


// #include <WiFiManager.h>
/*
Legal statement this code is the property of Financial Minds Consulting Ltd. Canada making any copy of the code or using any art of the code without written approval from the owner of Financial Minds Consulting Ltd is not permitted.

*/

/* To do 
1. Put the ultrasonic sensor to sleep mode. DONE
2. LED light display as needed on different action like setup mode, normal function mode etc. DONE
3. LED blink  DONE
    Single blink once sending the data to server (distance) green or blue
    Red blink 1 in every 5 seconds when setup is DONE
    Red blink 2 when sencinding the data to server and have issues
4. ESP light to put off. GPIO2 pin 
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
5. Wifi password problem DONE


6. Erase the EEROM to factory reset using button or somehow.
7. WIFI ssid changed or put off for some time
8. Bbattery check indicator


*/


const char* apiKeyValue = "SmartDabba!ndCan!001";

// Local path
//const char* initialSetupPath = "http://142.126.144.158/learnTest/containerSetup.php";
//const char* serverNamePath = "http://142.126.144.158/learnTest/containerInsert.php";

const char* initialSetupPath  = "http://deviceconnect.pdq2.com/containerSetup.php";
const char* serverNamePath = "http://deviceconnect.pdq2.com/containerInsert.php";

String macAddress;
const char* eeromCheck = "SK" ;
const char* eeromClear = "CL" ;

const String codeChipVersion = "1.0.0" ;

ESP8266WebServer    server(80);

// Structure to store to EEROM
struct settings {
  //char init[2] =  ["NI"];
  char init[3];
  char userWifiId[30];
  char password[30];
  char containerId[50];
  bool kontSetupflag;
  //int ssidnotavailCntr;
} user_info = {};


  // Local vairables 
  char initLocal[3]; // Not initialized
  char userWifiIdLocal[30];
  char passwordLocal[30];
  //char passwordLcl[30] = "NI";
  char containerIdLocal[30];
  bool kontSetupflagLocal;


// Process controlling variables
int wifiNotConnCntr = 0; // Number of time wifi is not connected for consecutive times.
int httpRequestCntr = 0; // Number of time wifi is not connected for consecutive times.
bool httpRequestFlag = false; // https Send Request Flag, control the number of times http request os sent.
bool wifiSavedFlag = false; // Flag to initialize wifi is saved in EEROM
bool wifiWrongpasswdOrSSID = false; // Flag to initialize wifi is saved in EEROM
bool wifiConnected = false; // Flag to initialize wifi is saved in EEROM
const int sleepTimer=120; // ESP sleep timer
const int configPortalsleepTimerHours=365; // ESP sleep for 15 days  (effectively until reset is clicked manually)
const char *ssid = "SmartKont"; // ESP advertise for Config Portal.
const char *password = "987654321"; // Not used

// Control the timing of config portal sleep the ESP for 15 days or until reset buttons i clicked
uint32_t configPortalTimeControl; 


// Sensor pin definition for distance calculation
#define trigPin D5
#define echoPin D6
#define snsrPwrPin D7

// LED pin definition for illumination
//const int PIN_RED   = D2; 
//const int PIN_GREEN = D1; 
//const int PIN_BLUE  = D7; 

const int PIN_GREEN = D2;
const int PIN_RED = D1;




// Sonic sensor distance and duration measurement
long  duration;
int distance;


// May be useful in container Setup
//AutoConnectConfig acConfig;
//acConfig.apid = "ESP-" + String(ESP.getChipId(), HEX);

// Wifimanager for connection and config Portal
//WiFiManager wifiManager;

//Work here using the generic code
/*bool startConfigPortal_L(){
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
*/

// Start New config Portal
bool startConfigPortalCustom(){
    // Config portal will be up for only 2.5 mins else it will eat up the battery in case user leave it open
    //Serial.println("startConfigPortalCustom inside");
    WiFi.mode(WIFI_AP); // Setting up ESP8266 at access point mode
    // Change color
    //bool res = WiFi.softAP("smartKont", "123456789"); // Return value?
    bool res = WiFi.softAP("smartKont" + codeChipVersion); // Random number ??
    // softAPdisconnect Explore
    server.on("/",  handlePortal);
    server.begin();
    //Serial.print("RES config Portal :");
    //Serial.println(res);
    //Serial.println("startConfigPortalCustom END");
    return res;
}


bool stopConfigPortalCustom(){
    // Stop the config portal if running
    //bool stopRes = server.stop();
    server.stop();
    WiFi.mode(WIFI_STA); //Setting up ESP8266 at station mode
    return true;
}

void eeromVarCopy() {
  //Serial.begin(115200);
  //Serial.println("Inside eeromVarCopy");
  EEPROM.begin(sizeof(struct settings) );
  EEPROM.get(0, user_info );
  //strncpy(user_info.init,  eeromCheck, sizeof(user_info.init) );

  //initLocal = user_info.init;
  //userWifiIdLocal = user_info.userWifiI;
  //passwordLocal = user_info.password;
  //containerIdLocal = user_info.containerId;
  //Serial.println("***1***");
  //Serial.print("user_info.init eeromVarCopy:");
  //Serial.print(user_info.init);
  //Serial.println(":");
  //Serial.print("user_info.userWifiId eeromVarCopy:");
  //Serial.print(user_info.userWifiId);
  //Serial.println(":");
  //Serial.print("user_info.password eeromVarCopy:");
  //Serial.print(user_info.password);
  //Serial.println(":");
  //Serial.print("user_info.containerId eeromVarCopy:");
  //Serial.print(user_info.containerId);
  //Serial.println(":");
  //Serial.print("user_info.kontSetupflag eeromVarCopy:");
  //Serial.print(user_info.kontSetupflag);
  //Serial.println(":");

  
  strlcpy(initLocal, user_info.init, sizeof(user_info.init));
  strlcpy(userWifiIdLocal, user_info.userWifiId, sizeof(user_info.userWifiId));
  strlcpy(passwordLocal, user_info.password, sizeof(user_info.password));

  //strncpy(passwordLcl,  user_info.password, sizeof(user_info.password));
  /*strncpy(userWifiIdLocal, user_info.password, sizeof(user_info.password));

  strncpy(passwordLcl, user_info.userWifiId, sizeof(user_info.userWifiId));
  strncpy(passwordLocal, user_info.userWifiId, sizeof(user_info.userWifiId));
  */
  
  //-- strncpy(passwordLocal, "swap!001", 30);// hack for now
  strlcpy(containerIdLocal, user_info.containerId, sizeof(user_info.containerId));

  kontSetupflagLocal = user_info.kontSetupflag;


  //Serial.print("initLocal:");
  //Serial.println(initLocal);

  //Serial.print("userWifiIdLocal:");
  //Serial.println(userWifiIdLocal);

  //Serial.print("passwordLcl:");
  //Serial.println(passwordLcl);

  //Serial.print("passwordLocal:");
  //Serial.println(passwordLocal);

  //Serial.print("user_info.password eeromVarCopy:");
  //Serial.println(user_info.password);
  

  //Serial.print("passwordLength:");
  //Serial.println(sizeof(user_info.password));

  //Serial.print("containerIdLocal:");
  //Serial.println(containerIdLocal);

  //Serial.print("kontSetupflagLocal:");
  //Serial.println(kontSetupflagLocal);
}

bool getWiFiIsSavedCustom(){
 bool wifiPasswdSaved = false;
 //EEPROM.begin(sizeof(struct settings) );
 //EEPROM.get( 0, user_info );
 //Serial.print("user_info.init :");
 //Serial.print(user_info.init);
 //Serial.println(":");
 //Serial.print("user_info.userWifiId :");
 //Serial.print(user_info.userWifiId);
 //Serial.println(":");
 //Serial.print("user_info.password :");
 //Serial.print(user_info.password);
 //Serial.println(":");
 //Serial.print("strncmp(user_info.init,SK,2) :");
 //Serial.print(strncmp(user_info.init,"SK",2) );
 //Serial.println(":");
 //if ( strncmp(initLocal, "SK",2) == 0 ) { // That mean Wifi id passwd is saved at least once
  //Serial.print("strlen(userWifiIdLocal) : ");
  //Serial.println(strlen(userWifiIdLocal)); // Meaning no userId saved

  //Serial.print("strlen(passwordLocal) : ");
  //Serial.println(strlen(passwordLocal)); // Meaning no passwrod save


  //Serial.print("strlen(containerIdLocal) : ");
  //Serial.println(strlen(containerIdLocal)); // Meaning no userId saved


 if ( strncmp(initLocal, "SK",2) == 0 && !(strlen(passwordLocal) == 0) && !(strlen(userWifiIdLocal) == 0) && !(strlen(containerIdLocal) == 0)) { // That mean Wifi id passwd is saved at least once
  wifiPasswdSaved=true;
 } else {
  wifiPasswdSaved=false;
 }
//Serial.print("wifiPasswdSaved:");
//Serial.print(wifiPasswdSaved);
//Serial.println(":");
return wifiPasswdSaved;

}

String scanNetworks () {
  //Serial.begin(115200);
  String networksHTML = "<select name='userWifiId' class='form-control'>";

  int numberOfNetworks = WiFi.scanNetworks();

  for(int i =0; i<numberOfNetworks; i++){

    networksHTML += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";

      //Serial.print("Network name: ");
      //Serial.println(WiFi.SSID(i));
      //Serial.print("Signal strength: ");
      //Serial.println(WiFi.RSSI(i));
      //Serial.println("-----------------------");

  }
  networksHTML += "</select>";
  return networksHTML;

}


// Connect to Wifi network
bool wifiConnect() {
  //Serial.begin(115200);  
  //Serial.println("Inside wifi connect");
  
  int wifiConnectTryCnt = 0; // Config portal is correct but not able to connect to wifi weak signal or some technical issue, try 2 times before give up.
  bool configPortalSetupflag = false; // Not used // ConfigPortal entry correct/ Not
  
  if(!getWiFiIsSavedCustom() || wifiWrongpasswdOrSSID){
  // First time no access point is setup EEROM check or wrong password was saved in earlier try or Wifi password changed
    //Serial.println("Wifi id or passwd not saved or wrong password is saved");
      if(stopConfigPortalCustom()) { // Always retrun true if code changed then else condition needs to be created.
      // Stop the config Portal Custom
        //Serial.println("stopConfigPortalCustom");
        delay(2000); // wait 2 seconds before starting again.
        wifiSavedFlag = false; // Config portal started meaning either wifi id/passwd not saved or it is wrong.
        configPortalSetupflag = startConfigPortalCustom(); // Start the config Portal Custom
        //if(configPortalSetupflag){ ** Do we need to blink the LED?
        // config portal started then blink three times
          //blinkLED("red", 3);
        //}
        //Serial.print("ConfigPortalSetupflag:");
        //Serial.print(configPortalSetupflag);
      }
  } else { // Access point is already saved.
    //Serial.println("Wifi id/passwd saved");
    configPortalSetupflag = true;
    wifiSavedFlag = true;
  }

  uint32_t setupProcessTime=millis(); 

  if(wifiSavedFlag) {
      //Serial.println("Inside wifiSavedFlag");
      do{
        uint32_t wifiConnectTime=millis(); // Wait for connection after config portal is setup
        //Serial.print("Do Loop wifiSavedFlag:");
        //Serial.print(wifiSavedFlag);

          // Only if local copy doesn't work
          //EEPROM.begin(sizeof(struct settings) );
          //EEPROM.get( 0, user_info );
          //Serial.println("***WIFI**");
          //Serial.print(userWifiIdLocal);
          //Serial.println("*****");
          //Serial.print("***PASSWD_LOCAL**");
          //Serial.println(passwordLocal);
          //Serial.println("*****");
          //Serial.println("*****");

          WiFi.mode(WIFI_STA); // Change to Station mode
          WiFi.begin(userWifiIdLocal, passwordLocal);
          
          //Serial.println("WiFi.status()");
          //Serial.print(WiFi.status());
            /* 3.1.2 not very stable
                WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
                WL_IDLE_STATUS      = 0,
                WL_NO_SSID_AVAIL    = 1,
                WL_SCAN_COMPLETED   = 2,
                WL_CONNECTED        = 3,
                WL_CONNECT_FAILED   = 4,
                WL_CONNECTION_LOST  = 5,
                WL_WRONG_PASSWORD   = 6,
                WL_DISCONNECTED     = 7
            */
          while (WiFi.status() != WL_CONNECTED && !(wifiWrongpasswdOrSSID) && (millis()-wifiConnectTime<60000))  { // Wait for 60 second each time before giving up.
            
            //if(WiFi.status() == WL_WRONG_PASSWORD || WiFi.status() == WL_NO_SSID_AVAIL) {
            //Serial.print(".");
            //Serial.print(WiFi.status()); // This should be commented
            if(WiFi.status() == WL_NO_SSID_AVAIL) {
              Serial.println("SSID not available, possible wifi connection lost!");
              // Break the loop and go to sleep mode
              wifiConnectTryCnt++; // if wifi is not avail then only try twice which is controled by retry counter at the setup function.
              break; // Break while loop
              
              // Try in 15 mins for 1 hour if it doesn't work then start the config portal and ask user to setup id/passwd again--- ?? needs to be coded

            }
            if (WiFi.status() == WL_CONNECT_FAILED) { // Wrong passwd main cause
              Serial.println("Wifi connection failed, start the config portal");

              //strncpy(user_info.init,  eeromClear, sizeof(user_info.init) );// Not needed as below flag will bring the portal up 
              
              // restart ESP after eerom edit?
              
              wifiWrongpasswdOrSSID = true;
              break;
              
            }
            //yield();
            delay(1000); // 1 second delay in checking the status
          }
          // Set the minimum signal quality default 8%
          //wifiManager.setMinimumSignalQuality();
          if(WiFi.status() == WL_CONNECTED){ // Wifi got connected.
                Serial.println("Wifi connected!!");
                wifiConnected = true;
                byte mac[6];
                WiFi.macAddress(mac);

                macAddress = String(mac[5], HEX) +(":") + 
                            String(mac[4], HEX) +(":") + 
                            String(mac[3], HEX) +(":") + 
                            String(mac[2], HEX) +(":") + 
                            String(mac[1], HEX) +(":") + 
                            String(mac[0], HEX) +(":") ;
                //Serial.print("**macAddress : ");
                //Serial.println(macAddress);
              } else { // Increase the counter to try the wifi again
                          wifiConnectTryCnt++;
                    } 
      } while((!wifiConnected) && (wifiConnectTryCnt<2) &&  !(wifiWrongpasswdOrSSID) && ((millis()-setupProcessTime)<90000)); // 900000??
    }
  //Serial.println("Inside wifi connect END!");
  return wifiConnected;
}

// Sensor Gnd pin high to wake up
void sensrPwrPinReady() {
  digitalWrite(snsrPwrPin, HIGH);
}

// Sensor Gnd pin LOW to sleep
void sensrPwrPinSleep() {
  digitalWrite(snsrPwrPin, LOW);
}

// Remove white spaces
String removeSpaces(String str) {
  String result = "";
  
  for (int i = 0; i < str.length(); i++) {
    // Check if the character is not a space
    if (str.charAt(i) != ' ') {
      result += str.charAt(i);
    }
  }

  return result;
}


// Distance measurement
float distanceMeasure() {
  //Serial.println("Insite distance measurement");

  setupSensorPinModes();
  sensrPwrPinReady();
  delay(75); // 10 Seconds delay to start .. Needed ? to have the seonsor ready for the function
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  // Put the sensor to sleep as soon distance is measured
  sensrPwrPinSleep();
  
  distance = duration/2*.0343; // As of now cosidering speed of sound in dry air (humidity factor not considered)

  if(distance>=400 || distance <=2) {
    Serial.println("Out of range");
    distance=-1000; // -1000 distance when out of scope
  }
return distance;
}

// Create http request
String createHttpRequestData (String apikeyValue_f, float distanceValue_f, String  macAddress_f) {
  //******** String class uses lot of memory try to replace option SafeString
  String postApiKey="api_key=";
  String postDistance="&distance=";
  String postMacAddress = "&mac=";
  String postDeviceId = "&deviceId=";
  String httpRequestData_f = postApiKey + apikeyValue_f + postDistance + distanceValue_f + postDeviceId + containerIdLocal + postMacAddress + macAddress_f  ;
  return httpRequestData_f;
}


// Create http request container Setup
String createHttpRequestSetup (String apikeyValue_f, String userContainerId, String  macAddress_f, float measuredDistance_f) { // This is if findout what size of container is
//String createHttpRequestSetup (String apikeyValue_f, String userContainerId, String  macAddress_f) {
  //******** String class uses lot of memory try to replace option SafeString
  String postApiKey="api_key=";
  String postContainerId="&containerId=";
  String postMacAddress = "&mac=";
  String postDistance="&distance=";
  //userContainerId = 1000;

  String httpRequestData_f = postApiKey + apikeyValue_f + postContainerId + userContainerId + postMacAddress + macAddress_f + postDistance + measuredDistance_f ;
  //String httpRequestData_f = postApiKey + apikeyValue_f + postContainerId + userContainerId + postMacAddress + macAddress_f ;
  return httpRequestData_f;
}

// Send http request to web server
bool sendHttpRequestData () {
    HTTPClient http;    //Declare object of class HTTPClient
    WiFiClient client;  // Object for WiFi Client

    String httpRequestData;
    //Serial.print("kontSetupflagLocal");
    //Serial.println(kontSetupflagLocal);
    
    if (!kontSetupflagLocal){
      Serial.println("kontSetupflagLocal is false");
      http.begin(client, serverNamePath); //Request destination
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //content-type header
      // Get from sensor
      float measuredDistance = distanceMeasure();
      httpRequestData = createHttpRequestData(apiKeyValue,measuredDistance, macAddress);
    } else {
      Serial.println("kontSetupflagLocal is true");

      // Container setup request to code

      //EEPROM.begin(sizeof(struct settings) );
      //EEPROM.get( 0, user_info );
      float measuredDistance = distanceMeasure(); // Initial setup empty container distance measurement.
      // One time to setup the flag
      http.begin(client, initialSetupPath); //Request destination
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //content-type header
      httpRequestData = createHttpRequestSetup(apiKeyValue,containerIdLocal, macAddress, measuredDistance);
      //httpRequestData = createHttpRequestSetup(apiKeyValue,containerIdLocal, macAddress);
    }
    
    
    //int httpSendCounter=0; // Try sending the data 2 times if not successful in first time
    int httpCode;
    bool httpReturnCode = false;

    //while(!httpRequestFlag) { // Try multiple time mainely in case of Server issue.

      Serial.print ("httpRequestData:");
      Serial.println (httpRequestData.c_str());

      httpCode = http.POST(httpRequestData.c_str());   //Send the request after changing the String object to char*
      //Serial.print("HTTP Code");
      //Serial.println(httpCode);

      if(httpCode == 200) {
        // Payload 1 Success and 0 Failure
       const String& payload = removeSpaces(http.getString());
       
       Serial.print("HTTP Response code:");
       Serial.print(payload);    //Print request response payload
       Serial.println(":");

        if(!kontSetupflagLocal) {
          if(payload == "1") {
              //httpRequestFlag=true;
              blinkLED("green", 1);
              httpReturnCode = true;              
            } else {
              Serial.println("System will retry automatically in next retry!");
              //httpSendCounter++;
              blinkLED("red", 1);
            }
        } else if (kontSetupflagLocal){// kont is not setup yet
            if(payload == "1") {
              Serial.println("Change the flag in EEROM that container is setup");
              // SET EEROM Boolean variable to True conntainer set up complete ** CHANGE
              user_info.kontSetupflag = false; 
              EEPROM.put(0, user_info);
              EEPROM.commit(); // Change the kontflag only
              httpReturnCode = true;
              blinkLED("green", 3); // 3 Green flashes meaning kont is setup

            } else {
              Serial.println("Retry: Ask user to press reset button after 1 min to update the mac address and containerId");
              //httpRequestFlag=true;
              //httpSendCounter++;
              blinkLED("red", 3); // 3 red blink if container is not setup
            }
        }

      }
      else { // If response not received program will try two times and then print the error of last error.
          //httpSendCounter++;
          //if (httpSendCounter>=2) { // Two tries to send the data.
            //  httpRequestFlag=true; 
              //blink(255, 0 , 0 , 3); // 3 RED Blink meaning some issue with the webserver.
              blinkLED("red", 1);
              // Blink Red LED
              Serial.print("Error code: ");
              Serial.println(httpCode);
              //http.end();  //Close connection
              //httpReturnCode = true; //false; Hack to have it deep sleep mode
           //}

       }
       
    //} // Do we need a retry ??
    http.end();  //Close connection
    return httpReturnCode;
}

/*void blink(int b_RED, int b_GREEN, int b_BLUE, int b_times){
  int b_time = 0;
  while (b_time < b_times) {
    setColor(b_RED,b_GREEN,b_BLUE); 
	  delay(250);
    setColor(0,0,0);
    delay(250);
    b_time++;
  }

} 
*/

void blinkLED(String color, int b_times){
  //Serial.print("Blink LED color:");
  //Serial.println(color);
  //Serial.print("Blink LED b_times:");
  //Serial.println(b_times);
  int b_time = 0;
  if(color == "red") {
    while (b_time < b_times) {
      // Red Blink more steady and bold
      analogWrite(PIN_RED, 255);
      delay(1000);
      analogWrite(PIN_RED, 0);
      b_time++;
    }
   }
  else if (color == "green"){
    while (b_time < b_times) {
      analogWrite(PIN_GREEN, 255);
      delay(100);
      analogWrite(PIN_GREEN, 0);
      b_time++;
    }
   }
    
  
} 

void shutLED(){
  analogWrite(PIN_RED, 0);
  analogWrite(PIN_GREEN, 0);
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

  pinMode(echoPin,INPUT);
}

// Make the board in deep sleep mode to conserve the battery
void deepSleep(const int sleepTimeSecs) {
  //Serial.begin(115200);
  // Code to sleep mode.
  // ********** Also add the code to put the sensor in sleep mode
  Serial.print("Going to deep sleep mode for seconds:");
  Serial.println(sleepTimeSecs);
  int sleepTimeSecsCal = sleepTimeSecs * 1000000;
  shutLED();
  ESP.deepSleep(sleepTimeSecsCal);
  Serial.println("Wake up!");
}

// Define LED Pin modes
void setupLEDPinModes() {
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  //pinMode(PIN_BLUE,  OUTPUT);
}


//Set the color of LED
/*void setColor(int RED, int GREEN, int BLUE) {
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
  /*
}*/



// Handle Portal
void handlePortal() {

  String htmlPage;
  if (server.method() == HTTP_POST) {
    strlcpy(user_info.init,  eeromCheck, sizeof(user_info.init) );
    strlcpy(user_info.userWifiId, server.arg("userWifiId").c_str(), sizeof(user_info.userWifiId) );
    strlcpy(user_info.password, server.arg("passwd").c_str(), sizeof(user_info.password) );
    strlcpy(user_info.containerId, server.arg("containerId").c_str(), sizeof(user_info.containerId) );
    user_info.kontSetupflag = true; // Boolean works ?

      //Serial.print("user_info.init[2]):");
      //Serial.println(user_info.init[2]);


      //Serial.print("server.arg(userWifiId).length():");
      //Serial.println(server.arg("userWifiId").length());

      //Serial.print("server.arg(passwd).length():");
      //Serial.println(server.arg("passwd").length());

      //Serial.println("server.arg(passwd).c_str()");
      //Serial.println(server.arg("passwd").c_str());

      //Serial.print("server.arg(containerId).length():");
      //Serial.println(server.arg("containerId").length());


      //Serial.print("user_info.init[2]):");
      //Serial.println(user_info.init[2]);

    //    user_info.init[2] = user_info.userWifiId[server.arg("userWifiId").length()] = user_info.password[server.arg("password").length()] = user_info.emailId[server.arg("email").length()] = '\0';
    user_info.init[2] = '\0';


    //Serial.print("server.arg(userWifiId).length()");
    //Serial.println(server.arg("userWifiId").length());

    //user_info.init[2] = '\0';
    user_info.userWifiId[server.arg("userWifiId").length()] = user_info.password[server.arg("passwd").length()] = user_info.containerId[server.arg("containerId").length()] = '\0';

    //Serial.print("user_info.init HandlePortal:");
    //Serial.print(user_info.init);
    //Serial.println(":");
    //Serial.print("user_info.userWifiId HandlePortal:");
    //Serial.print(user_info.userWifiId);
    //Serial.println(":");
    //Serial.print("user_info.password HandlePortal:");
    //Serial.print(user_info.password);
    //Serial.println(":");
    //Serial.print("server.arg(passwd).length():");
    //Serial.print(server.arg("passwd").length());
    //Serial.println(":");
    //Serial.print("user_info.containerId HandlePortal:");
    //Serial.print(user_info.containerId);
    //Serial.println(":");

    EEPROM.put(0, user_info);
    EEPROM.commit();
    wifiSavedFlag = true;
    //Serial.println("user_wifi put:");
    //Serial.println(user_info.emailId);
    //EEPROM.put(60, user_info); // Change to dynamic
    //EEPROM.commit();
    htmlPage = "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>smartKont Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please reset the device by pressing reset button at the bottom of container.</p></main></body></html>";
    server.send(200, "text/html", htmlPage);
    // server.send(200,   "text/html",  "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Wifi Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please restart the device.</p></main></body></html>" );
    // Restart the device here
    //restartESP();
  } else {
    String networksHTML = scanNetworks();
    htmlPage = "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>...</style></head>"
                      "<body><main class='form-signin'><form action='/' method='post'>"
                      "<h1 class=''>Wifi Setup</h1><br/>"
                      "<div class='form-floating'><label>SSID</label>" + networksHTML + "</div>"
                      "<div class='form-floating'><br/><label>Password</label><input type='text' class='form-control' name='passwd'/></div>"
                      "<div class='form-floating'><br/><label>Container-Id</label><input type='text' class='form-control' name='containerId'> </div><br/><br/>"
                      "<button type='submit'>Save</button><p style='text-align: right'><a href='https://www.smartkont.ca' style='color: #32C5FF'>smartkont.ca</a></p><br/><br/>"
                      "</form></main></body></html>";
    server.send(200, "text/html", htmlPage);
    // Wrong password say it is wrong password entered*******
    // server.send(200,   "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style></head><body><main class='form-signin'><form action='/' method='post'><h1 class=''>Wifi Setup</h1><br/><div class='form-floating'><label>SSID</label><input type='text' class='form-control' name='userWifiId'> </div><div class='form-floating'><br/><label>Password</label><input type='text' class='form-control' name='passwd'/></div><div class='form-floating'><br/><label>Container-Id</label><input type='text' class='form-control' name='containerId'> </div><br/><br/><button type='submit'>Save</button><p style='text-align: right'><a href='https://www.smartkont.ca' style='color: #32C5FF'>smartkont.ca</a></p><br/><br/></form></main></body></html>" );
  }
}




void setup() {
  // Switch off blue light on ESP
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  delay(100);// 2 Second delay can be removed...
  //Serial.println("Inside setup");
  
  // Set Input output pins
//  setupSensorPinModes();
  // LED pin setup
  setupLEDPinModes();
  //WiFi.persistent(true);
  //WiFi.setAutoConnect(true);
  //WiFi.setAutoReconnect(true);

  // Make a local copy of EEROM
  eeromVarCopy();

do {
  if (WiFi.status() != WL_CONNECTED) { //Check WiFi connection status 
    //Serial.println("Inside WiFi.status() != WL_CONNECTED");
    if(!wifiConnect()){// Connecting to wifi, if not connected
        //Serial.println("Inside Wifi no Connected");
        wifiNotConnCntr++; // increase how much time wifi is not connected since chip started
      } 
  }

  if(WiFi.status() == WL_CONNECTED) {
  //if(wifiConnected) {
    //Serial.println("Inside Wifi Connected");
      bool requestStat = sendHttpRequestData();
      
      if (requestStat) { 
      // If request is sent then go to deepSleep mode. There is a hack in sendHttpRequestData which will return true even if request isnot sent successfully so that chip go to sleep mode.
        deepSleep(sleepTimer); //Send a request every x seconds
        Serial.println("Wake up from sleep mode!!!");
      } else {
        httpRequestCntr++; // Http request not sent counter, try twice http request
      }

  }

  if((wifiNotConnCntr>=2 && wifiSavedFlag) || (httpRequestCntr >=2 && wifiSavedFlag)) { // Restart ESP if wifiConnect is not working for 2 time in a sequence.
        Serial.println("Restarting ESP");
        deepSleep(sleepTimer); // let it go to sleep for 30 mins, if it doesn't more retry alert can be made visible on app that recording is not happening, please check wifi strength
        //restartESP();
  } // Saving something to permanent memory so it doesn't go in infinite loop after number of retiries to save batteries.
  //Serial.print("wifiNotConnCntr:");
  //Serial.println(wifiNotConnCntr);

  //Serial.print("wifiSavedFlag:");
  //Serial.println(wifiSavedFlag);

  //Serial.print("wifiWrongpasswdOrSSID:");
  //Serial.println(wifiWrongpasswdOrSSID);

} while(wifiNotConnCntr < 3 && wifiSavedFlag && httpRequestCntr < 3); // Retry 2 times ESP will restart after 2 tries? should it go to sleep


 configPortalTimeControl=millis(); 
}




void loop() {
Serial.println("Inside Loop");
//Serial.print("millis()-configPortalTimeControl:");
//Serial.println(millis()-configPortalTimeControl);
server.handleClient();
if((millis()-configPortalTimeControl>600000)) { // 20 mins, this code check needs to revisit based on reset button container.
  // If config portal is on for more than 2.5 minutes put the ESP to sleep 15 days or until reset button is clicked.
  deepSleep(configPortalsleepTimerHours*3600);
}


}