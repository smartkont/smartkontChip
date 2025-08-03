#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>


#include <user_interface.h> 
#include <Arduino.h>
/*
Legal statement this code is the property of Financial Minds Consulting Ltd. Canada making any copy of the code or using any art of the code without written approval from the owner of Financial Minds Consulting Ltd is not permitted.

*/

/* To do 
1. Put the ultrasonic sensor to sleep mode. DONE
4. ESP light to put off. GPIO2 pin, this is done but seems like blue light needs to be put off physically
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
7. WIFI ssid changed or put off for some time - Partially done try for 6 hours 12 time and if not connected go to sleep forever
   and when reset button is clicked wifi id passwd can be configured again, if reset button is pressed twice it will restart again trying same id and password.
8. Battery check indicator - Need seperate module
9. Write code require for app 
10. Change sever path once new server are bought
11. Waiting circle when wifi id password is saved

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

//ESP8266WebServer    server(80);
AsyncWebServer server(80);

//Need to handle this change the mode and wait for few seconds
Ticker modeSwitchDelay;

// Structure to store to EEROM
struct settings {
  //char init[2] =  ["NI"];
  char init[3];
  char userWifiId[30];
  char password[30];
  char containerId[50];
  bool kontSetupflag;
  int wifiConCntr;
  //int sleepCycles; // Number of sleepCycles

  IPAddress IP;  // Use the Local IP from Serial Output
  IPAddress gateway;     // Use the Gateway IP from Serial Output
  IPAddress subnet; //(255, 255, 252, 0);    // Use the Subnet Mask from Serial Output
  IPAddress primaryDNS; //(8, 8, 8, 8);      // Google DNS Use Primary DNS from Serial Output
  IPAddress secondaryDNS; //(8, 8, 4, 4);    // Use Secondary DNS from Serial Output
  short wifiChannel; // Wifi channel
  //char bssid[37]; // BSSID // Check the actual char needed shold be 6
  uint8_t bssid[6];
} user_info = {};


// RTC memory to differentiate between reset buttom press and deepsleep wakeup
struct ESPResetRtcData {
  //uint32_t resetMarker;  // known marker
  uint32_t resetCount;
  //uint32_t lastResetMillis;
} resetData = {};


  // Local vairables 
  char initLocal[3]; // Not initialized
  char userWifiIdLocal[30];
  char passwordLocal[30];
  //char passwordLcl[30] = "NI";
  char containerIdLocal[30];
  bool kontSetupflagLocal;
  int wifiConCntrLocal;

  //Local Variables for wifi connection
  IPAddress ipLocal;
  IPAddress gatewayLocal;
  IPAddress subnetLocal;
  IPAddress primaryDNSLocal;
  IPAddress secondaryDNSLocal;
  short wifiChannelLocal;
  String bssidStrLocal;
  bool saveInfotoERROMFlag = false; // When true save above wifi connection local variables to EEROM
  //uint8_t* bssidLocal[]; // CHeck this
  uint8_t bssidLocal[6];


  
// Process controlling variables
int wifiNotConnCntr = 0; // Number of time wifi is not connected for consecutive times.
int httpRequestCntr = 0; // Number of time wifi is not connected for consecutive times.
//bool httpRequestFlag = false; // https Send Request Flag, control the number of times http request os sent.
bool wifiSavedFlag = false; // Flag to initialize wifi is saved in EEROM
bool configPortalONFlag = false; // Flag to initialize wifi is saved in EEROM
bool wifiWrongpasswdOrSSID = false; // Flag to initialize wifi is saved in EEROM
bool wifiConnected = false; // Flag to initialize wifi is saved in EEROM
bool loopLogic = true;

// ESP Sleep timers
const int sleepTimer=120; // ESP sleep timer
const int sixhoursSleepTimer=6; // Long sleep 2 days if not able to connect for 3 hrs to conserve battery
const int twoDaySleepTimer=48; // Long sleep 2 days if not able to connect for 3 hrs to conserve battery
const int sevenDaySleepTimer=168; // Long sleep 7 days if not able to connect for 24 hrs to conserve battery
const int thirtyDaySleepTimer=720; // Long sleep 30 days if not able to connect for 24 hrs to conserve battery
const int tenYearDaySleepTimer=87600; // Long sleep 10 Years permanent sleep if not getting connected
const int configPortalsleepTimerHours=365; // ESP sleep for 15 days  (effectively until reset is clicked manually)

const char *ssid = "SmartKont"; // ESP advertise for Config Portal.
//const char *password = "987654321"; // Not used

// Control the timing of config portal sleep the ESP for 15 days or until reset buttons i clicked
uint32_t configPortalTimeControl; 

// Wifi scan network
String networksHTML = ""; // Wifi Netwrok avail


// Sensor pin definition for distance calculation
#define trigPin D5
#define echoPin D6
#define snsrPwrPin D7

// LED pin definition for illumination
//const int PIN_RED   = D2; 
//const int PIN_GREEN = D1; 
//const int PIN_BLUE  = D7; 
// CHECK which one is connected to which PIN
const int PIN_GREEN = D1;
const int PIN_RED = D2;



// Sonic sensor distance and duration measurement
long  duration;
int distance;


// May be useful in container Setup
//AutoConnectConfig acConfig;
//acConfig.apid = "ESP-" + String(ESP.getChipId(), HEX);



String ssidList = "[]"; // Global
bool wifiConnecting = false;
bool paramMissing = false;
unsigned long wifiStartTime = 0;
String ssidToConnect = "";
String passToConnect = "";
String containerIdToConnect = "";

void kontSetupRequest(int wifiConCntrKontSetup) {
  server.on("/", HTTP_GET, [wifiConCntrKontSetup](AsyncWebServerRequest *request) {
    //request->send(200, "text/html",
    
    String html =  R"rawliteral(

<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        margin: 0;
        font-family: sans-serif;
        background-color: #f9f9f9;
      }

      .top-bar {
        width: 100%;
        height: 50px;
        background-color: #ae8e65;
        color: white;
        text-align: center;
        line-height: 50px;
        font-size: 20px;
        font-weight: bold;
        box-shadow: 0 2px 5px rgba(0,0,0,0.2);
      }

      .content {
        max-width: 400px;
        margin: 0 auto;
        padding: 20px;
        text-align: center;
        font-size: 18px;
      }

      input, select {
        width: 100%;
        padding: 10px;
        margin-top: 5px;
        margin-bottom: 15px;
        font-size: 16px;
      }

      button {
        padding: 12px 24px;
        font-size: 16px;
        background-color: #ae8e65;
        color: white;
        border: none;
        border-radius: 5px;
        cursor: pointer;
      }

      button:hover {
        background-color: #ae8e65;
      }
    </style>

<style>
  .overlay {
    position: fixed;
    top: 0; left: 0;
    width: 100vw; height: 100vh;
    background-color: rgba(0, 0, 0, 0.5); /* fade background */
    z-index: 1000;
  }

  .modal {
    position: fixed;
    top: 50%; left: 50%;
    transform: translate(-50%, -50%);
    background: white;
    border-radius: 10px;
    padding: 20px 40px;
    z-index: 1001;
    text-align: center;
    box-shadow: 0 4px 10px rgba(0, 0, 0, 0.3);
  }

  .spinner {
    margin: 15px auto 0;
    border: 4px solid #f3f3f3;
    border-top: 4px solid #3498db;
    border-radius: 50%;
    width: 30px;
    height: 30px;
    animation: spin 1s linear infinite;
  }

  @keyframes spin {
    0% { transform: rotate(0deg);}
    100% { transform: rotate(360deg);}
  }
