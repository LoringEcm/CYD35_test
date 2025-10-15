/*
  Initial development/test of the Cheap Yellow Display (CYD) for
    the ESP-3248S035 (3.5" screen) board, which exercises the 
    display graphics, Touch interface, and the Wireless interface (2.4G)

  This project was developed on Windows 11 under (so the PlatformIO '.ini'
  file is configured to) the following directory structure:

  c:/
    /Archive
        /Projects
            /cyd35_test2
            /libraries
                /TFT_eSPI
                /AsyncTCP
                /ESPAsyncWebServer
                /gt911-arduino
 
  All four libraries were the latest versions (ie, 'master') as of 10/15/25
  downloaded from GitHub.
  
  No other libraries are required.
  
  The libraries are built external to the project, so that they will only be
  built once at installation.  If you perform a PlatformIO 'clean' ('clean' on
  the upper-right pull-down) they will be rebuilt within the project.
  
  A compressed (*.zip) copy of a 'clean' version of the project folder ('cyd35_test2')
  results in a file of 52K (since the library object code isn't present).
  
  The code will initialize the CYD screen showing two circles (one 'filled' and one not), as
  well as two buttons labeled 'Press #1' and 'WebServer'.  Pressing anywhere on the screen will
  output the x:y touch coordinates on the PlatformIO serial monitor (when run in that mode). 
  
  If the user button 'Press #1' is pressed the a 'btn1' message appears in the serial monitor
  showing the x:y coordinates of the press as well as the top/bottom/left/right boundaries of the
  'Press #1' button.

  If the user button 'WebServer' is pressed on the CYD screen the button metrics are show as above,
  and the user  screen changes to display the current status of the 3 webserver buttons and slider, 
  and the (2.4G) webserver is started.  The default IP address is 192.168.4.1 (http only, not https),
  and the default password is 'password'. The user must connect their computer to the webserver wireless
  via their (computer specific) Internet connection API. Once connected any browser may be used to attach
  to the webpage at 192.168.4.1.  This page shows three buttons and a slider.  The buttons may be clicked
  on/off_type and the slider graphic may be slid over the 0-255 range.  Updates are once-per-second.  All
  actions performed on the web page are echoed on the CYD status screen (in this mode the user 'touch' is 
  disabled on the CYD).  Pressing either of the top two 'buttonWsSamp' buttons simply updates their status
  on both the webpage and CYD. Updating the slider via the graphic likewise updates both screens.
  
  Pressing the webpage 'WebServerExit' button causes the webpage processing to be disabled and restores
  the CYD GUI.
  
  Toggling between the two screens may be repeated as often as desired. The wireless connection will sustain
  over multiple switches of the displays.
  
  The ESP32-3248S035 screen resolution is 320 x 480

  The screen is configured in landscape mode, as:
    TFT API - rotation 'inverted' (ie, '1')
    GT911 API - rotation is 'ROTATION_RIGHT'
      (in this configuration the screen is in LANDSCAPE mode
        with the Point-Of-Origin at the upper-left, and the X
        axis counting RIGHT from the POO, and the Y axis
        counting DOWN from the POO.)
  
  The display is configured in Landscape mode for a point of origin 
  at the upper-left corner:
      rhe X axis goes RIGHT horizontally from 0 -> 480
      the Y axis goes DOWN  vertically   from 0 -> 320

  Bill Graham     version: 20251015
  billgraham214@gmail.com

*/

#include <TFT_eSPI.h>   // Include the TFT_eSPI library
#include "TAMC_GT911.h" // For capacitive touch

// comment-out the following line to disable button debug console prints
#define DEBUG_BUTTONS
#define DEBUG_HW_TIMER

TFT_eSPI tft = TFT_eSPI(); // Create an instance of the library

TFT_eSPI_Button button1, button2;

