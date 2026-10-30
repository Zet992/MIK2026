#include <Wire.h>
#include "src/MAX7357.h"
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 4
#define SCL_PIN 5

MAX7357 I2CMux(0x77); // Иниализация коммутатора с адресом 0x77
LiquidCrystal_I2C lcd(0x27, 16, 2);
float cm;


void display_text(int string, float cm, int channel) {
  lcd.setCursor(0, string); // Столбец 0, строка 0
  String text = "S1(Ch" + String(channel) + "): " + String(cm, 1) + "cm";
  while (text.length() < 16) {
    text += " ";
  }
  lcd.print(text);
}


float get_distance() {
  Wire.beginTransmission(0x57);
  Wire.write(0x01);
  Wire.endTransmission(true);
  delay(200);
  Wire.beginTransmission(0x57);
  Wire.write(0xAF); // Запрос на результат измерения
  Wire.endTransmission(false);
  uint8_t res = Wire.requestFrom(0x57, 3);
  if (res != 3) {
      // обработка ошибки
      // данные не получены в желаемом количестве
      return -1;
  } else {
      // кол-во данных в буфере
      uint32_t H = Wire.read();
      uint32_t M = Wire.read();
      uint32_t L = Wire.read();
      uint32_t micrometers = (H << 16) | (M << 8) | L;
      float cm = micrometers / 10000.0f;  // convert µm → cm
      return cm;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  I2CMux.begin(Wire);
  pinMode(34, OUTPUT);

  lcd.init();         // Инициализация дисплея
  lcd.backlight();    // Включение подсветки
  I2CMux.openChannel(7);

  I2CMux.readRegisters(); // Вывод регистров через Serial
  Serial.println();
  I2CMux.openAll();
}

void loop() {
  I2CMux.readRegisters();
//  
//  I2CMux.openChannel(6);
//  cm = get_distance();
//  Serial.print("Расстояние на шестом канале: ");
//  if (cm == -1) {
//      Serial.println("ОШИБКА: res != N");
//  } else {
//      Serial.print(cm, 2);
//      Serial.println(" cm");
//      display_text(0, cm, 6);
//  }
//  I2CMux.closeChannel(6);
//  
//  I2CMux.openChannel(3);
//  cm = get_distance();
//  Serial.print("Расстояние на третьем канале: ");
//  if (cm == -1) {
//      Serial.println("ОШИБКА: res != N");
//  } else {
//      Serial.print(cm, 2);
//      Serial.println(" cm");
//      display_text(1, cm, 3);
//  }
//  I2CMux.closeChannel(3);
//  Serial.println();
}
