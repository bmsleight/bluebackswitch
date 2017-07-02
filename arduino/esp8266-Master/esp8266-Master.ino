#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "local_passwords.h"
// Peter Lerup wonderful espsoftwareserial
#include <SoftwareSerial.h>

#define LED 2           // NodeMCU LED
#define BTBRK 4         // BlueTooth Pin to break connection
#define BTCONN 5         // BlueTooth Pin to connection status
#define DEVICENAME "BigBlueM123"


SoftwareSerial BTserial(14, 12, false, 256); // RX | TX
// HTTP server will listen at port 80
ESP8266WebServer server(80);
String inputString = ""; // a string to hold incoming data
bool confirmed = true;   // All commands have been confirmed
char c = ' ';



String send_bt_at_command(String command, bool reply_to_serial = false) {
  String full_command = "AT+" + command;
  String full_reply = "";
  int simple_time = 0;
  for (int i = 0; i < full_command.length(); i++)
  {
    BTserial.write(full_command[i]);   // Push each char 1 by 1 
  }
  // Wait for a reply or 1000ms
  while(simple_time <5000)
  {
    if(BTserial.available())
    {
      simple_time = 5000;
    }
    delay(1);
    simple_time += 1;
  }
  // Assuming nothing else was waiting
  while (BTserial.available())
  {
    c = BTserial.read();
    full_reply += c;
    delay(1); // 960 symbol /sec
  }
  full_reply.remove(0, 3); // Remove AT+
  if(reply_to_serial)
  {
    for (int i = 0; i < full_reply.length(); i++)
    {
      Serial.write(full_reply[i]);   // Push each char 1 by 1 on each loop pass
    }
  }
  return full_reply;
}

void reset_connection() {
  digitalWrite(BTBRK, LOW);
  delay(1500);
  digitalWrite(BTBRK, HIGH);
}

void reset_bluetooth()  {
  Serial.println("Reset Bluetooth");
  delay(2500);
  BTserial.println("AT");
  send_bt_at_command("RENEW", false);
  send_bt_at_command(String("NAME") + String(DEVICENAME), true);
  send_bt_at_command("IMME1", true);
  send_bt_at_command("ROLE1", true);
  send_bt_at_command("SHOW1", true);
  delay(1000);
  send_bt_at_command("DISC?", true);
  delay(7500);
  send_bt_at_command("CONN0", true);
}


void switch_state(String state) {
  if (state == "on" ) {
    digitalWrite(LED, LOW);
  }
  else if (state == "off" ) {
    digitalWrite(LED, HIGH);
  }
//  Serial.println(!digitalRead(LED));
}

void handle_switch() {
  // get the value of request argument "state" and convert it to an int
  switch_state(server.arg("state"));
  handle_state();
  confirmed = false;
}

void handle_state()
{
  if (!digitalRead(LED)){
    server.send(200, "text/plain", String("{\"state\": \"on\"}"));    
  }
  else {
    server.send(200, "text/plain", String("{\"state\": \"off\"}"));        
  }  
}

void handle_confirmed()
{
  server.send(200, "text/plain", String("{\"state\": \"")  + String(confirmed) + String("\"}"));    
} 

void handle_connection()
{
  if (!digitalRead(LED)){
    server.send(200, "text/plain", String("{\"state\": \"on\"}"));    
  }
  else {
    server.send(200, "text/plain", String("{\"state\": \"off\"}"));        
  }  
}

void handle_index() {
  String form = "<form action='switch'><input type='radio' name='state' value='on' checked>On<input type='radio' name='state' value='off'>Off<input type='submit' value='Submit'></form>";
  server.send(200, "text/html", form);
}

void setup() {
  
  // Reset Bluetoooth
  pinMode(BTBRK, OUTPUT);
  reset_connection();
  pinMode(BTCONN, INPUT);

  // Serial set-up
  BTserial.begin(9600);
  Serial.begin(9600);
  
  // Reset BlueTooth to default 
  reset_bluetooth();

  delay(500);
  BTserial.println("C1");
  delay(500);
  BTserial.println("C0");
  delay(500);
  BTserial.println("C1");
  delay(500);
  BTserial.println("C0");


  // Hostname does not actually work
  WiFi.hostname(DEVICENAME);
  // Connect to WiFi network
  WiFi.begin(MYSSID, PASSWORD);  
  // Wait for connection
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//  }
  delay(500);

  // Set up the endpoints for HTTP server
  server.on("/", handle_index);
  server.on("/switch", handle_switch);
  server.on("/state", handle_state);
  server.on("/confirmed", handle_confirmed);  
  server.on("/connection", handle_connection);
  // Start the server 
  server.begin();  
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //  inputString = "";

  // Keep sending out request confirms....
  server.handleClient();
  if (!confirmed) {
    BTserial.println("C" + String(digitalRead(LED)));
    delay(200);
  }

  // If Connection borken
  if (digitalRead(BTCONN) == LOW) {
    reset_bluetooth();
  }

  while (BTserial.available() > 0) {
    char inChar = (char)BTserial.read();    
    if (inChar == '\n') {
      if(inputString[0] == 'R'){
        // We may need to check back reply bit are equal to send bits...
        confirmed = true;
      }
      else if(inputString[0] == 'C'){
        if(inputString[1] == '1'){
          switch_state("on");
        }
        else if(inputString[1] == '0'){
          switch_state("off");
        }
        BTserial.println("R");
      }
      else {
        Serial.println("Something else:");
      }
      for (int i = 0; i < inputString.length(); i++) {
        Serial.println(inputString[i]);
      }
      inputString = "";
    }
    else{
      inputString += inChar;
    }
  }


  if (Serial.available())
  {
    c =  Serial.read();
    BTserial.write(c);  
  }
}
