#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "local_passwords.h"

  
// HTTP server will listen at port 80
ESP8266WebServer server(80);

#define LED 2            // Led in NodeMCU at pin GPIO16 (D0).
String inputString = ""; // a string to hold incoming data
bool confirmed = true;

void setup(void) {
  Serial.begin(9600);
  Serial.println("AT+CONN1");
  inputString.reserve(200);
  pinMode(LED, OUTPUT);
  
  // Connect to WiFi network
  WiFi.begin(MYSSID, PASSWORD);  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
//  WiFi.hostname("switchABC");
  
  // Set up the endpoints for HTTP server
  server.on("/", handle_index);
  server.on("/switch", handle_switch);
  server.on("/state", handle_state);
  server.on("/confirmed", handle_confirmed);
  
  // Start the server 
  server.begin();  
}


void handle_switch() {
  // get the value of request argument "state" and convert it to an int
  switch_state(server.arg("state"));
  handle_state();
  confirmed = false;
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

void handle_index() {
  String form = "<form action='switch'><input type='radio' name='state' value='on' checked>On<input type='radio' name='state' value='off'>Off<input type='submit' value='Submit'></form>";
  server.send(200, "text/html", form);
}
 
void loop(void) {
  // check for incomming client connections frequently in the main loop:
  server.handleClient();
  if (!confirmed) {
    Serial.println("C" + String(!digitalRead(LED)));
    delay(200);
  }

  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();    
    if (inChar == '\n') {
      Serial.println(inputString[0]);
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
        Serial.println("R" + !digitalRead(LED));
        switch_state(inputString);
      }
      inputString = "";
    }
    else{
      inputString += inChar;
    }
  }
}

