#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>

const size_t strLen = 33;
const int SSIDPos = 0;
const int passwordPos = 32;
char SSID[strLen];
char password[strLen];

void printCurrent();
void ask(char *bytes, const char *msg);
void writeStr(char *data, int pos);
void readStr(int pos, char *data, size_t len);


void setup() {
  Serial.begin(115200);
  Serial.println("Configuring Wifi credentials & OTA in EEPROM.");

  EEPROM.begin(512);

  printCurrent();

  ask(SSID, "WiFi SSID: ");
  ask(password, "WiFi password: ");

  writeStr(SSID, SSIDPos);
  writeStr(password, passwordPos);

  Serial.print("Configuration complete, connecting WiFi");

  WiFi.begin(SSID, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("Connected, IP address: "); Serial.println(WiFi.localIP());

}

void printCurrent() {
  Serial.println("Current configuration:");

  char SSID_OLD[strLen];
  readStr(SSIDPos, SSID_OLD, strLen);
  Serial.print("  WiFi SSID: "); Serial.println(SSID_OLD);

  char password_old[strLen];
  readStr(passwordPos, password_old, strLen);
  Serial.print("  WiFi password: "); Serial.println(password_old);

  Serial.println();
}

void ask(char *bytes, const char *msg) {
  boolean waitingForInput = true;
  byte ndx = 0;
  char rc;

  Serial.print(msg);

  while (waitingForInput) {
    while (Serial.available() > 0) {
      rc = Serial.read();
      if (rc != '\n' && rc != '\r') {
        bytes[ndx++] = rc;
        if (ndx >= strLen) ndx = strLen - 1;
      } else {
        bytes[ndx] = '\0';
        waitingForInput = false;
      }
    }
  }
  Serial.println();
}

void writeStr(char *data, int pos) {
  int i=0;
  for (; i<=strLen; i++) {
    EEPROM.write(pos+i, data[i]);
  }
  EEPROM.write(pos+i, '\0');
  EEPROM.commit();
}

void readStr(int pos, char *data, const size_t len) {
  for (int i=0; i<len; i++) data[i] = EEPROM.read(pos+i);
}

void loop() {}