#include "painlessMesh.h"
#include "time.h"

#define   MESH_PREFIX     "mesh"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define   GATEWAY_NODE    622108909

Scheduler userScheduler;
painlessMesh  mesh;

void sendMessage();

Task taskSendMessage( TASK_SECOND * 30 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  DynamicJsonDocument jsonBuffer(1024);
  JsonObject json = jsonBuffer.to<JsonObject>();
  String msg = "This is a test message from ";
  msg += mesh.getNodeId();
  json["msg"] = msg;
  json["destIp"] = "192.168.1.151";
  json["destPort"] = "8080";
  String jsonStr;
  serializeJson(json, jsonStr);
  mesh.sendSingle(GATEWAY_NODE, jsonStr);
  Serial.printf("I'm sending a message to the outside world.\n");
}

void receivedCallback( uint32_t from, String &msg ) {}

void newConnectionCallback(uint32_t nodeId) {}

void changedConnectionCallback() {}

void nodeTimeAdjustedCallback(int32_t offset) {}

void setup() {
  Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT); // will she auto-detect the channel?
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}
