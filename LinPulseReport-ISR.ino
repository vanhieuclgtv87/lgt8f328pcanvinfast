const unsigned int SIF_PIN = 3;

short rpm = 0;
bool regen = false;
bool brake = false;
byte driveModeBits = 0;
String driveModeName = "";

unsigned long lastTime;
unsigned long lastDuration = 0;
byte lastCrc = 0;
byte data[12];
int bitIndex = -1;

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
  // ISR handles all the processing.
}

void sifChange()
{
  int val = digitalRead(SIF_PIN);
  unsigned long duration = micros() - lastTime;
  lastTime = micros();

  // Print pulse details for every change
  Serial.print("Pin state: ");
  Serial.print(val == HIGH ? "HIGH" : "LOW");
  Serial.print(", Duration (us): ");
  Serial.print(duration);
  Serial.print(", Last Duration (us): ");
  Serial.print(lastDuration);

  if (val == LOW)
  {
    if (lastDuration > 0)
    {
      bool bitComplete = false;
      float ratio = 0;
      if (duration > 0) {
        ratio = float(lastDuration) / float(duration);
      }
      
      // Print the calculated ratio
      Serial.print(", Ratio: ");
      Serial.print(ratio, 2);

      // Sync frame
      if (round(ratio) >= 31)
      {
        bitIndex = 0;
        Serial.print(" -> SYNC FRAME");
      }
      else if (ratio > 1.5)
      {
        // bit 0
        bitClear(data[bitIndex / 8], 7 - (bitIndex % 8));
        bitComplete = true;
        Serial.print(" -> BIT 0");
      }
      else if ((1 / ratio) > 1.5)
      {
        // bit 1
        bitSet(data[bitIndex / 8], 7 - (bitIndex % 8));
        bitComplete = true;
        Serial.print(" -> BIT 1");
      }

      if (bitComplete)
      {
        bitIndex++;
        if (bitIndex == 96) // 12 bytes
        {
          bitIndex = 0;
          byte crc = 0;

          for (int i = 0; i < 11; i++)
          {
            crc ^= data[i];
          }

          if (crc == data[11] && crc != lastCrc)
          {
            lastCrc = crc;

            // Raw data for debug
            Serial.println("\n--- VALID DATA PACKET ---");
            Serial.print("Raw: ");
            for (int i = 0; i < 12; i++)
            {
              Serial.print(data[i], HEX);
              Serial.print(" ");
            }
            Serial.println();

            // Decode data from byte 4
            byte statusByte = data[4];
            brake = bitRead(statusByte, 5);
            regen = bitRead(statusByte, 3);
            bool reverse = bitRead(data[5], 2);

            driveModeBits = statusByte & 0b00000111;
            switch (driveModeBits) {
              case 0: driveModeName = "THE THAO"; break;
              case 1: driveModeName = "NORMAL"; break;
              case 2: driveModeName = "TIET KIEM"; break;
              default: driveModeName = "UNKNOWN"; break;
            }

            // RPM from byte 7-8
            rpm = ((data[7] << 8) + data[8]);

            // Print results
            Serial.print("Decoded: RPM: "); Serial.print(rpm);
            Serial.print("  Mode: "); Serial.print(driveModeName);
            if (brake) Serial.print("  BRAKE");
            if (regen) Serial.print("  REGEN");
            if (reverse) Serial.print("  SO LUI");
            Serial.println("\n-------------------------");
          }
        }
      }
    }
    // Add a new line after the LOW pulse for better readability
    Serial.println();
  }
  
  lastDuration = duration;
}