</style>


  <style>
    .password-container {
      position: relative;
      width: 100%;
      max-width: 400px;
    }

    #password {
      width: 100%;
      padding: 10px 40px 10px 10px;
      font-size: 16px;
      box-sizing: border-box;
    }

    .toggle-password {
      position: absolute;
      right: 10px;
      top: 50%;
      transform: translateY(-50%);
      cursor: pointer;
      font-size: 24px;
      padding: 8px;
      user-select: none;
    }

  </style>


  </head>

  <body>
    <div class="top-bar">SmartKont WiFi Setup</div>
    <div class="content">
      <form id="wifiForm">
        SSID:
        <select id="ssid"></select><br>

        Password:
      <div class="password-container">
        <input type="password" id="password" placeholder="Enter WiFi Password">
        <span id="togglePassword" class="toggle-password" role="button" tabindex="0" aria-label="Toggle password visibility">👁️</span>
      </div>
        <br>

        <div id="containerIdDiv">
        Container ID:
        <input type="text" id="containerId"><br>
        </div>

        <button type="submit" id="saveBtn">Save</button>

        <!-- Overlay and Modal -->
        <div id="overlay" class="overlay" style="display:none;"></div>
        <div id="spinnerModal" class="modal" style="display:none;">
          <div class="modal-content">
            <p>Setting up smartKont...</p>
            <div class="spinner"></div>
          </div>
        </div>

      </form>
    </div>

      <div id="status" style="display: none;"></div>
    <div id="wifiModal" style="
      display: none;
      position: fixed;
      z-index: 999;
      left: 0; top: 0;
      width: 100%; height: 100%;
      background-color: rgba(0, 0, 0, 0.5);
    ">
      <div style="
        background-color: #fff;
        margin: 15% auto;
        padding: 20px;
        width: 80%;
        max-width: 400px;
        border-radius: 10px;
        text-align: center;
        box-shadow: 0 0 10px #333;
      ">
        <h3 id="modalTitle">Status</h3>
        <p id="modalMessage">Message</p>
        <button onclick="closeModal()">OK</button>
      </div>
    </div>
   
  <script>

  var wifiConCntrKontSetup = )rawliteral";

  html += String(wifiConCntrKontSetup);
  // -50 just to be on safer side some time -1 is allocated to the wifiConCntrKontSetup which caused container id not to display
  html += R"rawliteral(;

  if (wifiConCntrKontSetup < -50) {
    document.getElementById("containerIdDiv").style.display = "none";
  }

  let savedState = null; // Global to keep the status for submitForm
  // Alert Message
  function showModal(title, message) {
    document.getElementById('modalTitle').innerText = title;
    document.getElementById('modalMessage').innerText = message;
    document.getElementById('wifiModal').style.display = 'block';

  }

  function closeModal() {
    if(savedState === 'connected') {
      submitForm(savedState);
    }
    document.getElementById('wifiModal').style.display = 'none';
  }


  function reenableButton(buttonId, newLabel = null) {
  const btn = document.getElementById(buttonId);
  if (!btn) return;

  btn.disabled = false;
  btn.style.backgroundColor = '';
  btn.style.cursor = 'pointer';

  if (newLabel !== null) {
    btn.innerText = newLabel;
  }
}

