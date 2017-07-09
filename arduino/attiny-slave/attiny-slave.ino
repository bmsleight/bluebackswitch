// Using a Nano in testing - with migrate to ATTiny

#include <SoftwareSerial.h>
#define LED 13            // Nano PIN13
#define BTPOWERSINK1 11   // Nano Sink pins 20mA per pin 
#define BTPOWERSINK2 12   // Nano Sink pins 20mA per pin
#define BTBRK 9         // BlueTooth Pin to break connection
#define DEVICENAME "BigBlueS123"

String inputString = ""; // a string to hold incoming data
bool confirmed = true;
char c = ' ';
int loop_count = 0;

SoftwareSerial BTserial(2, 3); // RX | TX


void switch_state(String state) {
  if (state == "on" ) {
    digitalWrite(LED, HIGH);
  }
  else if (state == "off" ) {
    digitalWrite(LED, LOW);
  }
}

void switch_bt(String state) {
  if (state == "on" ) {
    digitalWrite(BTPOWERSINK1, LOW);
    digitalWrite(BTPOWERSINK2, LOW);
  }
  else {
    digitalWrite(BTPOWERSINK1, HIGH);
    digitalWrite(BTPOWERSINK2, HIGH);
  }
}


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


void setup() {
  pinMode(LED, OUTPUT);
  switch_state("off");

  // Tunr on Bluetoooth
  pinMode(BTPOWERSINK1, OUTPUT);
  pinMode(BTPOWERSINK1, OUTPUT);
  pinMode(BTBRK, OUTPUT);
  digitalWrite(BTBRK, HIGH);
  switch_bt("on");

  // Serial set-up
  Serial.begin(9600);
  BTserial.begin(9600);
  delay(5000);
  // Renew, set to sleep on disconnect
  send_bt_at_command("RENEW", true);  
  send_bt_at_command("RENEW", true);
  send_bt_at_command("PWRM0", true);

}

void loop() {  
  if (!confirmed) {
    BTserial.println("C" + String(!digitalRead(LED)));
    delay(200);
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
      for (int i = 0; i < inputString.length(); i++) {
        Serial.println(inputString[i]);
      }
      inputString = "";
    }
    else {
      inputString += inChar;
      Serial.write(inChar);
      }
  }

  if (Serial.available())
  {
    c =  Serial.read();
    BTserial.write(c);  
  }
}
