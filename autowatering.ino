/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "DaSE-314-Printer";
const char* password = "dase314314";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);




int WIFI_STATUS_PIN = D7;
int PUMP_PIN = D2;
int pump_power_status = 0;
int pump_ready = 0;
long pump_duration = 0;
unsigned long pump_end_time = 0;



void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare GPIO
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, 0);

  pinMode(WIFI_STATUS_PIN, OUTPUT);
  digitalWrite(WIFI_STATUS_PIN, 0);

  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());  

  digitalWrite(WIFI_STATUS_PIN, 1);
}

void loop() {
  

  //Check the time and control the power of the pump
  if (1 == pump_power_status) {
    int time_now = millis();
    if (time_now > pump_end_time) {
      pump_power_status = 0;
      digitalWrite(PUMP_PIN,pump_power_status);
    }
  }
  if (pump_ready) {
    pump_ready = 0;
    pump_end_time = millis() + pump_duration * 1000;
    if (0 == pump_power_status) {
      pump_power_status = 1;
      digitalWrite(PUMP_PIN,pump_power_status);
    }
  }


  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int idx = req.indexOf("/gpio/");
  String response_text = "";
  if (idx != -1) {
    String str_val = req.substring(idx + 6);
    pump_duration = str_val.toInt(); //seconds
    if (pump_duration > 0 && pump_duration <= 600) { //limit to 1 min
      pump_ready = 1;
      response_text += "Pump is ready to water for ";
      response_text +=  pump_duration;
      response_text +=  " seconds.";
    } else if (pump_duration > 600) {
      response_text += "The watering time is ";
      response_text += pump_duration;
      response_text += " which is too long, should be less than 600 seconds.";
    } else if (-1 == pump_duration) {
      // emergent stop!
      pump_ready = 0;
      pump_power_status = 0;
      digitalWrite(PUMP_PIN,0);
      response_text += "The watering stops now.";
    } else {
      response_text += "Invalid request";
    }
    Serial.println(response_text); 
  }
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
  s += response_text + "\r\n";
  s += "</html>\r\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
  
  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}