// Check and change ******
  function submitForm(state) {
     var jsonState = JSON.stringify({
          "status": state
     });
      AndroidInterface.sendJson(jsonState);
  }

  // Show Spinner and Hide Spinner
  function showSpinner() {
    document.getElementById('overlay').style.display = 'block';
    document.getElementById('spinnerModal').style.display = 'block';
  }

  function hideSpinner() {
    document.getElementById('overlay').style.display = 'none';
    document.getElementById('spinnerModal').style.display = 'none';
  }


  const togglePassword = document.getElementById('togglePassword');
  const passwordInput = document.getElementById('password');

    togglePassword.addEventListener('click', () => {
      const isPassword = passwordInput.getAttribute('type') === 'password';
      passwordInput.setAttribute('type', isPassword ? 'text' : 'password');
      togglePassword.textContent = isPassword ? '🚫👁️' : '👁️';
    });


    // Load SSIDs into dropdown
    fetch('/networks')
      .then(res => res.json())
      .then(data => {
//        if (data.status === "scanning") {
//            setTimeout(fetchNetworks, 1000);  // retry in 1s
//          } else {
            const sel = document.getElementById('ssid');
            data.forEach(ssid => {
              const opt = document.createElement('option');
              opt.value = ssid;
              opt.textContent = ssid;
              sel.appendChild(opt);
//          }
        });
      });

    // Form submission
    document.getElementById('wifiForm').addEventListener('submit', function(e) {
      e.preventDefault();

      const saveBtn = document.getElementById('saveBtn');

      // Disable and grey out the button
      saveBtn.disabled = true;
      saveBtn.style.backgroundColor = '#888';
      saveBtn.style.cursor = 'not-allowed';

      // Data collect from form
      const ssid = document.getElementById('ssid').value;
      const password = document.getElementById('password').value;
      const containerId = document.getElementById('containerId').value;
      const params = 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password) + '&containerId=' + encodeURIComponent(containerId);

      showSpinner();

      fetch('/startwifi', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: params
      }).then(() => {
        const interval = setInterval(() => {
          fetch('/checkwifi')
            .then(r => r.json())
            .then(res => {
              document.getElementById('status').innerText = res.status;
              savedState = res.status;
              if(res.status === 'connected'){
                clearInterval(interval);
                hideSpinner();
                showModal("WiFi Saved!", "Press Ok for next Step");
              } else if (res.status === 'paramMissing') {
                clearInterval(interval);
                hideSpinner();
                showModal("Missing Info", "Please Enter all fields and Submit");

                reenableButton('saveBtn');

              } else if (res.status === 'failed') {
                clearInterval(interval);
                hideSpinner();
                showModal("Wrong Id or Password", "Please check correct Wifi Id and Password is submitted");
                reenableButton('saveBtn','Try Again');
              } else if (res.status === 'credSaveFail') {
                clearInterval(interval);
                hideSpinner();
                showModal("Info not Saved!", "Please re-enter all information and try again!");
                reenableButton('saveBtn','Try Again');
              //} else if (res.status === 'idle') { // ?? remove
                //clearInterval(interval);
                //showModal("Enter Details", "Enter Details"); 
              } else if (res.status !== 'connecting') {
                clearInterval(interval);
                hideSpinner();
                showModal("Unknown Error!", "Retry: Press reset button at bottom of container and restart application");
              }
              // if (res.status === 'connected' || res.status === 'failed' || res.status === 'wrongPasswd' || res.status === 'paramMissing' ) clearInterval(interval);
            })
            .catch(error => {
                hideSpinner();
                alert("Error checking Wi-Fi connection.");
                console.error(error);
              });
        }, 500);
      });
    });
  </script>
</body></html>
    )rawliteral";

  request->send(200, "text/html", html);
  });

  

  // POST handler to start connecting to WiFi
  server.on("/startwifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true) && request->hasParam("containerId", true)) {
      if(request->getParam("password", true)->value() == 0 || request->getParam("containerId", true)->value() == 0 ) {
        paramMissing = true;
      } else {
        ssidToConnect = request->getParam("ssid", true)->value();
        passToConnect = request->getParam("password", true)->value();
        containerIdToConnect = request->getParam("containerId", true)->value();
        WiFi.begin(ssidToConnect.c_str(), passToConnect.c_str()); // Check if wifi is correct or not
        wifiStartTime = millis();
        wifiConnecting = true;
        request->send(200, "text/plain", "Connecting...");
      }
      
    } else {
      request->send(400, "text/plain", "Missing ssid or password or container Id");
    }
  });


  // Polling endpoint to check status
  server.on("/checkwifi", HTTP_GET, [wifiConCntrKontSetup](AsyncWebServerRequest *request) {
      if(paramMissing) {
        paramMissing = false; // Reset for rentry of id and password
        request->send(200, "application/json", "{\"status\":\"paramMissing\"}");
        return;
      }
      


      int wifiStatusConnectCheck = WiFi.status();

      if (wifiStatusConnectCheck == WL_CONNECTED) {
        if(saveCredentialNInfo(ssidToConnect, passToConnect, containerIdToConnect, wifiConCntrKontSetup)) {
            // Blink LED here
            request->send(200, "application/json", "{\"status\":\"connected\"}"); // Connected and Saved Credentials

            // Switches back to station mode this will help Mobile to connect back to orginal wifi
            modeSwitchDelay.once_ms(100, []() {
              WiFi.softAPdisconnect(true); // Turn off AP
              WiFi.mode(WIFI_STA);       // Schedules callback after 100 ms
            });
            
        } else { // Save Credential is unsuccessful
            //Serial.print("saveCredentialNInfo unsuccessful");
            request->send(200, "application/json", "{\"status\":\"credSaveFail\"}"); // Connected and Saved Credentials
        }
        //WiFi.softAPdisconnect(true); // See how to disable
      } /*else if(wifiStatusConnectCheck == WL_WRONG_PASSWORD){ // Doesn't work in ESP8266

        Serial.println("❌ Wrong Password");
        WiFi.disconnect();
        request->send(200, "application/json", "{\"status\":\"wrongPasswd\"}");

      }*/ else if (millis() - wifiStartTime > 10000) { // 10 Second wait

          Serial.println("❌ Failed to connect to WiFi");
          //WiFi.disconnect(); //do we need?
          request->send(200, "application/json", "{\"status\":\"failed\"}");
      } else {
        request->send(200, "application/json", "{\"status\":\"connecting\"}");
      }
  });


  server.on("/networks", HTTP_GET, [](AsyncWebServerRequest *request) { // Rerun the scan network  use this networksHTML = scanNetworks();
//  WiFi.softAPdisconnect(true);   
//  WiFi.mode(WIFI_STA);
//  Serial.println("Scan Networks Start");


/*  int n = WiFi.scanComplete();

  if(n == -2) {
      // No scan started yet → start async scan
      WiFi.scanNetworks(true);  // true = async
      request->send(200, "application/json", "{\"status\":\"scanning\"}");
  } else if (n == -1) {
      // Scan is running
      request->send(200, "application/json", "{\"status\":\"scanning\"}");
    }  else {
      // Scan complete → build JSON
      String ssidList = "[";
      for (int i = 0; i < n; ++i) {
        if (i > 0) ssidList += ",";
        ssidList += "\"" + WiFi.SSID(i) + "\"";

//        if (i) ssidList += ",";
//        ssidList += "{";
//        ssidList += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
//        ssidList += "\"rssi\":" + String(WiFi.RSSI(i));
//        ssidList += "}";
      }
      ssidList += "]";

      WiFi.scanDelete();  // Clean up memory

  */
      extern String ssidList;

      //  int n = WiFi.scanNetworks();
      //  Serial.println("Scan Networks done");
      //  String json = "[";
      //  for (int i = 0; i < n; ++i) {
          //if (i > 0) json += ",";
          //json += "\"" + WiFi.SSID(i) + "\"";Inside credentialRequest
        //}
        //json += "]";
        //Serial.println("Scan Networks end!");
        //WiFi.mode(WIFI_AP_STA);
        //WiFiMode_t mode1 = WiFi.getMode();
      Serial.print("ssidList:");
      Serial.println(ssidList);
      request->send(200, "application/json", ssidList);
 //  }
  });

  server.begin();
}


