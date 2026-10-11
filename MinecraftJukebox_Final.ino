#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// RFID pins
#define RFID_SS   5
#define RFID_RST  22

// SD card
#define SD_CS     13

// MAX98357A (Amplifier) pins
#define I2S_BCLK  26
#define I2S_LRC   25
#define I2S_DOUT  27

MFRC522 rfid(RFID_SS, RFID_RST);

AudioGeneratorWAV *wav = nullptr;
AudioFileSourceSD *file = nullptr;
AudioOutputI2S *out = nullptr;

String currentUID = "";

void stopSong() {
  if (wav != nullptr) {
    if (wav->isRunning()) {
      wav->stop();
    }
    delete wav;
    wav = nullptr;
  }

  if (file != nullptr) {
    delete file;
    file = nullptr;
  }

  currentUID = "";
}

void playSong(const char *path, String uid) {
  stopSong();

  Serial.print("Opening: ");
  Serial.println(path);

  file = new AudioFileSourceSD(path);

  if (!file->isOpen()) {
    Serial.println("ERROR: Could not open song.");
    delete file;
    file = nullptr;
    return;
  }

  wav = new AudioGeneratorWAV();

  if (wav->begin(file, out)) {
    currentUID = uid;
    Serial.println("Playing song!");
  } else {
    Serial.println("ERROR: Audio playback could not start.");
    stopSong();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Minecraft RFID Jukebox");

  SPI.begin(18, 19, 23);

  // Initializing SD
  pinMode(RFID_SS, OUTPUT);
  digitalWrite(RFID_SS, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("ERROR: SD card initialization failed!");
    while (true) delay(1000);
  }

  Serial.println("SD card initialized.");

  // Initialize RFID reader
  rfid.PCD_Init();
  delay(50);
  digitalWrite(SD_CS, HIGH);

  Serial.println("RFID reader initialized.");
  Serial.println("Scan a disc!");

  // Initialize amplifier
out = new AudioOutputI2S();
out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
out->SetGain(0.8);
out->SetOutputModeMono(true);
}

void loop() {
  // Keeps audio playing while waiting for RFID scans
  if (wav != nullptr && wav->isRunning()) {
    if (!wav->loop()) {
      Serial.println("Song finished.");
      stopSong();
    }
  }

  // Check for a new RFID chip
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();

  Serial.print("Scanned UID: ");
  Serial.println(uid);

  // Stop reading the chip
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Deselect RFID before accessing SD
  digitalWrite(RFID_SS, HIGH);

// RFID Chip ID + Song
if (uid == "69ED6720") {
  Serial.println("Disc: Don't Mine at Night");
  playSong("/Songs/dontMineAtNight.wav", uid);
}
else if (uid == "1A741856") {
  Serial.println("Disc: Poker Face");
  playSong("/Songs/PokerFace.wav", uid);
}
else {
  Serial.println("Unknown disc.");
}

  // Deselect SD after accessing it
  digitalWrite(SD_CS, HIGH);
}
