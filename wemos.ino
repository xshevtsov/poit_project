/****************************************************************************************************************************
  ESP8266_WebSocketClientSocketIO.ino
  For ESP8266

  Based on and modified from WebSockets libarary https://github.com/Links2004/arduinoWebSockets
  to support other boards such as  SAMD21, SAMD51, Adafruit's nRF52 boards, etc.

  Built by Khoi Hoang https://github.com/khoih-prog/WebSockets_Generic
  Licensed under MIT license

  Originally Created on: 06.06.2016
  Original Author: Markus Sattler
*****************************************************************************************************************************/

#if !defined(ESP8266)
  #error This code is intended to run only on the ESP8266 boards ! Please check your Tools->Board setting.
#endif

#define _WEBSOCKETS_LOGLEVEL_     2

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ArduinoJson.h>

#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>

#include <Hash.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>


#include <TimerMs.h>

TimerMs displayData(1000, 1, 0);




#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BMP280 bmp;




ESP8266WiFiMulti WiFiMulti;
SocketIOclient socketIO;

// Select the IP address according to your local network
IPAddress serverIP(192, 168, 1, 16);
uint16_t  serverPort = 5000;    //8080;    //3000;


// int* drawLine(int x1, int y1, int x2, int y2, int n) {

//     double v_x = x2 - x1;
//     double v_y = y2 - y1;
//     double length = sqrt(v_x * v_x + v_y * v_y);
//     double unitary_x = v_x / length;
//     double unitary_y = v_y / length;

//     int* points = (int*)malloc(n * 2 * sizeof(int)); // Allocate memory for n points, each with x and y

//     for (int i = 0; i < n; i++) {
//         points[2 * i] = round(x1 + unitary_x * i);     // x-coordinate
//         Serial.print(round(x1 + unitary_x * i));
//         points[2 * i + 1] = round(y1 + unitary_y * i); // y-coordinate
//         Serial.print(round(y1 + unitary_y * i));
//         Serial.println();
//     }

//     return points; // Return the array of points
// }

void socketIOEvent(const socketIOmessageType_t& type, uint8_t * payload, const size_t& length)
{
  StaticJsonDocument<128> doc;
  StaticJsonDocument<64> doc1;
  switch (type)
  {
    case sIOtype_DISCONNECT:
      Serial.println("[IOc] Disconnected");
      break;

    case sIOtype_CONNECT:
      Serial.print("[IOc] Connected to url: ");
      Serial.println((char*) payload);

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");

      break;

    case sIOtype_EVENT:
      // Serial.print("[IOc] Get event: ");

      
      
      deserializeJson(doc, payload);
      deserializeJson(doc1, doc[1]);
    
      if(doc[0] == "SendDisplayData"){
        if(doc1["x"].as<int>() >= 0 && doc1["y"].as<int>() >= 0){
          display.drawPixel(doc1["x"].as<int>(),doc1["y"].as<int>(),1);
          display.display();
          
        }

      }
      if(doc[0] == "clearDisplay"){
        display.clearDisplay();
        display.display();

      } 

      // serializeJson(doc1["x"].as<int>(), Serial);

      // Serial.println("test"+doc[1]['x'].as<int>());
      // // Serial.println(doc[1]["x"].as<int>());



      // Serial.println((char*) payload);

      break;

    case sIOtype_ACK:
      Serial.print("[IOc] Get ack: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_ERROR:
      Serial.print("[IOc] Get error: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_BINARY_EVENT:
      Serial.print("[IOc] Get binary: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_BINARY_ACK:
      Serial.print("[IOc] Get binary ack: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_PING:
      Serial.println("[IOc] Get PING");

      break;

    case sIOtype_PONG:
      Serial.println("[IOc] Get PONG");

      break;

    default:
      break;
  }
}

void setup()
{
  // Serial.begin(921600);
  Serial.begin(9600);

  while (!Serial);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.display();
  // display.setTextSize(1);
  // display.setTextColor(WHITE);
  // display.setCursor(0, 10);
  // Display static text

  if(!bmp.begin(BMP280_ADDRESS_ALT) ) { 
    Serial.println("BMP280 SENSOR ERROR"); 
    while(1); 
  }
  
  




  Serial.print("\nStart ESP8266_WebSocketClientSocketIO on ");
  Serial.println(ARDUINO_BOARD);
  Serial.println(WEBSOCKETS_GENERIC_VERSION);

  //Serial.setDebugOutput(true);

  // disable AP
  if (WiFi.getMode() & WIFI_AP)
  {
    WiFi.softAPdisconnect(true);
  }

  WiFiMulti.addAP("T-WiFi-489266", "768mjs6mduk7");

  //WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println();

  // Client address
  Serial.print("WebSockets Client started @ IP address: ");
  Serial.println(WiFi.localIP());

  // server address, port and URL
  Serial.print("Connecting to WebSockets Server @ IP address: ");
  Serial.print(serverIP);
  Serial.print(", port: ");
  Serial.println(serverPort);

  // setReconnectInterval to 10s, new from v2.5.1 to avoid flooding server. Default is 0.5s
  socketIO.setReconnectInterval(500);

  socketIO.setExtraHeaders("Authorization: 1234567890");

  // server address, port and URL
  // void begin(IPAddress host, uint16_t port, String url = "/socket.io/?EIO=4", String protocol = "arduino");
  // To use default EIO=4 fron v2.5.1
  socketIO.begin(serverIP, serverPort);

  // event handler
  socketIO.onEvent(socketIOEvent);

  


}

// String getDisplayBuffer() {
//   DynamicJsonDocument doc(8192);
//   for (size_t i = 0; i < 64; i++) {
//     JsonArray row = doc.createNestedArray();
//     for(size_t u = 0; u < 128; u++){
//       row.add((int)display.getPixel(u,i));
//       // Serial.print(display.getPixel(u,i));
      
//     }
//     // Serial.println();
  
//   }
  

//   // Serialize the JSON document to a string
//   String output;
//   serializeJson(doc, output);


//   return output;
  
// }

unsigned long messageTimestamp = 0;

void loop()
{
  socketIO.loop();

 

  if(displayData.tick()){
    // display.clearDisplay();
    // display.setCursor(0, 0);
    // display.println(WiFi.localIP());
    // display.println(bmp.readTemperature());
    // display.println(bmp.readPressure()/100.0F);
    // display.drawFastHLine(50,50,10,1);
    // display.display(); 
    

    

    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add evnet name
    // Hint: socket.on('event_name', ....
    array.add("graph_data");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();
    param1["temp"]     = bmp.readTemperature();
    param1["pressure"]  = bmp.readPressure()/100.0F;

    // Serial.println(getDisplayBuffer());
    // param1["display_buffer"] = getDisplayBuffer();

   
    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);

    // Send event
    socketIO.sendEVENT(output);

    // // Print JSON for debugging
    // Serial.println(output);
  }

}


  