// Button properties
#define BUTTON_1_X  340
#define BUTTON_1_Y  100
#define BUTTON_2_X  350
#define BUTTON_2_Y  240
#define BUTTON_W    180
#define BUTTON_H    60
#define BUTTON_TEXT_SIZE 2
#define BUTTON_COLOR TFT_BLUE
#define BUTTON_BG_COLOR TFT_DARKGREY
#define BUTTON_BORDER_COLOR TFT_WHITE

// --- Touch Setup start ---
#define TOUCH_SDA       33
#define TOUCH_SCL       32
#define TOUCH_INT       21
#define TOUCH_RST       25
#define TOUCH_WIDTH     480
#define TOUCH_HEIGHT    320

TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);
void checkTouch();
// --- Touch Setup end ---

// ========================================================
// === Web Server Code Begin ==============================
// ========================================================
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// --- Wi-Fi Configuration ---
const char* ssid = "ESP32_Webserver"; // The name of the Wi-Fi network
const char* password = "password";    // The password

// --- Web Server Setup ---
AsyncWebServer server(80);
void tsDrawGuiMain();       // Touch Screen Graphical User Interface - Top Level 'Main'

// --- State Variables ---
bool wsSampleButtonNewState = false;
bool wsSampleButtonState = false;
bool wsExitButtonState = false;
int  sliderValue = 127; // 0-255 range
bool bDisableTouchScreenDetection = false;

// --- HTML for the Web Page ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Control</title>
  <style>
    html { font-family: Arial; display: inline-block; text-align: center; }
    h2 { font-size: 1.7rem; }
    p { font-size: 1.5rem; padding: 0.1rem; }
    body { max-width: 600px; margin: 0px auto; padding-bottom: 25px; font-size: 1.3rem; }
    .slider { width: 300px; }
    .button { padding: 10px 40px; 
      width: 300px; height: 3.7rem; 
      background:#e3b7b3; color:white;
      border: 2px solid black; border-radius: 15px;
      text-decoration: none; font-size: 1.4rem;
      margin: 5px; cursor: pointer      }
    .buttonGreen { background:#51b05d; color:white; }
    .buttonRed   { background:#e3b7b3; color:white; }
    }
  </style>
</head>
<body>
  <h2>ESP32 Web Server</h2>
  <p></p>
  <button type="button" class="button" id="wsSampleButton">buttonWsSamp OFF</button>
  <button type="button" class="button" id="wsSampleButtonNew">buttonWsSampNew OFF</button>
  <button type="button" class="button" id="wsExitButton">WebServerExit OFF</button>
  <p></p>
  <p>Slider: <span id="sliderValue"></span></p>
  <input type="range" onchange="updateSlider(this)" id="mySlider" min="0" max="255" value="127" class="slider">
  <p></p>
<script>
  // *************************************************
  //
  function syncButtonOn_New(isOn) {
    let buttonStateText = (isOn == true) ? "ON" : "OFF";
    document.getElementById("wsSampleButtonNew").innerText = "buttonWsSampNew "+buttonStateText;
    document.getElementById("wsSampleButtonNew").classList.toggle('buttonGreen', isOn);
  }
  function syncButtonOn_Sample(isOn) {
    let buttonStateText = (isOn == true) ? "ON" : "OFF";
    document.getElementById("wsSampleButton").innerText = "buttonWsSamp "+buttonStateText;
    document.getElementById("wsSampleButton").classList.toggle('buttonGreen', isOn);
  }
  function syncButtonOn_Exit(isOn) {
    let buttonStateText = (isOn == true) ? "ON" : "OFF";
    document.getElementById("wsExitButton").innerText = "WebServerExit  "+buttonStateText;
    document.getElementById("wsExitButton").classList.toggle('buttonGreen', isOn);
  }
  //
  // *************************************************
  const wsSampleButtonNewPressed = (e) => {
    // new state is opposite the incoming state, so invert the incoming (/current) state
    let newButtonState = !(e.target.innerText == "buttonWsSampNew ON");   
    syncButtonOn_New(newButtonState);
   
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/WebServerSampleButtonNew?value="+(newButtonState ? "ON" : "OFF"), newButtonState);
    xhttp.send();
  }
  const sampleButtonNewRef = document.getElementById("wsSampleButtonNew");
  sampleButtonNewRef.addEventListener("click", wsSampleButtonNewPressed );
  //
  // *************************************************
  const wsSampleButtonPressed = (e) => {
    // new state is opposite the incoming state, so invert the incoming (/current) state
    let newButtonState = !(e.target.innerText == "buttonWsSamp ON");   
    syncButtonOn_Sample(newButtonState);
   
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/WebServerSampleButton?value="+(newButtonState ? "ON" : "OFF"), newButtonState);
    xhttp.send();
  }
  const sampleButtonRef = document.getElementById("wsSampleButton");
  sampleButtonRef.addEventListener("click", wsSampleButtonPressed );
  //
  // *************************************************
  const wsExitButtonPressed = (e) => {
    // new state is opposite the incoming state, so invert the incoming (/current) state
    let newButtonState = !(e.target.innerText == "Web Server Exit ON");   
    syncButtonOn_Exit(newButtonState);

    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/WebServerExitButton?value="+(newButtonState ? "ON" : "OFF"), newButtonState);
    xhttp.send();
  }
  const exitButtonRef   = document.getElementById("wsExitButton");
  exitButtonRef.addEventListener("click", wsExitButtonPressed );
  //
  // *************************************************
  function updateSlider(element) {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/slider?value="+element.value, true);
    xhttp.send();
  }
  // In case of a web server 'reload' this code will capture the 'working' value of 
  //  the 'slider' c++ variable
  sliderElement = document.getElementById("sliderValue");
  sliderElement.value = "-1";     // flag the code not to update the c++ variable with this value
  updateSlider(sliderElement);
  const sliderGraph = document.getElementById("mySlider");
  //
  // *************************************************
  setInterval(function() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
  
      if (this.readyState == 4 && this.status == 200) {
        var data = JSON.parse(this.responseText);
        console.log("responseText: " + this.responseText);
        document.getElementById("sliderValue").innerHTML = data.slider;
        sliderGraph.value = data.slider;

        syncButtonOn_New(data.wsSampleButtonNewState);
        syncButtonOn_Sample(data.wsSampleButtonState);
        syncButtonOn_Exit(data.wsExitButtonState);
      }
    }
  xhttp.open("GET", "/status", true);
  xhttp.send();
}, 1000);
</script>
</body>
</html>
)rawliteral";


