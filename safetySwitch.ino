#include <SoftwareSerial.h>
SoftwareSerial NFCserial(3, 2);  //RX, TX

void show_serial_data(bool print = true);

const int lock_pin = 6;
const int buzzer_pin = 9;

uint8_t received_buf_pos = 0;
uint8_t response_byte;
uint8_t data_len;
boolean received_complete = false;
String tag_id = "";
char Byte_In_Hex[3];

uint8_t echo_command[1] = { 0x55 };

uint8_t initialise_cmd_iso14443_1[6] = { 0x09, 0x04, 0x68, 0x01, 0x07, 0x10 };
uint8_t initialise_cmd_iso14443_2[6] = { 0x09, 0x04, 0x68, 0x01, 0x07, 0x00 };
uint8_t initialise_cmd_iso14443_3[6] = { 0x02, 0x04, 0x02, 0x00, 0x02, 0x80 };
uint8_t initialise_cmd_iso14443_4[6] = { 0x09, 0x04, 0x3A, 0x00, 0x58, 0x04 };
uint8_t initialise_cmd_iso14443_5[6] = { 0x09, 0x04, 0x68, 0x01, 0x01, 0xD3 };

uint8_t detect_cmd_iso14443_1[4] = { 0x04, 0x02, 0x26, 0x07 };
uint8_t detect_cmd_iso14443_2[5] = { 0x04, 0x03, 0x93, 0x20, 0x08 };

struct tag {
  String Card;
  int Access;
};

tag Tags[2] = {
  { "41945364", 1 },
  { "884523d", 1 }
};

int Tag_Status;

unsigned long StartTime = 0;
unsigned long RunTime = 72000000;

void setup() {
  Serial.begin(9600);
  NFCserial.begin(57600);
  delay(1000);

  pinMode(lock_pin, OUTPUT);

  //Serial.println("Echo command: ");
  //NFCserial.write(echo_command, 1);
  //delay(1000);
  //show_serial_data();
  //delay(1000);
  Serial.println("Initialise commands: ");
  NFCserial.write(initialise_cmd_iso14443_1, 6);
  delay(1000);
  show_serial_data();
  NFCserial.write(initialise_cmd_iso14443_2, 6);
  delay(1000);
  show_serial_data();
  NFCserial.write(initialise_cmd_iso14443_3, 6);
  delay(1000);
  show_serial_data();
  NFCserial.write(initialise_cmd_iso14443_4, 6);
  delay(1000);
  show_serial_data();
  NFCserial.write(initialise_cmd_iso14443_5, 6);
  delay(1000);
  show_serial_data();
  tone(buzzer_pin, 500, 300);
}

void show_serial_data(bool print = true) {
  while (NFCserial.available() != 0) {
    if (print)
      Serial.print(NFCserial.read(), HEX);
    else
      NFCserial.read();
  }
  if (print)
    Serial.println("");
}

void Read_Tag() {
  uint8_t received_char;
  while (NFCserial.available() != 0) {
    received_char = char(NFCserial.read());

    if (received_buf_pos == 0) response_byte = received_char;
    else if (received_buf_pos == 1) data_len = received_char;
    else if (received_buf_pos >= 2 and received_buf_pos < 6) {
      sprintf(Byte_In_Hex, "%x", received_char);
      tag_id += Byte_In_Hex;  //adding to a string
    }
    received_buf_pos++;
    if (received_buf_pos >= data_len) {
      received_complete = true;
    }
  }
}

void loop() {
  received_buf_pos = 0;
  received_complete = false;
  tag_id = "";
  response_byte = 0;

  //Serial.println("Searching new card...");
  NFCserial.write(detect_cmd_iso14443_1, 4);
  delay(500);
  //Serial.println("Response:");
  show_serial_data(false);

  NFCserial.write(detect_cmd_iso14443_2, 5);
  delay(200);

  if (NFCserial.available()) {
    Read_Tag();
  }

  if (response_byte == 0x80 && tag_id != "") {
    Tag_Status = 2;
    for (int i = 0; i < sizeof(Tags) / sizeof(tag); i++) {
      if (Tags[i].Card == tag_id) Tag_Status = Tags[i].Access;
    }
    if (Tag_Status == 1) {
      if (digitalRead(lock_pin) == HIGH) {
        //switch is already on
        if (StartTime != 0 && millis() - StartTime > RunTime - 30000) {
          //reset start time
          StartTime = millis();
        } else {
          digitalWrite(lock_pin, LOW);
          StartTime = 0;
        }
      } else {
        digitalWrite(lock_pin, HIGH);
        StartTime = millis();
      }
      tone(buzzer_pin, 1000, 500);
      delay(1000);
    } else {
      digitalWrite(lock_pin, LOW);
      StartTime = 0;
      tone(buzzer_pin, 500, 200);
      delay(200);
      tone(buzzer_pin, 500, 200);
    }
    Serial.print("Tag detected. Tag id: ");
    Serial.println(tag_id);
    Serial.println("");
    delay(100);
  }

  if (StartTime != 0 && millis() - StartTime > RunTime - 30000) {
    tone(buzzer_pin, 1000, 200);
    delay(200);
    tone(buzzer_pin, 1000, 200);
    delay(200);
    tone(buzzer_pin, 1000, 200);
  }

  if (StartTime != 0 && millis() - StartTime > RunTime) {
    //time to turn off switch
    Serial.println("Timed out.");
    Serial.print("Run time: ");
    Serial.println(RunTime);
    digitalWrite(lock_pin, LOW);
    StartTime = 0;
  }

  delay(10);
}