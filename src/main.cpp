#include <Arduino.h>
#include <EEPROM.h>

const size_t strLen = 32;
const int SSIDPos = 0;
const int passwordPos = 32;
char SSID[strLen];
char password[strLen];
boolean waitingForInput;

void ask(char *bytes, const char* msg);
void writeStr(char *data, int pos);
void readStr(int pos, char *data, size_t len);


void setup() {
  Serial.begin(115200);
  Serial.println("Configuring Wifi credentials & OTA in EEPROM.");

  EEPROM.begin(2*strLen);

  char SSID_OLD[strLen];
  readStr(SSIDPos, SSID_OLD, strLen);
  Serial.print("Current SSID: "); Serial.println(SSID_OLD);

  char password_old[strLen];
  readStr(passwordPos, password_old, strLen);
  Serial.print("Current password: "); Serial.println(password_old);

  Serial.println();
  ask(SSID, "WiFi SSID: ");
  ask(password, "WiFi password: ");

  writeStr(SSID, SSIDPos);
  writeStr(password, passwordPos);

  // Serial.print("Your SSID is "); Serial.println(SSID);
  // Serial.print("Your password is "); Serial.println(password);
  //Serial.print("Your password is "); Serial.println(password);

  Serial.println("Bootstrap complete.");
}

void ask(char *bytes, const char* msg) {
  waitingForInput = true;
  static byte ndx;
  char rc;

  Serial.print(msg);
  while (waitingForInput) {
    while (Serial.available() > 0) {
      rc = Serial.read();
      if (rc != '\n') {
        bytes[ndx++] = rc;
        if (ndx >= strLen) ndx = strLen - 1;
      } else {
        bytes[ndx] = '\0';
        ndx = 0;
        waitingForInput = false;
      }
    }
  }
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
  //char *flash = (char*) malloc(len+1);
  

  for (int i=0; i<len; i++) {
    data[i] = EEPROM.read(pos+i);
  }
  //*data = flash;
}

void loop() {}