bool restartConfigPortalCustom() {

  
    //Serial.println("Restart Config Portal");
    server.end();                     // Stop server
    WiFi.softAPdisconnect(true);      // Stop AP
    WiFi.mode(WIFI_OFF);              // WIFI OFF
    unsigned long timeout = millis();
    while (WiFi.getMode() != WIFI_OFF && millis() - timeout < 1000) {
      //Serial.println("Restart Config Portal wait");
     delay(10);
    }

    // Config portal needs to be setup for limited time (2.5 mins?) else it will eat up the battery in case user leave it open
    //delay(2000);
    WiFi.disconnect(true);  // Clears saved connection
    delay(200); // To make sure disconnects completes
    WiFi.mode(WIFI_AP_STA);
    bool res = WiFi.softAP("smartKont" + codeChipVersion); // Random number ??
    //Serial.println("Started AP: smartKont");
  return res;
}

void eeromVarCopy() {
  // Not using CRC32 check to avoid battery over usage
  EEPROM.begin(sizeof(struct settings) );
  EEPROM.get(0, user_info );
 
  strlcpy(initLocal, user_info.init, sizeof(user_info.init));
  strlcpy(userWifiIdLocal, user_info.userWifiId, sizeof(user_info.userWifiId));
  strlcpy(passwordLocal, user_info.password, sizeof(user_info.password));
  strlcpy(containerIdLocal, user_info.containerId, sizeof(user_info.containerId));

  kontSetupflagLocal = user_info.kontSetupflag;
  wifiConCntrLocal = user_info.wifiConCntr;

  //sleepCyclesLocal = user_info.sleepCycles;

  // Check later why -1 is allocated sometime
  //Serial.print("wifiConCntrLocal:");

  ipLocal = user_info.IP;
  gatewayLocal = user_info.gateway;
  subnetLocal = user_info.subnet;
  primaryDNSLocal = user_info.primaryDNS;
  secondaryDNSLocal = user_info.secondaryDNS;
  wifiChannelLocal = user_info.wifiChannel;
  // Copying bssid to local variable  
  for (int i = 0; i < 6; i++) {
      bssidLocal[i] = user_info.bssid[i];
    }

}

bool getWiFiIsSavedCustom(){
  bool wifiPasswdSaved = false;
  if ( strncmp(initLocal, "SK",2) == 0 && !(strlen(passwordLocal) == 0) && !(strlen(userWifiIdLocal) == 0) && !(strlen(containerIdLocal) == 0)) { // That mean Wifi id passwd is saved at least once
    wifiPasswdSaved=true;
  } else {
    wifiPasswdSaved=false;
  }
  return wifiPasswdSaved;
}


/*need find alternative to this function and edit the list dynamically*/
void oneScanNetworks() {
  // Accessing global ssidList
  //WiFi.mode(WIFI_STA);
  int numberOfNetworksCount = WiFi.scanNetworks();
  ssidList = "[";

  for (int i = 0; i < numberOfNetworksCount; ++i) {
    if (i > 0) ssidList += ",";
    ssidList += "\"" + WiFi.SSID(i) + "\"";
  }
  ssidList += "]";

  //Serial.println("Scan network done!!");
  //Serial.println(ssidList);


}