// --- Function Prototypes ---
void notFound(AsyncWebServerRequest *request);
void handleRoot(AsyncWebServerRequest *request);
void handleWebServerSampleButton(AsyncWebServerRequest *request);
void handleWebServerSampleButtonNew(AsyncWebServerRequest *request);
void handleWebServerExitButton(AsyncWebServerRequest *request);
void handleSlider(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
void updateCydDisplayWhenWebServerIsRunning();

// ========================================================
// === Web Server Code End ===============================
// ========================================================

//
// *************************************************
// The primary top-level 'setup' method which the ESP32 (/Arduino) run-time environment expects to call (once)
void setup() {                // Config TFT_eSPI native graphics and Touch support only
                              //    (note: the WebServer is started later via User button)
  tft.init();                 // Initialize the TFT display

  tft.setRotation(1);         // Set rotation for landscape mode

  Serial.begin(115200);
  
  // Initialize touch screen
  ts.begin();
  ts.setRotation( ROTATION_RIGHT );

  tsDrawGuiMain();
}

//
// *************************************************
void tsDrawGuiMain() {
  tft.fillScreen(TFT_WHITE);  // Clear the screen with a white background

  // Draw a filled circle
  int x = tft.width() / 3;    // Center horizontally
  int y = tft.height() / 4;   // Position vertically
  int r = 50;                 // Radius
  uint16_t color = TFT_BLUE;  // Use a blue color
  tft.fillCircle(x, y, r, color); 

  // Draw a circle outlined
  x = tft.width() / 3;        // Center horizontally
  y = tft.height() * 3 / 4;   // Position vertically
  r = 50;                     // Radius
  color = TFT_RED;            // Use a red color
  tft.drawCircle(x, y, r, color); 
  
  Serial.println("ESP32-3248S035 Button Demo");

  // At this point the display point of origin is the upper-left corner.
  //    rhe X axis goes RIGHT horizontally from 0 -> 480
  //    the Y axis goes DOWN  vertically   from 0 -> 320

  // Initialize the CYD test button #1
  button1.initButton(&tft, BUTTON_1_X, BUTTON_1_Y, BUTTON_W, BUTTON_H,
                     TFT_WHITE, BUTTON_COLOR, BUTTON_BORDER_COLOR,
                     (char *)"Press #1", BUTTON_TEXT_SIZE);
  
  // Draw the button on the screen
  button1.drawButton();
  button1.press( true );      // initialize 'press' state

  // Initialize the CYD test button #2
  button2.initButton(&tft, BUTTON_2_X, BUTTON_2_Y, BUTTON_W, BUTTON_H,
                     TFT_WHITE, BUTTON_COLOR, BUTTON_BORDER_COLOR,
                     (char *)"WebServer", BUTTON_TEXT_SIZE);
 
  // Draw the CYD buttons on the screen
  button2.drawButton();
  button2.press( true );      // initialize 'press' state
}

// *******************************************************
// === Web Server Start Code Begin =======================
// *******************************************************
void  startWebServer() {      // started by the user via the 'WebServer' button
  // Connect to Wi-Fi
  Serial.println("Setting up Access Point...");
  WiFi.softAP(ssid, password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/WebServerSampleButtonNew", HTTP_GET, handleWebServerSampleButtonNew);
  server.on("/WebServerSampleButton", HTTP_GET, handleWebServerSampleButton);
  server.on("/WebServerExitButton",   HTTP_GET, handleWebServerExitButton);
  server.on("/slider", HTTP_GET, handleSlider);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(notFound);

  server.begin();     // Startup the web server

  updateCydDisplayWhenWebServerIsRunning();        // Initial display update
}

void  stopWebServer() {     // Stopped by the user via the 'WebServer' button
  Serial.println("Stopping Web Server ***");
  server.reset();           // Reset the web server
  server.end();             // Stop  the web server

  updateCydDisplayWhenWebServerIsRunning();        // Restore the native UI display update
}
// *******************************************************
// === Web Server Start Code End =======================
// *******************************************************

// The primary top-level 'loop' method which the ESP32 (/Arduino) run-time environment expects to poll
void loop() {
  // Add any ongoing animation or logic here if needed
  // For this simple example, we poll the Touch display.
  if (!bDisableTouchScreenDetection) {
    checkTouch();     // Check for screen touches
  }
  delay(75);        // Small delay to prevent blocking and help lower power consumption
}

// *******************************************************
// === Web Server Support Code Begin =====================
// *******************************************************

// The WebServer event 'handlers' (based on the User initiated 'event')
//
// *************************************************
void notFound(AsyncWebServerRequest *request) {
  if (request->hasParam("id")) {
    const char * idVal = (const char *)request;
    Serial.print("Not Found - id: "); Serial.println(idVal);
    updateCydDisplayWhenWebServerIsRunning();
  }
  request->send(404, "text/plain", "Not found");
}
//
// 'void AsyncWebServerRequest::send_P(int, const String&, const char*, AwsTemplateProcessor)' is deprecated: Replaced by send(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) [-Wdeprecated-declarations]
// *************************************************
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html);
}
//
// *************************************************
void handleWebServerSampleButtonNew(AsyncWebServerRequest *request) {

  if (request->hasParam("value")) {
    String value = request->getParam("value")->value();
    wsSampleButtonNewState = (value == "ON") ? true : false;
    Serial.print("WebServerSampleButtonNew state changed to: ");
    Serial.println(wsSampleButtonNewState ? "ON" : "OFF");
    updateCydDisplayWhenWebServerIsRunning();
  }
  request->redirect("/");
}
//
// *************************************************
void handleWebServerSampleButton(AsyncWebServerRequest *request) {

  if (request->hasParam("value")) {
    String value = request->getParam("value")->value();
    wsSampleButtonState = !wsSampleButtonState;
    Serial.print("WebServerSampleButton state changed to: ");
    Serial.println(wsSampleButtonState ? "ON" : "OFF");
    updateCydDisplayWhenWebServerIsRunning();
  }
  request->redirect("/");
}
//
// *************************************************
void handleWebServerExitButton(AsyncWebServerRequest *request) {

  String message = "No message received";
  if (request->hasParam("value")) {
    String value = request->getParam("value")->value();
    wsExitButtonState = !wsExitButtonState;
    Serial.print("WebServerEnd button state changed to: ");
    Serial.println(wsExitButtonState ? "ON" : "OFF");
    if ( wsExitButtonState ) {
      Serial.println("Stopping Web Server");
      
      delay( 150 );
      stopWebServer();      // Stop the web server
      server.end();         //
      bDisableTouchScreenDetection  = false;
      wsExitButtonState             = false;  // configure for next re-entry if performed
      tsDrawGuiMain();      // Re-draw top level Touch Screen 'Main' GUI
    }
    else {
      updateCydDisplayWhenWebServerIsRunning();
    }
  }
  request->redirect("/");
}
//
// *************************************************
void handleSlider(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {

    int temp = request->getParam("value")->value().toInt();
    char buf[256];
    sprintf(buf, "Slider value incoming is: %d", temp);
    Serial.println(buf);

    if ( temp > 0 ) {     // a negative value (ie, '-1') means just return the current c++ variable value
      sliderValue = temp;
    }
    // sliderValue = request->getParam("value")->value().toInt();
    Serial.print("Slider value changed to: ");
    Serial.println(sliderValue);
    updateCydDisplayWhenWebServerIsRunning();
  }
  request->redirect("/");
}
//
// *************************************************
void handleStatus(AsyncWebServerRequest *request) {  
  String json =  "{\"wsSampleButtonState\":"     + String(wsSampleButtonState ? "true" : "false")    +
                ", \"wsExitButtonState\":"       + String(wsExitButtonState ? "true" : "false")      +
                ", \"wsSampleButtonNewState\":"  + String(wsSampleButtonNewState ? "true" : "false") + 
                ", \"slider\":"                  + String(sliderValue)                               + "}";
#if 1
  Serial.println("debugJSON: "+json);
#endif
  request->send(200, "application/json", json);
}
//
// *************************************************
void updateCydDisplayWhenWebServerIsRunning() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.println("Web Server Controlling");
  tft.println("System Status Display");

  // Display Sample button state
  tft.setCursor(10, 70);
  tft.setTextColor(wsSampleButtonState ? TFT_GREEN : TFT_RED);
  tft.print("Sample Button: ");
  tft.println(wsSampleButtonState ? "ON" : "OFF");

  // Display Sample button state New
  tft.setCursor(10, 90);
  tft.setTextColor(wsSampleButtonNewState ? TFT_GREEN : TFT_RED);
  tft.print("Sample Button New: ");
  tft.println(wsSampleButtonNewState ? "ON" : "OFF");

  // Display Exit button state
  tft.setCursor(10, 110);
  tft.setTextColor(wsExitButtonState ? TFT_GREEN : TFT_RED);
  tft.print("ServerEnd Button: ");
  tft.println(wsExitButtonState ? "ON" : "OFF");
  
  // Display slider value
  tft.setCursor(10, 140);
  tft.setTextColor(TFT_CYAN);
  tft.print("Slider: ");
  tft.println(sliderValue);

  // Draw a visual slider bar
  int barWidth = map(sliderValue, 0, 255, 0, tft.width() - 20);
  tft.drawRect(10, 170, tft.width() - 20, 20, TFT_WHITE);
  tft.fillRect(10, 170, barWidth, 20, TFT_YELLOW);
}
// *******************************************************
// === Web Server Support Code End =======================
// *******************************************************


