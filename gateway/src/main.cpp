//************************************************************
// for more details
// https://gitlab.com/painlessMesh/painlessMesh/wikis/bridge-between-mesh-and-another-network
// for more details about my version
// https://gitlab.com/Assassynv__V/painlessMesh
// and for more details about the AsyncWebserver library
// https://github.com/me-no-dev/ESPAsyncWebServer
//************************************************************
#include "Arduino.h"
#include "IPAddress.h"
#include "painlessMesh.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <string.h>

#include "sdkconfig.h"

#define   MESH_PREFIX     "mesh"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555
#define   MESH_CHANNEL    6

#define HOSTNAME "mesh-gateway"

// Prototype
void socketClient(String outboundMsg, String destIp, int destPort);
void receivedCallback( const uint32_t &from, const String &msg );
IPAddress getlocalIP();

bool busy = false;

painlessMesh  mesh;
AsyncWebServer server(80);
IPAddress myIP(0,0,0,0);
IPAddress myAPIP(0,0,0,0);

Scheduler userScheduler;
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, [] () {
  
} );

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );

  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(&receivedCallback);

  mesh.stationManual("Siloo", "Siloo1399");
  mesh.setHostname(HOSTNAME);

  myAPIP = IPAddress(mesh.getAPIP());
  Serial.println("My AP IP is " + myAPIP.toString());

  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form>SSID<br><input type='text' name='ssid'><br><br>Password<br><input type='password' name='password'><br><br><input type='submit' value='Connect'></form>");
    if (request->hasArg("ssid") && request->hasArg("password")){
      String ssid = request->arg("ssid");
      String password = request->arg("password");
      mesh.stationManual(ssid, password);
      mesh.setHostname(HOSTNAME);

      myAPIP = IPAddress(mesh.getAPIP());
      Serial.println("My AP IP is " + myAPIP.toString());
    }
  });
  server.begin();

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
  if(myIP != getlocalIP()){
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());
  }
}

void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("gateway: Received from %u msg=%s\n", from, msg.c_str());
  if (busy) {
    Serial.printf("gateway: Busy right now!");
    return;
  }
  busy = true;  // -- LOCKED --

  DynamicJsonDocument jsonBuffer(1024 + msg.length());
  deserializeJson(jsonBuffer, msg);
  JsonObject root = jsonBuffer.as<JsonObject>();
  String outboundMsg = root["msg"].as<String>();
  String destIp = root["destIp"].as<String>();
  int destPort = root["destPort"].as<int>();
  socketClient(outboundMsg, destIp, destPort);
  Serial.println(">>> sent");


  busy = false; // -- UNLOCKED --
}


// [https://github.com/nkolban/esp32-snippets/]
void socketClient(String outboundMsg, String destIp, int destPort) {
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	Serial.printf("socket: rc: %d\n", sock);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	inet_pton(AF_INET, destIp.c_str(), &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(destPort);

	int rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
	Serial.printf("connect rc: %d\n", rc);

	rc = send(sock, outboundMsg.c_str(), strlen(outboundMsg.c_str()), 0);
  Serial.printf("send: rc: %d\n", rc);
	rc = close(sock);
  Serial.printf("close: rc: %d\n", rc);
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}
