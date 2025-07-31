const uint8_t SHOW_CMD[24] = {0xc9, 0x14, 0x02, 0x53, 0x48, 0x4f, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0x00, 0xaa, 0x04, 0x64, 0x00, 0x74, 0xc8, 0x0d};

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 200; // 200ms

// Sử dụng chân 8 cho LED ngoài
const int LED_PIN = 8;

void setup() {
  Serial.begin(9600);
  
  // Không cần chờ Serial nếu không dùng debug
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Gửi lệnh SHOW đầu tiên
  Serial.write(SHOW_CMD, 24);
  lastSendTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    Serial.write(SHOW_CMD, 24);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastSendTime = currentTime;
  }
  
  // Xóa bộ đệm nhận
  while (Serial.available()) Serial.read();
}