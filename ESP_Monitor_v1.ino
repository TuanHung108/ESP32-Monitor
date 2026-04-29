#define LED_PIN 2   // GPIO 2 (LED built-in trên nhiều board ESP32)

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // bật LED
  delay(1000);                 // đợi 1 giây
  digitalWrite(LED_PIN, LOW);  // tắt LED
  delay(1000);                 // đợi 1 giây
}