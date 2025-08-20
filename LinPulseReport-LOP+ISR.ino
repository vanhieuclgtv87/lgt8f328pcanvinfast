const unsigned int SIF_PIN = 3;

// Decoded data variables
volatile short rpm = 0;
volatile bool regen = false;
volatile bool brake = false;
volatile byte driveModeBits = 0;
String driveModeName = "";
volatile bool newDataReady = false;

// ISR variables
volatile unsigned long lastTime;
volatile unsigned long lastDuration = 0;
volatile byte lastCrc = 0;
volatile byte data[12];
volatile int bitIndex = -1;

void setup()
{
  Serial.begin(115200);
  Serial.println("Begin.");

  pinMode(SIF_PIN, INPUT);
  lastTime = micros();
  attachInterrupt(digitalPinToInterrupt(SIF_PIN), sifChange, CHANGE);
}

void loop()
{
  // Check a flag instead of doing work inside the ISR
  if (newDataReady) {
    // Safely copy data and reset flag
    noInterrupts();
    byte localData[12];
    memcpy(localData, (const byte*)data, 12);
    newDataReady = false;
    interrupts();

    // Perform all the heavy processing outside the ISR
    byte crc = 0;
    for (int i = 0; i < 11; i++) {
      crc ^= localData[i];
    }

    if (crc == localData[11] && crc != lastCrc) {
      lastCrc = crc;

      // In raw data để debug
      Serial.print("Raw: ");
      for (int i = 0; i < 12; i++) {
        Serial.print(localData[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Giải mã từ byte 4
      byte statusByte = localData[4];
      brake = bitRead(statusByte, 5);
      regen = bitRead(statusByte, 3);
      bool reverse = bitRead(localData[5], 2);

      // 3 bit thấp = chế độ lái
      driveModeBits = statusByte & 0b00000111;
      switch (driveModeBits) {
        case 0: driveModeName = "THE THAO"; break;
        case 1: driveModeName = "NORMAL"; break;
        case 2: driveModeName = "TIET KIEM"; break;
        default: driveModeName = "UNKNOWN"; break;
      }

      // RPM từ byte 7-8
      rpm = ((localData[7] << 8) + localData[8]);

      // In kết quả
      Serial.print("RPM: "); Serial.print(rpm);
      Serial.print("  Mode: "); Serial.print(driveModeName);
      if (brake) Serial.print("  BRAKE");
      if (regen) Serial.print("  REGEN");
      if (reverse) Serial.print("  SO LUI");
      Serial.println();
    }
  }
}

void sifChange()
{
  int val = digitalRead(SIF_PIN);
  unsigned long duration = micros() - lastTime;
  lastTime = micros();

  if (val == LOW) {
    if (lastDuration > 0) {
      bool bitComplete = false;
      unsigned long ratio100 = (lastDuration * 100) / duration;

      // Sync frame
      if (ratio100 >= 3100) {
        bitIndex = 0;
      }
      // Bit 0: ratio > 1.5 -> lastDuration > 1.5 * duration -> lastDuration * 100 > 150 * duration
      else if (ratio100 > 150) {
        bitClear(data[bitIndex / 8], 7 - (bitIndex % 8));
        bitComplete = true;
      }
      // Bit 1: (1 / ratio) > 1.5 -> duration > 1.5 * lastDuration -> duration * 100 > 150 * lastDuration
      else if ((duration * 100) > (lastDuration * 150)) {
        bitSet(data[bitIndex / 8], 7 - (bitIndex % 8));
        bitComplete = true;
      }

      if (bitComplete) {
        bitIndex++;
        if (bitIndex == 96) {
          bitIndex = 0;
          newDataReady = true;
        }
      }
    }
  }
  lastDuration = duration;
}