void checkTouch() {
  static bool printTouchLock = false;
  static uint32_t printTouchTimer = 0;

  // Keep reading the touch screen interface even when the Touch Screen 
  //  processing is suspended to ensure the I2C channel remains healthy
  //  during the suspension period
  //
  ts.read();

  if ( !printTouchLock ) {
    if ( ts.isTouched ) {
      bool buttonWasPressed = false;
      for (int i=0; i<ts.touches; i++) {
#ifdef DEBUG_BUTTONS
        static char buf[ 160 ];

        Serial.print("Touch #");Serial.print(i+1);Serial.print(":");
        Serial.print("  x:");Serial.print(ts.points[i].x);
        Serial.print("  y:");Serial.print(ts.points[i].y);
        Serial.print("  size:");Serial.println(ts.points[i].size);
#endif    // endif DEBUG_BUTTONS

        if ( button1.contains( ts.points[i].x, ts.points[i].y ) ) {
          buttonWasPressed = true;
          if ( button1.justPressed() ) {
            // Invert the button's appearance to show it was pressed
            button1.drawButton(true);
          }
          else if ( button1.justReleased() ) {
            button1.drawButton(false);
          }
#ifdef DEBUG_BUTTONS
          sprintf(buf, "    Btn1   x:%3d  y:%3d  H:%3d  W:%3d  left:%4.1f  right:%4.1f  top:%5.1f  bot:%4.1f ST:%d", 
                  BUTTON_1_X, BUTTON_1_Y, BUTTON_H, BUTTON_W,
                  320 - (BUTTON_1_Y * .5), 
                  480 - (BUTTON_1_X * .5), 
                  BUTTON_1_Y - (BUTTON_H * 0.5),
                  BUTTON_1_Y + (BUTTON_H * 0.5), (int)button1.isPressed() ); 
          Serial.println(buf);
#endif    // endif DEBUG_BUTTONS
          button1.press( !button1.isPressed() );
        }

        if ( button2.contains( ts.points[i].x, ts.points[i].y ) ) {
          buttonWasPressed = true;
          if ( button2.justPressed() ) {
            // Invert the button's appearance to show it was pressed
            button2.drawButton(true);
          }
          else if ( button2.justReleased() ) {
            button2.drawButton(false);
          }
#ifdef DEBUG_BUTTONS
          sprintf(buf, "    Btn2   x:%3d  y:%3d  H:%3d  W:%3d  left:%4.1f  right:%4.1f  top:%5.1f  bot:%4.1f ST:%d",  
                  BUTTON_2_X, BUTTON_2_Y, BUTTON_H, BUTTON_W,
                  320 - (BUTTON_2_Y * .5), 
                  480 - (BUTTON_2_X * .5), 
                  BUTTON_2_Y - (BUTTON_H * 0.5),
                  BUTTON_2_Y + (BUTTON_H * 0.5), (int)button2.isPressed() );  
          Serial.println(buf);
#endif    // endif DEBUG_BUTTONS

          button2.press( !button2.isPressed() );

          bDisableTouchScreenDetection = true;
          startWebServer();   // startup the web server
        }

        if ( buttonWasPressed ) {
          printTouchTimer = millis();     // capture the time the button was pushed
#ifdef DEBUG_HW_TIMER
          Serial.print("   << printTouchTimer start: "); Serial.println( printTouchTimer );
#endif  // if DEBUG_HW_TIMER
          printTouchLock = true;          // block further touch processing until the timer expires
          break;    // no need to process any more touch points in this session
        }
      }
    }
  }

  // debounce the touch detection by ~500 milliseconds or so using the hardware timer
  // note: the software 'delay()' timer will not work in this application)
  if ( printTouchLock && ( millis() - printTouchTimer > 500 ) ) {
#ifdef DEBUG_HW_TIMER
    Serial.print("   << printTouchTimer done: "); Serial.println( millis() );
#endif  // if DEBUG_HW_TIMER
    printTouchLock = false;
  }
}
