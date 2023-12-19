#include <TimeLib.h>

// #define BLYNK_TEMPLATE_ID "TMPL66_J9cZh2"
// #define BLYNK_TEMPLATE_NAME "Micro"
// #define BLYNK_AUTH_TOKEN "TdTDGaeJnvE4dScXHptzlyKiqfnzesuN"

#define BLYNK_TEMPLATE_ID "TMPL6a4_UgTbW"
#define BLYNK_TEMPLATE_NAME "Door Lock"
#define BLYNK_AUTH_TOKEN "wrGRRtRw2rwrBtPfejHjs3fn_L9H2KZi"

#ifndef STASSID
#define STASSID "J_AGUNG BAKAR PERDANA"
#define STAPSK "akugaktau"
#endif

#define SS_PIN D10
#define RST_PIN D9
#define BLYNK_PRINT Serial

#define buzzer D2
#define relay D8
#define LCD_SDA_PIN D14
#define LCD_SCL_PIN D15
#define TOMBOL_KELUAR_PIN D0

unsigned long startTime;
unsigned long updateInterval = 1000;  // 1 detik


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include "certs.h"
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "Wire.h"
#include "I2CKeyPad.h"

WidgetLCD blynkLCD(V4);

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);
boolean wifiConnected = false;
unsigned long wifiStartTime;

const uint8_t KEYPAD_ADDRESS = 0x20;
I2CKeyPad keyPad(KEYPAD_ADDRESS);
char keymap[19] = "123A456B789C*0#DNF";

char* correctPin = "1234";


const char* ssid = STASSID;
const char* password = STAPSK;

X509List cert(cert_DigiCert_Global_Root_CA);

void connectWiFi() {
  setupLcd();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.print("Menghubungkan");
    delay(500);
    lcd.clear();

    //offline mode emergency
    if (millis() - wifiStartTime >= 30000) {
      Serial.println("\nBeralih ke offline mode.");
      lcd.clear();
      lcd.print("Offline Mode");
      lcd.clear();
      lcd.print("Masukkan PIN:");
      usingPin();
      wifiConnected = false;
      return;
    }
  }
  wifiConnected = true;
  lcd.print("Berhasil");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  Serial.println("");
  Serial.println("WiFi connected");

  delay(1000);
  lcd.setCursor(0, 0);
  lcd.clear();
}

void synchronizeTime() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
}


String getDataFromURL() {
  WiFiClientSecure client;
  client.flush();
  client.setTrustAnchors(&cert);

  if (!client.connect(github_host, github_port)) {
    Serial.println("Connection failed");
    return "";
  }

  String url = "/rizpedia/api-micro/main/data.json";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + github_host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Cache-Control: no-cache\r\n" + "Pragma: no-cache\r\n" + "Connection: close\r\n\r\n");

  bool headersFound = false;
  bool isJsonStarted = false;
  String line;
  String responseData = "";
  while (client.connected() || client.available()) {
    line = client.readStringUntil('\n');
    if (!headersFound && line == "\r") {
      headersFound = true;
    } else if (headersFound && !isJsonStarted && line.startsWith("[")) {
      isJsonStarted = true;
      responseData += line + "\n";
    } else if (isJsonStarted) {
      responseData += line + "\n";
    }
  }

  client.flush();
  client.stop();
  return responseData;
}

void setupLcd() {
  lcd.init();
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.backlight();
}

