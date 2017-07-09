
#include <SoftwareSerial.h>

#define BTBRK 4         // BlueTooth Pin to break connection
#define DEVICENAME "BigBlueN"     // "BigBlueN"
#define LED 2           // NodeMCU LED


SoftwareSerial BTserial(14, 12, false, 256);
int loop_count = 0;
String inputString = ""; // a string to hold incoming data
bool confirmed = true;   // All commands have been confirmed
bool bt_connected = false;   // All commands have been confirmed

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
    for (int i = 0; i < full_reply.length(); i++)
    {
      Serial.write(full_reply[i]);   // Push each char 1 by 1 on each loop pass
    }
  }
  return full_reply;
}

void btc() {
  Serial.println("\nReconnect");

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
  Serial.println("\n :: ");
}

void btc_disconnect() {
  digitalWrite(BTBRK, LOW);    
  Serial.println("..");
  delay(1100);
  digitalWrite(BTBRK, HIGH);

}

void bt_disconnect_back_to_sleep() {
  Serial.println("Disconnect");
  btc_disconnect();
  send_bt_at_command("RENEW", true);
  send_bt_at_command("COSU1", true);
  send_bt_at_command("PWRM0", true);
}

void switch_state(String state) {
  Serial.println(state);
  if (state == "on" ) {
    digitalWrite(LED, LOW);
  }
  else if (state == "off" ) {
    digitalWrite(LED, HIGH);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(BTBRK, OUTPUT);
  btc_disconnect();
  
  BTserial.begin(9600);
  delay(4000);
  bt_disconnect_back_to_sleep();
  Serial.println("\nSoftware serial started");  

  pinMode(LED, OUTPUT);
  switch_state("on");
  delay(500);
  switch_state("off");
  delay(500);
  switch_state("on");
  delay(500);
  
  
}

void loop() {
  
  if (!confirmed) {
    if (!bt_connected)  {
      btc();
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
        Serial.println("Something else:");
      }
      for (int i = 0; i < inputString.length(); i++) {
        Serial.println(inputString[i]);
      }
      inputString = "";
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
//    BTserial.write(Serial.read());
  }
}
