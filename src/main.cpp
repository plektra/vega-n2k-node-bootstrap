#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_https_ota.h>
#include <esp_task_wdt.h>

const size_t strLen = 32;
const int SSIDPos = 0;
const int passwordPos = 32;
const int nodePos = 64;
const int versionPos = 96;
char SSID[strLen];
char password[strLen];
char node[strLen]; // e.g. vega-n2k-node1
char version[16]; // semver

const char *imageUrlFormat = "https://vega-ota.s3.eu-north-1.amazonaws.com/%s/%s";
char imageUrl[128];

void interactiveConfig();
void connectWiFi();
void getLatestVersion();
void ask(char *bytes, size_t len, const char *msg, const char *currentValue);
void writeStr(char *data, int offset);
void readStr(int pos, char *data, size_t len);
esp_err_t doUpdate();

extern const uint8_t s3CA_start[] asm("_binary_src_s3_ca_pem_start");
extern const uint8_t s3CA_end[] asm("_binary_src_s3_ca_pem_end");


void setup() {
  Serial.begin(115200);
  Serial.println("Configuring Wifi credentials & OTA in EEPROM.");

  EEPROM.begin(512);

  interactiveConfig();

  connectWiFi();

  getLatestVersion();
  writeStr(version, versionPos);

  char imageName[32];
  sprintf(imageName, "firmware-%s.bin", version);
  sprintf(imageUrl, imageUrlFormat, node, imageName);

  if (doUpdate() != ESP_OK) {
    Serial.println("Update failed, please reboot and try again.");
  }
}

void connectWiFi() {
  Serial.print("Connecting WiFi");

  WiFi.begin(SSID, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }

  Serial.print("\nConnected, IP address: "); Serial.println(WiFi.localIP());
}

void getLatestVersion() {
  char versionUrl[128];
  sprintf(versionUrl, imageUrlFormat, node, "latest.txt");

  Serial.printf("Fetching latest version number from %s\n", versionUrl);

  HTTPClient https;

  if (https.begin(versionUrl, (char *)s3CA_start)) {
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
      String ver = https.getString();
      ver.trim();
      ver.toCharArray(version, 16);
      Serial.printf("Got version %s\n", version);
    } else {
      Serial.printf("Version fetch failed, HTTP code %i\n", httpCode);
    }
  }
}

esp_err_t doUpdate() {
  Serial.println("Starting OTA upgrade");

  esp_http_client_config_t http_conf = {
    .url = imageUrl,
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
  ask(SSID, strLen, "WiFi SSID", SSID_C);

  char PWD_C[strLen];
  readStr(passwordPos, PWD_C, strLen);
  ask(password, strLen, "WiFi password", PWD_C);

  char NODE_C[strLen];
  readStr(nodePos, NODE_C, strLen);
  ask(node, strLen, "Node project name", NODE_C);

  writeStr(SSID, SSIDPos);
  writeStr(password, passwordPos);
  writeStr(node, nodePos);
}

void ask(char *bytes, size_t len, const char *msg, const char *currentValue) {
  boolean waitingForInput = true;
  byte ndx = 0;
  char rc;

  Serial.printf("%s [%s]: ", msg, currentValue);

  while (waitingForInput) {
    while (Serial.available() > 0) {
      rc = Serial.read();
      if (rc != '\n') {
        bytes[ndx++] = rc;
        if (ndx >= len) ndx = len - 1;
      } else {
        if (ndx == 0) {
          strncpy(bytes, currentValue, len);
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
  for (; i<strlen(data); i++) {
    EEPROM.write(offset+i, data[i]);
  }
  EEPROM.write(offset+i, '\0');
  EEPROM.commit();
}

void readStr(int pos, char *data, const size_t len) {
  for (int i=0; i<len; i++) data[i] = EEPROM.read(pos+i);
}

void loop() {}