void setupPinMode() {
  pinMode(relay, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(TOMBOL_KELUAR_PIN, INPUT_PULLUP);
}

String getUID() {
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content;
}

void waktuMilis() {
  if (millis() - startTime >= updateInterval) {
    startTime = millis();
  }
}

void setupKeyPad() {
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  if (keyPad.begin() == false) {
    Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
    while (1)
      ;
  }
  keyPad.loadKeyMap(keymap);
}

bool verifyPin(const char* enteredPin, const char* correctPin) {
  return strcmp(enteredPin, correctPin) == 0;
}

//login dnegan pin
void usingPin() {
  char enteredPin[5];
  int pinIndex = 0;

  lcd.clear();
  lcd.print("Masukkan PIN:");

  while (true) {
    if (keyPad.isPressed()) {
      char Input = keyPad.getChar();
      tone(buzzer, HIGH);
      delay(100);
      tone(buzzer, LOW);
      if (Input == 'D') {
        lcd.clear();
        lcd.print("Masukkan PIN:");
        pinIndex = 0;
        memset(enteredPin, 0, sizeof(enteredPin));
      } else if (Input == '#') {
        enteredPin[pinIndex] = '\0';
        if (verifyPin(enteredPin, correctPin)) {
          lcd.clear();
          lcd.print("PIN Benar");
          pintuBuka();
        } else {
          lcd.clear();
          lcd.print("PIN Salah");
          delay(2000);
          lcd.clear();
          lcd.print("Masukkan PIN:");
        }
        break;
      } else {
        if (pinIndex < 4) {
          enteredPin[pinIndex] = Input;
          lcd.setCursor(pinIndex, 1);
          lcd.print('*');
          pinIndex++;
        }
      }
    }
    delay(200);
  }
}

void setup() {
  Serial.begin(19200);
  setupKeyPad();
  connectWiFi();
  Blynk.begin(BLYNK_AUTH_TOKEN, STASSID, STAPSK, "iot.amikom.ac.id", 8080);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println();
  synchronizeTime();
  setupPinMode();
  lcd.print("Tempelkan Kartu");
}

void pintuBuka() {
  lcd.clear();
  int i;
  digitalWrite(relay, HIGH);
  tone(buzzer, HIGH);
  // tone(buzzer, 3000);
  delay(500);
  noTone(buzzer);
  lcd.print("Pintu Terbuka");
  //Blynk.virtualWrite(V4, "Pintu Terbuka");
  blynkLCD.print(0,0, "Pintu Terbuka");
  lcd.setCursor(0, 1);
  for (i = 0; i < 12; i++) {
    lcd.print(".");
    blynkLCD.print(i,1, ".");
    delay(800);
  }
  digitalWrite(relay, LOW);
}

void getCard() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      String dataUID_Rfid = getUID().substring(1);
      String dataUID_API = getDataFromURL();

      Serial.print("UID terdeteksi : ");
      Serial.println(dataUID_Rfid);
      Serial.print("UID dari API : ");
      Serial.println(dataUID_API);

      const size_t capacity = JSON_ARRAY_SIZE(4) + 4 * JSON_OBJECT_SIZE(4) + 90;
      DynamicJsonDocument doc(capacity);
      DeserializationError error = deserializeJson(doc, dataUID_API);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

      for (JsonVariant obj : doc.as<JsonArray>()) {
        const char* uid = obj["UID"];
        const char* name = obj["Name"];
        const char* status = obj["status"];

        Blynk.virtualWrite(V0, name);
        Blynk.logEvent("history", String("Ada yang masuk mencoba masuk") + name);

        if (dataUID_Rfid == uid) {
          Serial.print("Nama : ");
          Serial.println(name);
          Serial.print("Uid : ");
          Serial.println(uid);
          Serial.print("Status : ");
          Serial.println(status);
          Serial.println("");
          if (strcmp(status, "allow") == 0) {
            lcd.clear();
            lcd.setCursor(0, 0);
            Blynk.virtualWrite(V1, name);
            Blynk.virtualWrite(V5, year() + month());
            Blynk.logEvent("detect_orang_masuk", String("Orang Yang Masuk: ") + name);
            // lcd.print(name);
            pintuBuka();
            lcd.clear();
            return;
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("No akses");
            delay(2000);
            lcd.clear();
            return;
          }
        }
      }
    }
  }
}


void loop() {
  Blynk.run();
  lcd.clear();
  blynkLCD.clear();
  lcd.print("Tempelkan kartu");
  blynkLCD.print(0,0, "Tempelkan Kartu");

  getCard();

  if (keyPad.isPressed()) {
    char Input = keyPad.getChar();
    if (Input == 'D') {
      usingPin();  // Panggil fungsi untuk memproses PIN
    }
  }

  if(digitalRead(D8) == 1){
    pintuBuka();
  }


  if (digitalRead(TOMBOL_KELUAR_PIN) == LOW) {
    lcd.clear();
    pintuBuka();
  }

  delay(500);
}
