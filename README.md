# MAX7357

Данная библиотека написана для интегральной схемы I2C-коммутатора MAX7357 под ESP32.

### Команды
---
Список команд есть в протоколе команд.

### Установка
---
Перенесите папку src в папку с вашим кодом.

### Использование
---
Вся работа с коммутатором ведётся через экземпляр класа MAX7357. Для каскадирования
использовать библиотеку "MAX7357Cascade.h"

### Пример

```c++
#include <Wire.h>
#include "src/MAX7357.h"

#define SDA_PIN 21
#define SCL_PIN 22

MAX7357 I2CMux(0x77); // Иниализация коммутатора с адресом 0x77

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  I2CMux.begin(Wire);

  I2CMux.readRegisters(); // Вывод регистров через Serial
  Serial.println();

  I2CMux.openChannel(2); // Открытие второго канала

  // Работа с устройством на канале 2
  Wire.beginTransmission(0x57);
  Wire.write(0x01);
  Wire.endTransmission(true);

  I2CMux.closeChannel(2); // Закрытие второго канала

  // Задание настроек по умолчанию (такие же настройки при включении устройства)
  I2CMux.writeChannels(0); // Запись значения в регистр состояния каналов
  I2CMux.writeConfiguration(1); // Запись значения в регистр настроек
  I2CMux.writeFlushOutSequence(0xFF); // Запись значения в регистр 
  I2CMux.readRegisters();
  Serial.println();

  // Пример изменения настроек
  // Доступные опции можно посмотреть в MAX7357.h
  uint8_t config = (ENABLE_FLUSH_OUT | FREEZE_LOCK_UP_REGISTER | DISCONNECT_ONLY_LOCKED_UP_BUS);
  I2CMux.writeConfiguration(config);
  I2CMux.readRegisters();
  Serial.println();
}

void loop() {

}
```