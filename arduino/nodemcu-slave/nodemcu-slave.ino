#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "local_passwords.h"
// Peter Lerup wonderful espsoftwareserial
#include <SoftwareSerial.h>

#define BTBRK 4         // BlueTooth Pin to break connection
#define DEVICENAME "BigBlueN"     // "BigBlueN"
#define LED 2           // NodeMCU LED

ESP8266WebServer server(80);
SoftwareSerial BTserial(14, 12, false, 256);

int loop_count = 0;
String inputString = ""; // a string to hold incoming data
bool confirmed = true;   // All commands have been confirmed
bool bt_connected = false;   // BlueTooth Connected or not

void logging(String log1, String log2="", String log3="")  {
  String full_log = "Log: " + log1 + " " + log2 + " " + log3;
  Serial.println(full_log);
}

String send_bt_at_command(String command, bool reply_to_serial = false) {
  String full_command = "AT+" + command;
  String full_reply = "";
  int simple_time = 0;
  char c = ' ';
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
    logging(full_reply);
  }
  return full_reply;
}

void btc() {
  logging("Reconnect");

  // To Wake
  //  - Either send 80 chars AND AT+RENEW
  //  - or Long press system KEY >=1000 ms
  
  // Wake by send 80 chars to serial
//  for (int i=0; i <= 81; i++) {
//    BTserial.write('l');
//  }
//  BTserial.write('AT');

  // Long press system KEY >=1000 ms
  btc_disconnect();
  
  // #BTserial.write('AT');
  // Renew may not be needed.
//  send_bt_at_command("RENEW", true);
//  send_bt_at_command("COSU1", true);
//  send_bt_at_command("IMME1", true);
  send_bt_at_command("ROLE1", true);
//  delay(1000);
  send_bt_at_command("CON5CF821880465", true);
  logging("Connected ?");
}

void btc_disconnect() {
  logging("Disconnect ?");
  digitalWrite(BTBRK, LOW);    
  delay(1100);
  digitalWrite(BTBRK, HIGH);
  logging("Disconnect .,.");
}

void bt_disconnect_back_to_sleep() {
  btc_disconnect();
  send_bt_at_command("RENEW", true);
  send_bt_at_command("COSU1", true);
  send_bt_at_command("PWRM0", true);
  logging("Sleep");
}

void switch_state(String state) {
  logging("switch_state: ", state);
  if (state == "on" ) {
    digitalWrite(LED, LOW);
  }
  else if (state == "off" ) {
    digitalWrite(LED, HIGH);
  }
}

void handle_index() {
  String form = "<form action='switch'><input type='radio' name='state'\
   value='on' checked>On<input type='radio' name='state' \
   value='off'>Off<input type='submit' value='Submit'></form>";
  server.send(200, "text/html", form);
}

void handle_state()
{
  if (digitalRead(LED) == LOW) {
    server.send(200, "text/plain", String("{\"state\": \"on\"}"));    
  }
  else {
    server.send(200, "text/plain", String("{\"state\": \"off\"}"));        
  }  
}

void handle_switch() {
  // get the value of request argument "state" and convert it to an int
  switch_state(server.arg("state"));
  handle_state();
  confirmed = false;
}

void setup_server()  {
  logging("setup_server");
  server.on("/", handle_index);
  server.on("/switch", handle_switch);
  server.on("/state", handle_state);
  server.begin();
}

void setup_wifi()  {
  logging("setup_wifi");
  // Hostname does not actually work
  WiFi.hostname(DEVICENAME);
  // Connect to WiFi network
  WiFi.begin(MYSSID, PASSWORD);  
  // Wait does not work on nodemcu3
  // Wait for connection
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//  }
  delay(500);
  logging("WiFi.localIP()", WiFi.localIP().toString());
}


void setup() {
  Serial.begin(9600);
  logging("setup");

  pinMode(BTBRK, OUTPUT);
  btc_disconnect();
  
  BTserial.begin(9600);
  delay(4000);
  bt_disconnect_back_to_sleep();
  logging("\nBT to sleep");  

  pinMode(LED, OUTPUT);
  switch_state("on");
  delay(500);
  switch_state("off");
  delay(500);
  switch_state("on");
  delay(500);
  
  setup_wifi();
  setup_server();
}

void loop() {
  server.handleClient();
 
  if (!confirmed) {
    if (!bt_connected)  {
      btc();
      // More logic ... btc() return true
      bt_connected = true;
    }
    if (digitalRead(LED) == LOW)  {
        BTserial.println("C1");      
    }
    else  {
        BTserial.println("C0");
    }
    delay(200);
  }


  while (BTserial.available() > 0) {
    char inChar = (char)BTserial.read();    
    if (inChar == '\n') {
      if(inputString[0] == 'R'){
        // We may need to check back reply bit are equal to send bits...
        // Only get a [R]eply when we are Master - hence disconnect
        confirmed = true;
        bt_disconnect_back_to_sleep();
        bt_connected = false;
      }
      else if(inputString[0] == 'C'){
        if(inputString[1] == '1'){
          switch_state("on");          
        }
        else if(inputString[1] == '0'){
          switch_state("off");
        }
        // I been sent something, 
        // hence the other end is master and connected
        // Just bee to reply as slave
        // and master will disconnect
        BTserial.println("R");
      }
      else {
        logging("Something else:");
      }
      logging("inputString", inputString);
    }
    else{
      inputString += inChar;
      Serial.write(inChar);
    }
  }

  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '#') {
      switch_state("on");
    }
    else if (inChar == ']') {
      switch_state("off");
    }   
    confirmed = false;
    bt_connected = false;
  }
}