// Connect to Wifi network
// Wifi connect trying 3 time 2 with regular connect and one with scan method connect
bool wifiConnect() {
  //Serial.println("wifiConnect before");

  if(WiFi.status() != WL_CONNECTED) {

      int wifiConnectTryCnt = 0; // Config portal is correct but not able to connect to wifi weak signal or some technical issue, try 2 times before give up.
      
      // Just to be safer initialized to -50 set to -100 when number of tries exceed threshold
      if(!getWiFiIsSavedCustom() || wifiWrongpasswdOrSSID || wifiConCntrLocal < -50 ) { //|| wifiConCntrLocal > 12){  
        // If id/passwd not saved OR wifi id/passwd wrong OR wifiConCntrLocal is less than 0 meaning retry happened for sometime and give up or REtry happend 10 times
      
        // First time no access point is setup EEROM check or wrong password was saved in earlier try or Wifi password changed
        
          if(restartConfigPortalCustom()){
              wifiSavedFlag = false; // Config portal started meaning either wifi id/passwd not saved or it is wrong.
              //Serial.println("Restart Config Portal");
              configPortalONFlag = true;
              oneScanNetworks();
              kontSetupRequest(wifiConCntrLocal); // Setup request with error handling
              blinkLED("both", 5); // Blink the green light 3 time to start the setup ** Check

/*
              if(wifiConCntrLocal >= 0) { // wifiid, passwd and container id setup Initial setup
               
              } else { // wifi id password changed call the url with only id and password fields and NO container id fields
                //kontSetupRequest(); // Another version of kontSetupRequest - > wifiSetupRequest()
                blinkLED("both", 3); // Blink the green light 3 time to start the setup ** Check
              }
  */            
          }
      } else { // Access point is already saved.
        wifiSavedFlag = true;
      }

      uint32_t setupProcessTime=millis(); 

      if(wifiSavedFlag) {
        //Serial.println("regular connect wifiSavedFlag on");
          do{
            uint32_t wifiConnectTime=millis(); // Wait for connection after config portal is setup

              // Battery performance
              //WiFi.persistent(false);

              //WiFi.mode(WIFI_STA); // Change to Station mode
              // Battery performance
              if (!kontSetupflagLocal && wifiConnectTryCnt < 2){

                // Other options tried for faster wifi connection
                //WiFi.begin(userWifiIdLocal, passwordLocal, 2, {0x176, 0x25, 0x33, 0x129, 0x119, 0x34});
                //uint8_t bssid1[] = {0xB0, 0x19, 0x21, 0x81, 0x77, 0x22};
                //WiFi.begin(userWifiIdLocal, passwordLocal, 2, bssid1);
                //WiFi.begin(userWifiIdLocal, passwordLocal, 2);
                WiFi.persistent(false);
                WiFi.mode(WIFI_STA); // Use at the begning of Setup
                WiFi.setAutoConnect(true);
                WiFi.setAutoReconnect(true);


                //IPAddress local_IP(192, 168, 68, 91);  // Use the Local IP from Serial Output
                //IPAddress gateway(192, 168, 68, 1);     // Use the Gateway IP from Serial Output
                //IPAddress subnet(255, 255, 252, 0);    // Use the Subnet Mask from Serial Output

                //IPAddress primaryDNS(8, 8, 8, 8);      // Google DNS Use Primary DNS from Serial Output
                //IPAddress secondaryDNS(8, 8, 4, 4);    // Use Secondary DNS from Serial Output

                //IPAddress primaryDNS(209, 197, 128, 2);      // Use Primary DNS from Serial Output
                //IPAddress secondaryDNS(209, 197, 128, 5);    // Use Secondary DNS from Serial Output
                

                WiFi.config(ipLocal, gatewayLocal, subnetLocal, primaryDNSLocal, secondaryDNSLocal);

                //WIFI_NONE_SLEEP, WIFI_LIGHT_SLEEP and WIFI_MODEM_SLEEP
                //WiFi.setSleepMode(WIFI_NONE_SLEEP); 

                WiFi.begin(userWifiIdLocal, passwordLocal, wifiChannelLocal, bssidLocal);
                saveInfotoERROMFlag = false; // No need to save to EEROM, needed ?

              } else {
                WiFi.begin(userWifiIdLocal, passwordLocal);
                saveInfotoERROMFlag = true; // Meaning save Wifi IP address data to EEROM
              }
             
                /* 3.1.2 not very stable --- Using 3.1.2 for json responses hope stability works
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
              int wifiStatus = WiFi.status();
              while (wifiStatus != WL_CONNECTED && !(wifiWrongpasswdOrSSID) && (millis()-wifiConnectTime<15000))  { // Wait for 15 second each time before giving up, change to 10 Seconds
                
                if(wifiStatus == WL_NO_SSID_AVAIL) { 
                  //Serial.println("SSID not available, possible wifi connection lost!");
                  // Break while loop and go to sleep mode
                  break;
                }
                
                if (wifiStatus == WL_CONNECT_FAILED) { // Wrong passwd main cause 
                  //Serial.println("Wifi connection failed, start the config portal");
                  wifiWrongpasswdOrSSID = true;
                  break;
                }
                // Yield is better option as per books but delay is better in terms of realtime testing by .5 to .9 seconds in some cases with yield total time is around 3.7 to 3.9 second however with delay it is around 3.2 seconds
                //Serial.print(".");
                //yield();
                delay(100); // .25 second delay in checking the status
                wifiStatus = WiFi.status();
                //Serial.print(wifiStatus);
              }
              // Set the minimum signal quality default 8%
              //wifiManager.setMinimumSignalQuality();
              if(wifiStatus == WL_CONNECTED){ // Wifi got connected.
                    wifiConnected = true;
                    byte mac[6];
                    WiFi.macAddress(mac);

                    macAddress = String(mac[5], HEX) +(":") + 
                                String(mac[4], HEX) +(":") + 
                                String(mac[3], HEX) +(":") + 
                                String(mac[2], HEX) +(":") + 
                                String(mac[1], HEX) +(":") + 
                                String(mac[0], HEX) +(":") ;
                  } else { // Increase the counter to try the wifi again
                              Serial.println("Try Again!");
                              wifiConnectTryCnt++;
                              //delay(5000); // Delay 5 second to reconnect if wifi is not connected in first try
                        }
          } while((!wifiConnected) && (wifiConnectTryCnt<3) &&  !(wifiWrongpasswdOrSSID) && ((millis()-setupProcessTime)<50000)); // 31 seconds, will try 2 times quick method and then 1 scan method
      }
      } else {
        // Wifi is connected
        wifiConnected = true;
      }
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
  setupSensorPinModes();
  sensrPwrPinReady();
  delay(750); // 10 Seconds delay to start .. Needed ? to have the seonsor ready for the function
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  // Put the sensor to sleep as soon distance is measured
  sensrPwrPinSleep();

  
  distance = duration/2*.0343; // As of now cosidering speed of sound in dry air (humidity factor not considered)

  if(distance>=400 || distance <=.2) { // Distance less than 2 mm and greater than 400 CM
    distance=-1000; // -1000 distance when out of scope
  }

return distance;
}

// Create http request for regular readings
String createHttpRequestData (String apikeyValue_f, float distanceValue_f) {
  //******** String class uses lot of memory try to replace option SafeString
  String postApiKey="api_key=";
  String postDistance="&distance=";
  //String postMacAddress = "&mac=";
  String postDeviceId = "&deviceId=";
  //String httpRequestData_f = postApiKey + apikeyValue_f + postDistance + distanceValue_f + postDeviceId + containerIdLocal + postMacAddress + macAddress_f  ;
  String httpRequestData_f = postApiKey + apikeyValue_f + postDistance + distanceValue_f + postDeviceId + containerIdLocal;
  return httpRequestData_f;
}


// Create http request container Setup
String createHttpRequestSetup (String apikeyValue_f, String userContainerId, float measuredDistance_f) { // This is if findout what size of container is
//String createHttpRequestSetup (String apikeyValue_f, String userContainerId, String  macAddress_f) {
  //******** String class uses lot of memory try to replace option SafeString
  String postApiKey="api_key=";
  String postContainerId="&containerId=";
  //String postMacAddress = "&mac=";
  String postDistance="&distance=";
  //userContainerId = 1000;

  String httpRequestData_f = postApiKey + apikeyValue_f + postContainerId + userContainerId + postDistance + measuredDistance_f ;
  //String httpRequestData_f = postApiKey + apikeyValue_f + postContainerId + userContainerId + postMacAddress + macAddress_f + postDistance + measuredDistance_f ;
  //String httpRequestData_f = postApiKey + apikeyValue_f + postContainerId + userContainerId + postMacAddress + macAddress_f ;
  return httpRequestData_f;
}

void disconnectWifi(){
    // Disconnect wifi
    //WiFi.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void kontSetupFlagsetup(bool kontFlag){
    // SET EEROM Boolean variable to True conntainer set up complete 
    if(kontSetupflagLocal) {
       //Serial.println("Change the flag in EEROM that container is setup");
       user_info.kontSetupflag = kontFlag; 
     }
   EEPROM.put(0, user_info);
   EEPROM.commit(); // Change the kontflag only
}


void saveInfotoEEROM(){
    
              //Serial.print("Local IP: ");
              //Serial.println(WiFi.localIP());  // Get local IP address
              user_info.IP=WiFi.localIP();
              
              //Serial.print("Gateway IP: ");
              //Serial.println(WiFi.gatewayIP());  // Get gateway IP
              user_info.gateway=WiFi.gatewayIP();
              
              //Serial.print("Subnet Mask: ");
              //Serial.println(WiFi.subnetMask());  // Get subnet mask
              user_info.subnet=WiFi.subnetMask();

              //Serial.print("Primary DNS: ");
              //Serial.println(WiFi.dnsIP(0));  // Get primary DNS
              user_info.primaryDNS=WiFi.dnsIP(0);

              //Serial.print("Secondary DNS: ");
              //Serial.println(WiFi.dnsIP(1));  // Get secondary DNS
              user_info.secondaryDNS=WiFi.dnsIP(1);

              memcpy(user_info.bssid, WiFi.BSSID(), 6);

              EEPROM.put(0, user_info);
              EEPROM.commit(); // Change the kontflag only
}

String createHttpMasterData () {
  String httpMasterData;
      if (!kontSetupflagLocal){
      // Get from sensor
      float measuredDistance = distanceMeasure();
      httpMasterData = createHttpRequestData(apiKeyValue,measuredDistance);
    } else {
      // Get from sensor
      float measuredDistance = distanceMeasure(); // Initial setup empty container distance measurement.
      httpMasterData = createHttpRequestSetup(apiKeyValue,containerIdLocal, measuredDistance);
    }
    return httpMasterData;
}


// Send http request to web server
bool sendHttpRequestData (String httpRequestData) {
    HTTPClient http;    //Declare object of class HTTPClient
    WiFiClient client;  // Object for WiFi Client

    if (!kontSetupflagLocal){
      http.begin(client, serverNamePath); //Request destination
    } else {
      // One time to setup the flag
      http.begin(client, initialSetupPath); //Request destination
    }
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //content-type header
    
    
    int httpCode;
    bool httpReturnCode = false;

    // Connect to wifi and if success send http request
    if(wifiConnect()) { 
      // Save static IP address to EEROM
      //if(kontSetupflagLocal || saveInfotoERROMFlag) {
      if(saveInfotoERROMFlag) {
          saveInfotoEEROM();// kont is setup change the flag
       }

       if(wifiConCntrLocal != 0) {
        user_info.wifiConCntr = 0;  // If connected set the counter back to 0
        EEPROM.put(0, user_info);
        EEPROM.commit(); 
       }

      String postMacAddress = "&mac=";
      httpRequestData = httpRequestData + postMacAddress + macAddress;
      
      /*Printing the request comment in final stage */
      Serial.print (":httpRequestData:");
      Serial.println (httpRequestData.c_str());

      httpCode = http.POST(httpRequestData.c_str());   //Send the request after changing the String object to char*

      if(httpCode == 200) {
        // Payload 1 Success and 0 Failure
       const String& payload = removeSpaces(http.getString());
       
       //Serial.print("HTTP Response code:");
       //Serial.print(payload);    //Print request response payload
       //Serial.println(":");

        if(!kontSetupflagLocal) {
          if(payload == "1") {
              // Disconnect Wifi to conserve power
              disconnectWifi();
              blinkLED("green", 1);
              httpReturnCode = true;    
            } else {
              blinkLED("red", 1);
            }
        } else if (kontSetupflagLocal){
            if(payload == "1") {
              // Disconnect Wifi to conserve power
              kontSetupFlagsetup(false); //Kont is setup set the flag to false
              disconnectWifi();
              httpReturnCode = true;
              blinkLED("green", 3); // 3 Green flashes meaning kont is setup
            } else {
              // Disconnect Wifi to conserve power
              //disconnectWifi();
              blinkLED("red", 3); // 3 red blink if container is not setup
            }
        }

      }
      else { // If response not received program will try two times and then print the error of last error.

              //blink(255, 0 , 0 , 3); // 3 RED Blink meaning some issue with the webserver.
              blinkLED("red", 1);
              // Blink Red LED
              Serial.print("Error code: ");
              Serial.println(httpCode);
       }

    } else {
      wifiNotConnCntr++;
      //Increase the counter of eerom
      user_info.wifiConCntr = wifiConCntrLocal+1;  // Increase the counter if wifi is not connected and save into EEROM
      EEPROM.put(0, user_info);
      EEPROM.commit(); 

    }
       
    http.end();  //Close connection
    return httpReturnCode;
}


void blinkLED(String color, int b_times){
  int b_time = 0;
  if(color == "red") {
    while (b_time < b_times) {
      // Red Blink more steady and bold
      for(int i = 0; i<5; i++){
        analogWrite(PIN_RED, 255);
        delay(10);
        analogWrite(PIN_RED, 0);
        delay(100);
      }
      
      b_time++;
    }
   }
  else if (color == "green"){
    while (b_time < b_times) {
      for(int i = 0; i<5; i++){
        analogWrite(PIN_GREEN, 255);
        delay(10);
        analogWrite(PIN_GREEN, 0);
        delay(100);
      }
      
      b_time++;
    }
   } else if (color == "both") {
    // Green Red Blink
     while (b_time < b_times) {
      for(int i = 0; i<100; i=i+10){
        analogWrite(PIN_GREEN, 255);
        delay(50);
        analogWrite(PIN_GREEN, 0);
        analogWrite(PIN_RED, 255);
        delay(50);
        analogWrite(PIN_RED, 0);
      }
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

/*
void longdeepSleep(const int sleepTimeSecs, const int totalSleepCycles) {

  //int totalSleepCycles = sleepTimeSecs/3600;
  user_info.sleepCycles = totalSleepCycles-1;  // Increase the counter if wifi is not connected and save into EEROM
  EEPROM.put(0, user_info);
  EEPROM.commit(); 
  deepSleep(3600); //Sleep for 1 hour


}
*/


// Make the board in deep sleep mode to conserve the battery
void deepSleep(const int sleepTimeSecs) {
  //Serial.begin(115200);
  // Code to sleep mode.
  // ********** Also add the code to put the sensor in sleep mode

    // Long sleep
  server.end();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200); // ensure clean shutdown

  int sleepTimeSecsCal = sleepTimeSecs * 1000000;
  shutLED();
  Serial.println("Sleep END!");
  ESP.deepSleep(sleepTimeSecsCal);
  Serial.println("Wake up!");
}

// Define LED Pin modes
void setupLEDPinModes() {
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  //pinMode(PIN_BLUE,  OUTPUT);
}


// Function to save credentials and containerID in EEROM
bool saveCredentialNInfo(String ssid, String password, String containerId, int wifiConCntrKontSetup) {
    strlcpy(user_info.init,  eeromCheck, sizeof(user_info.init) );
    strlcpy(user_info.userWifiId, ssid.c_str(),  sizeof(user_info.userWifiId));
    strlcpy(user_info.password, password.c_str(), sizeof(user_info.password) );

    if(wifiConCntrKontSetup >= -50) {
      strlcpy(user_info.containerId, containerId.c_str(), sizeof(user_info.containerId) );
    }
    user_info.kontSetupflag = true;
    user_info.wifiConCntr = 0;
    //user_info.sleepCycles = 0;

    user_info.init[2] = '\0';

    user_info.userWifiId[ssid.length()] = user_info.password[password.length()] = user_info.containerId[containerId.length()] = '\0';


    writeStructWithCRC(0, user_info);
    //EEPROM.put(0, user_info);
    //EEPROM.commit();
    wifiSavedFlag = true;
  return readStructWithCRC(0, user_info);
}



// crc32 calculation function
uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  while (length--) {
    crc ^= *data++;
    for (int i = 0; i < 8; i++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;
}


template <typename T>
void writeStructWithCRC(int eepromAddress, const T &data) {
  EEPROM.put(eepromAddress, data);

  uint32_t crc = crc32((uint8_t *)&data, sizeof(T));
  EEPROM.put(eepromAddress + sizeof(T), crc);

  EEPROM.commit(); // Save to EEROM user_info and CRC
}


template <typename T>
bool readStructWithCRC(int eepromAddress, T &data) {
  // Read struct
  EEPROM.get(eepromAddress, data);

  // Read stored CRC
  uint32_t storedCRC;
  EEPROM.get(eepromAddress + sizeof(T), storedCRC);

  // Compute CRC of data
  uint32_t computedCRC = crc32((uint8_t *)&data, sizeof(T));

  return (computedCRC == storedCRC);
}

// Erase EEROM
void eraseEEPROM() {
  EEPROM.begin(sizeof(struct settings) );
  for (int i = 0; i < sizeof(struct settings); i++) {
    EEPROM.write(i, 0xFF); // Erasing the flash bite
  }
  EEPROM.commit();
  EEPROM.end(); 
}


// Reset Button variables and fucntions
#define RTC_MEM_ADDR 65  // RTC memory address (range 64–127 safe)
#define RESET_TIMEOUT 10000UL   // 10 seconds timeout for triple reset
//#define RTC_MARKER 0xDEADBEEF

// Read reset data from RTC memory
bool readResetData(ESPResetRtcData &resetData) {
  return system_rtc_mem_read(RTC_MEM_ADDR, &resetData, sizeof(resetData));
}

// Write reset data to RTC memory
void writeResetData(const ESPResetRtcData &resetData) {
  system_rtc_mem_write(RTC_MEM_ADDR, &resetData, sizeof(resetData));
}

bool resetButtonDetectionAndClearEEROM() {
  /*
  There are other ways to detect multiple reset press like Marker and or savinf time using millis() but both ways are not working as expected, 
  RTC memory really doesn't get cleaned on reset button press and millis gives number of second since board start and doesn't work as out logic goies to sleep every 3-4 seconds.
  */
  uint8_t rstReason = system_get_rst_info()->reason;
  //Serial.printf("Reset cause code: %d\n", rstReason);
  uint32_t now = millis();
  
 // Only when physical reset is clicked  
 if (rstReason == REASON_EXT_SYS_RST) { // 6

    if (!readResetData(resetData)) {
      // Initialize
      resetData.resetCount = 0;
    } else {
      // Increment the count
      resetData.resetCount++;

      // If reset button is pressed 5 times then erase EEROM and container is already setup OR reset is pressed 10 time when kontainer is not setup.
      if ((resetData.resetCount >= 5 && !kontSetupflagLocal) || (resetData.resetCount >= 10 && kontSetupflagLocal)) {
        //Serial.println("Required reset detected! Erasing EEPROM...");
        eraseEEPROM();
        resetData.resetCount = 0;  // Reset count after erase
        writeResetData(resetData);
        blinkLED("both", 1); // erased
        return true; // Erasing eerom and hence return true
      }
      // Tried for 6 hours but failed and went to sleep forever and then check reset counter if greater than 2 then reset the wifilocal counter making it try for wifi again
      if (resetData.resetCount >= 1 && wifiConCntrLocal < -50 ) {
        // Not tested
        Serial.println("Failed to connect to wifi for six time one reset 2 times ");
        user_info.wifiConCntr = 0;  // Reset to -1 default is 0, -1 will be used to try one more time after, setting to -100 as it will increment each time reset is clicked and set to 0 when id passwd saved
        EEPROM.put(0, user_info);
        EEPROM.commit(); 

      }


      if(kontSetupflagLocal) {
        // If true then save and return
        resetData.resetCount = 0;
        writeResetData(resetData);
        return false;
      } else {// Write and wait
        writeResetData(resetData);
      }
      
      // If reset button is pressed then wait 10 second this will give enough to user to press it another time.
      while (millis() - now < RESET_TIMEOUT) {
        //delay(0);  
        yield();
      }
      resetData.resetCount = 0;
    }

    // After 10 second wait set to 0 and write it to RTC.
    writeResetData(resetData);
  }
 return false;
} 



void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  //digitalWrite(snsrPwrPin, HIGH); // Twice needed?
  
  // Set wifi to station mode
  
  //Serial.print("CPU Frequencey before");
  //Serial.println(ESP.getCpuFreqMHz());
  // Battery performance
  //ESP.setCpuFreqMHz(40);
  //Serial.print("CPU Frequencey after");
  //Serial.println(ESP.getCpuFreqMHz());

  // Switch off blue light on ESP
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  //yield();
  //delay(100);// Can this be reduced furthur?? 
  
  // Set Input output pins
  // setupSensorPinModes();
  // LED pin setup
  setupLEDPinModes();
  //WiFi.persistent(true);
  //WiFi.setAutoConnect(true);
  //WiFi.setAutoReconnect(true);

  // Make a local copy of EEROM 
  eeromVarCopy();

  // Reset button logic to erase EEROM
  if(!resetButtonDetectionAndClearEEROM()) {
      // If EEROM erased then exit everything and user need to setup the container again
      String requestMasterData = createHttpMasterData();

    do {
          bool requestStat = sendHttpRequestData(requestMasterData);
          
          if (requestStat) { 
          // If request is sent then go to deepSleep mode. There is a hack in sendHttpRequestData which will return true even if request isnot sent successfully so that chip go to sleep mode.
            //Serial.println("Sleep the ESP Normal");
            deepSleep(sleepTimer); //Send a request every x seconds
            Serial.println("Wake up from sleep mode!!!");
          } else {
            httpRequestCntr++; // Http request not sent counter, try twice http request
          }

        /* Wifi reconnect already try 3 time in wifi connect function so if still not connected then sleep; 
          if wifi is connected but still http request not send then try again and same wifi connection will be used */ 
      if((wifiNotConnCntr>=1 && wifiSavedFlag && !configPortalONFlag) || (httpRequestCntr >=2 && wifiSavedFlag && !configPortalONFlag)) { // Restart ESP if wifiConnect is not working for 2 time in a sequence.
            disconnectWifi();
            if(wifiConCntrLocal <= 12 && wifiConCntrLocal >= 0) { // If no connection for 6 hours 
              Serial.println("Sleep the ESP wifiConCntrLocal =< 12");
              deepSleep(sleepTimer); // let it go to sleep for 30 mins, if it doesn't more retry alert can be made visible on app that recording is not happening, please check wifi strength
            }else { // When reached 24 hours of trying
              Serial.println("Sleep the ESP wifiConCntrLocal forever");
              user_info.wifiConCntr = -100;  // Reset to -1 default is 0, -1 will be used to try one more time after, setting to -100 as it will increment each time reset is clicked and set to 0 when id passwd saved
              EEPROM.put(0, user_info);
              EEPROM.commit(); 
              deepSleep(0); // Sleep forever until reset button is pressed
              //restartESP();
            }

    /*
              // If counter # of time failed to send http request reset and go to do deep sleep forever
            if(wifiConCntrLocal=4) { // If no connection for 3 hours 
              Serial.println("Sleep the ESP wifiConCntrLocal =4");
              // Sleep for 6 hours
              longdeepSleep(sixhoursSleepTimer*3600, sixhoursSleepTimer); // 1 hour sleep cycle and hence number of sleep cycle is same as number of hours for sleep

            } else if (wifiConCntrLocal=8) { // Total try 10 times 6 intial in 3 hours and then additional 4 after 2 days
              Serial.println("Sleep the ESP wifiConCntrLocal =8");
              // Sleep for 2 days
              longdeepSleep(twoDaySleepTimer*3600, twoDaySleepTimer); // 1 hour sleep cycle and hence number of sleep cycle is same as number of hours for sleep
            } else if (wifiConCntrLocal=12) { // Total try 10 times 6 intial in 3 hours and then additional 4 after 2 days
              Serial.println("Sleep the ESP wifiConCntrLocal =12");
              // Sleep for 7 days
              longdeepSleep(sevenDaySleepTimer*3600, sevenDaySleepTimer); // 1 hour sleep cycle and hence number of sleep cycle is same as number of hours for sleep
            //} else if (wifiConCntrLocal=14) { // Total try 14 times 6 intial in 3 hours and then additional 4 after 2 days and additional 4 after 7 days
            // deepSleep(thirtyDaySleepTimer*3600); // Sleep for 30 days
            } else if (wifiConCntrLocal=16) { // Total try 14 times 6 intial in 3 hours and then additional 4 after 2 days and additional 4 after 7 days and additional 4 after 30 days
              // Reset the times
              Serial.println("Sleep the ESP wifiConCntrLocal =16");
              user_info.wifiConCntr = -100;  // Reset to -1 default is 0, -1 will be used to try one more time after, setting to -100 as it will increment each time reset is clicked and set to 0 when id passwd saved
              EEPROM.put(0, user_info);
              EEPROM.commit(); 
              // Sleep for 10 Years until reset is pressed
              longdeepSleep(tenYearDaySleepTimer*3600, tenYearDaySleepTimer); // 1 hour sleep cycle and hence number of sleep cycle is same as number of hours for sleep
            } else {
              Serial.println("Sleep the ESP wifiConCntrLocal < 4");
              deepSleep(sleepTimer); // let it go to sleep for 30 mins, if it doesn't more retry alert can be made visible on app that recording is not happening, please check wifi strength
              //restartESP();
            }
            */
      } // Saving something to permanent memory so it doesn't go in infinite loop after number of retiries to save batteries.

    } while(wifiNotConnCntr < 1 && wifiSavedFlag && httpRequestCntr < 3); // Retry 2 times ESP will restart after 2 tries? should it go to sleep

    configPortalTimeControl=millis();   
  } else {
    loopLogic = false;
  }
  
 
}


void loop() {


// Wait for 5 mins and then go to indefinate sleep 
if((millis()-configPortalTimeControl>300000) && loopLogic) { 
  // If config portal is on for more than 5 minutes put the ESP to sleep forever until reset button is clicked
  deepSleep(0);
}


}