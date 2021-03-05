#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <esp_https_ota.h>
#include <esp_task_wdt.h>

const size_t strLen = 32;
const int SSIDPos = 0;
const int passwordPos = 32;
char SSID[strLen];
char password[strLen];

void interactiveConfig();
void ask(char *bytes, const char *msg, const char *currentValue);
void writeStr(char *data, int offset);
void readStr(int pos, char *data, size_t len);
esp_err_t doUpdate();

extern const uint8_t s3CA_start[] asm("_binary_src_s3_ca_pem_start");
extern const uint8_t s3CA_end[] asm("_binary_src_s3_ca_pem_end");

const char *IMAGE_URL = "https://vega-ota.s3.eu-north-1.amazonaws.com/vega-n2k-node1/firmware-latest.bin";


void setup() {
  Serial.begin(115200);
  Serial.println("Configuring Wifi credentials & OTA in EEPROM.");

  EEPROM.begin(512);

  interactiveConfig();

  Serial.print("Connecting WiFi");

  WiFi.begin(SSID, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.print("\nConnected, IP address: "); Serial.println(WiFi.localIP());

  Serial.println("Starting OTA upgrade");
  if (doUpdate() != ESP_OK) {
    Serial.println("Update failed, please reboot and try again.");
  }
}

esp_err_t doUpdate() {
  esp_http_client_config_t http_conf = {
    .url = "https://vega-ota.s3.eu-north-1.amazonaws.com/vega-n2k-node1/firmware-latest.bin",
    .host = NULL,
    .port = 443,
    .username = NULL,
    .password = NULL,
    .auth_type = HTTP_AUTH_TYPE_NONE,
    .path = NULL,
    .query = NULL,
    .cert_pem = (char *)s3CA_start,
  };
  esp_https_ota_config_t config = { .http_config = &http_conf };

  esp_https_ota_handle_t ota_handle = 0;
  esp_err_t err = esp_https_ota_begin(&config, &ota_handle);
  if (err != ESP_OK) {
    Serial.println("OTA begin failed");
    return err;
  } else {
    Serial.println("OTA download started");
  }

  esp_task_wdt_init(15,0);
  while (1) {
    err = esp_https_ota_perform(ota_handle);
    if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      break;
    }
    Serial.printf("Image bytes read: %d\n", esp_https_ota_get_image_len_read(ota_handle));
  }
  esp_task_wdt_init(5,0);

  if (esp_https_ota_is_complete_data_received(ota_handle) != true) {
    Serial.println("Complete data was not received.");
    return ESP_FAIL;
  } else {
    err = esp_https_ota_finish(ota_handle);
    if (err == ESP_OK) {
      Serial.println("ESP_HTTPS_OTA upgrade successful. Rebooting...");
      delay(1000 / portTICK_PERIOD_MS);
      esp_restart();
    } else {
      Serial.printf("ESP_HTTPS_OTA upgrade failed 0x%x\n", err);
      return err;
    }
  }
}

void interactiveConfig() {
  char SSID_C[strLen];
  readStr(SSIDPos, SSID_C, strLen);
  ask(SSID, "WiFi SSID", SSID_C);

  char PWD_C[strLen];
  readStr(passwordPos, PWD_C, strLen);
  ask(password, "WiFi password", PWD_C);

  writeStr(SSID, SSIDPos);
  writeStr(password, passwordPos);
}

void ask(char *bytes, const char *msg, const char *currentValue) {
  boolean waitingForInput = true;
  byte ndx = 0;
  char rc;

  Serial.printf("%s [%s]: ", msg, currentValue);

  while (waitingForInput) {
    while (Serial.available() > 0) {
      rc = Serial.read();
      if (rc != '\n') {
        bytes[ndx++] = rc;
        if (ndx >= strLen) ndx = strLen - 1;
      } else {
        if (ndx == 0) {
          strncpy(bytes, currentValue, strLen);
        } else {
          bytes[ndx] = '\0';
        }
        waitingForInput = false;
      }
    }
  }
  Serial.println();
}

void writeStr(char *data, int offset) {
  int i=0;
  for (; i<strLen; i++) {
    EEPROM.write(offset+i, data[i]);
  }
  //EEPROM.write(offset+i, '\0');
  EEPROM.commit();
}

void readStr(int pos, char *data, const size_t len) {
  for (int i=0; i<len; i++) data[i] = EEPROM.read(pos+i);
}

void loop() {}