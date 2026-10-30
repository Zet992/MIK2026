# Протокол команд библиотеки MAX7357

**Версия:** 1.0
**Устройство:** MAX7357 — 8-канальный I²C-мультиплексор
**Адрес по умолчанию:** `0x77`

---

## 1. Общая модель

- Все публичные методы возвращают `MAX7357Status`.
- Ошибки не бросаются как исключения — обрабатывайте по возвращаемому коду.
- Перед любой командой должен быть вызван `begin()`; иначе `NotBegun`.

### 1.1. Перечисление статусов

| Значение | Код | Причина |
|----------|-----|---------|
| `Ok` | 0 | Успех |
| `NotBegun` | 1 | `begin()` не вызывался |
| `BadChannel` | 2 | `channel > 7` |
| `I2cWriteError` | 3 | `endTransmission() != 0` |
| `I2cReadError` | 4 | Прочитано байт меньше запрошенного |
| `NullBuffer` | 5 | Передан `nullptr` |

### 1.2. Модель состояния

| Поле | Тип | Значение по умолчанию |
|------|-----|-----------------------|
| `_channels` | `uint8_t` | `0x00` |
| `_configuration` | `uint8_t` | `0x01` |
| `_flushOutSequence` | `uint8_t` | `0xFF` |
| `_isBasicMode` | `bool` | `false` |

---

## 2. Инициализация

### `MAX7357(uint8_t address = 0x77)`
Конструктор. Только сохраняет адрес.

### `MAX7357Status begin(TwoWire &inWire = Wire)`
Привязывает шину, вызывает `Wire.begin()`. Устанавливает `_begun = true`.
**Возврат:** `Ok`.

---

## 3. Управление каналами

| Метод | Аргумент | Изменение `_channels` | I²C |
|-------|----------|-----------------------|-----|
| `openChannel(ch)` | 0..7 | `|= (1<<ch)` | 1 байт |
| `closeChannel(ch)` | 0..7 | `&= ~(1<<ch)` | 1 байт |
| `selectChannel(ch)` | 0..7 | `= (1<<ch)` | 1 байт |
| `openAll()` | — | `= 0xFF` | 1 байт |
| `closeAll()` | — | `= 0x00` | 1 байт |
| `writeChannels(v)` | 0..255 | `= v` | 1 байт |

**Возврат:** `Ok` / `BadChannel` / `I2cWriteError` / `NotBegun`.

### Пример

```cpp
mux.closeAll();
mux.openChannel(0);
mux.openChannel(2);   // каналы 0 и 2 открыты
mux.closeChannel(0);  // закрыт 0, открыт только 2
mux.selectChannel(5); // открыт только 5
```

---

## 4. Регистры конфигурации

### `MAX7357Status writeConfiguration(uint8_t value)`
Устанавливает `_configuration`, пересчитывает `_isBasicMode`.
**I²C-транзакция:** 3 байта — `_channels`, `_configuration`, `_flushOutSequence`.

### `MAX7357Status writeFlushOutSequence(uint8_t value)`
Устанавливает `_flushOutSequence`.
**I²C-транзакция:** 3 байта — `_channels`, `_configuration`, `_flushOutSequence`.

> **Важно:** обе команды шлют все три регистра подряд (автоинкремент MAX7357), поэтому состояние на устройстве всегда консистентно.

### Биты `_configuration`

| Бит | Имя | Описание |
|-----|-----|----------|
| `0x02` | `ENABLE_FLUSH_OUT` | Включить flush-out |
| `0x08` | `FREEZE_LOCK_UP_REGISTER` | Не сбрасывать lock-up до чтения |
| `0x10` | `DISCONNECT_ONLY_LOCKED_UP_BUS` | Отключать только залоченную шину |
| `0x20` | `DISABLE_LOCK_UP_DETECTION` | Отключить детект lock-up |
| `0x40` | `ENABLE_BASIC_MODE` | Только переключение каналов |
| `0x80` | `ENABLE_PRECONNECT_TEST` | Preconnect wiggle test |

---

## 5. Чтение

### `MAX7357Status read(uint8_t &out)`
Читает 1 байт с адреса устройства.
**Возврат:** `Ok` (значение в `out`) / `I2cReadError` / `NotBegun`.

### `MAX7357Status getRegisters(MAX7357Registers &out)`
Читает 7 регистров одной транзакцией.

```cpp
struct MAX7357Registers {
    uint8_t switchControl;    // [0]
    uint8_t configuration;    // [1]
    uint8_t flushOutSequence; // [2]
    uint8_t lockUpIndication; // [3]
    uint8_t trafficHigh;      // [4]
    uint8_t trafficLow;       // [5]
    uint8_t stuckHighFault;   // [6]
};
```

### `MAX7357Status readRegisters()`
Читает `getRegisters()` и печатает в `Serial`. Возвращает статус.

### Пример

```cpp
MAX7357Registers r;
if (mux.getRegisters(r) == MAX7357Status::Ok) {
    Serial.println(r.switchControl);
}

uint8_t v;
if (mux.read(v) == MAX7357Status::Ok) {
    Serial.println(v);
}
```

---

## 6. Геттеры

| Метод | Возврат |
|-------|---------|
| `channels()` | `uint8_t` |
| `configuration()` | `uint8_t` |
| `flushOutSequence()` | `uint8_t` |
| `isBasicMode()` | `bool` |
| `address()` | `uint8_t` |

---

## 7. Каскадирование — класс `MAX7357Cascade`

### 7.1. Схема адресации

Старшие 4 бита адреса фиксированы (`1110`), младшие 3 бита задаются выводами A2–A0 :

| A2 | A1 | A0 | Адрес (hex) |
|----|----|----|-------------|
| GND | GND | GND | `0x70` |
| GND | GND | VDD | `0x71` |
| GND | VDD | GND | `0x72` |
| GND | VDD | VDD | `0x73` |
| VDD | GND | GND | `0x74` |
| VDD | GND | VDD | `0x75` |
| VDD | VDD | GND | `0x76` |
| VDD | VDD | VDD | `0x77` |

**Правила:**
1. Каждый чип на одной шине должен иметь **уникальный** адрес `0x70..0x77`.
2. Максимум **8 чипов** на одну шину I²C.
3. Выводы A2–A0 нельзя оставлять «в воздухе» — подтягивайте к GND или VDD.

### 7.2. Модель состояния каскада

```cpp
class MAX7357Cascade {
    std::vector<MAX7357> _muxes;   // экземпляры по индексам 0..N-1
    TwoWire *_wire = nullptr;
    bool _begun = false;
};
```

- Индекс `muxIndex` — **позиция в `_muxes`**, а не I²C-адрес.
- `addMux(addr)` возвращает индекс; адрес `0x70..0x77`.

### 7.3. Команды каскада

| Метод | Аргументы | Возврат | Описание |
|-------|-----------|---------|----------|
| `addMux(addr)` | `uint8_t addr` = `0x70..0x77` | `int8_t` индекс или `-1` | Добавить чип в каскад. `-1` при неверном адресе или `>= 8` чипов. |
| `begin(wire)` | `TwoWire&` | `Status` | Инициализировать все чипы на общей шине. |
| `closeAll()` | — | `Status` | `closeAll()` на **каждом** чипе. |
| `selectPath(muxIndex, ch)` | `uint8_t`, `0..7` | `Status` | На `muxIndex` — `selectChannel(ch)`, на остальных — `closeAll()`. |
| `openChannelOn(muxIndex, ch)` | `uint8_t`, `0..7` | `Status` | `openChannel(ch)` только на `muxIndex`, остальные не трогает. |
| `closeChannelOn(muxIndex, ch)` | `uint8_t`, `0..7` | `Status` | `closeChannel(ch)` только на `muxIndex`. |
| `count()` | — | `size_t` | Число чипов в каскаде. |
| `at(index)` | `size_t` | `MAX7357*` / `nullptr` | Доступ к чипу по индексу. |

### 7.4. Семантика `selectPath` vs `openChannelOn`

| Метод | Что делает | Когда использовать |
|-------|------------|--------------------|
| `selectPath(i, ch)` | Гарантирует **ровно один** открытый канал во всём каскаде: на `i` открывает `ch`, на всех остальных — `closeAll()`. | Маршрутизация к одному устройству, изоляция шин. |
| `openChannelOn(i, ch)` | Открывает канал **только** на `i`, не трогая остальные чипы. | Нужно несколько одновременно открытых каналов на разных чипах (например, параллельный опрос). |

### 7.5. Коды ошибок каскада

| Ситуация | Возврат |
|----------|---------|
| `addMux` с адресом вне `0x70..0x77` | `-1` |
| `addMux` при `count() >= 8` | `-1` |
| Метод каскада до `begin()` | `NotBegun` |
| `muxIndex >= count()` | `BadChannel` |
| `channel > 7` | `BadChannel` |
| Ошибка I²C на любом чипе | `I2cWriteError` / `I2cReadError` (возвращается последняя) |

> **Примечание:** методы каскада, затрагивающие несколько чипов (`begin`, `closeAll`, `selectPath`), продолжают выполнение при ошибке на одном чипе и возвращают **последний** ненулевой статус. Если нужна строгая остановка — обрабатывайте статус после каждого шага вручную.

### 7.6. Пример: плоский каскад из 3 чипов

```cpp
#include <Wire.h>
#include "MAX7357Cascade.h"

MAX7357Cascade cascade;

void setup() {
    Serial.begin(115200);

    int8_t m0 = cascade.addMux(0x70);   // A2=A1=A0=GND
    int8_t m1 = cascade.addMux(0x71);   // A0=VDD
    int8_t m2 = cascade.addMux(0x72);   // A1=VDD

    if (m0 < 0 || m1 < 0 || m2 < 0) {
        Serial.println("addMux failed");
        return;
    }

    if (cascade.begin(Wire) != MAX7357Status::Ok) {
        Serial.println("begin failed");
        return;
    }

    // Ровно один открытый канал: mux1 / ch5
    cascade.selectPath(1, 5);

    // Обращение к устройству на канале 5 мультиплексора 0x71
    // по его обычному адресу.
}

void loop() {}
```

### 7.7. Пример: несколько открытых каналов

```cpp
// Открыть ch0 на mux0 и ch3 на mux2, не закрывая друг друга
cascade.openChannelOn(0, 0);
cascade.openChannelOn(2, 3);

// Закрыть только ch0 на mux0
cascade.closeChannelOn(0, 0);
```

### 7.8. Ограничения каскада

1. **Уникальность адресов.** Два чипа с одинаковым адресом → конфликт на шине, `begin()` вернёт `Ok`, но поведение непредсказуемо.
2. **Максимум 8 чипов** на одну шину (`0x70..0x77`).
3. **`selectPath` закрывает все остальные чипы.** Для древовидного каскада (выход одного MAX7357 → вход другого) нужна отдельная логика `selectRoute` (см. §7.9).
4. **Одновременное открытие одинаковых адресов на разных чипах** допустимо (шины изолированы), но обращаться к ним нужно поочерёдно, закрывая «лишние» каналы.

### 7.9. Древовидный каскад (mux → mux)

Если выход одного MAX7357 подключён к входу другого, используйте `selectRoute`:

```cpp
struct MuxHop {
    uint8_t muxIndex;
    uint8_t channel;
};

// Цепочка: mux0/ch2 -> mux1/ch5
MAX7357Status selectRoute(const MuxHop* hops, uint8_t count)
{
    closeAll();
    for (uint8_t i = 0; i < count; ++i) {
        MAX7357Status st = at(hops[i].muxIndex)
                              ->selectChannel(hops[i].channel);
        if (st != MAX7357Status::Ok) return st;
    }
    return MAX7357Status::Ok;
}

// Использование:
MuxHop route[] = { {0, 2}, {1, 5} };
selectRoute(route, 2);
```

---

## 8. Сводная таблица (одиночный чип + каскад)

| Метод | Класс | Аргументы | Возврат | I²C-байт |
|-------|-------|-----------|---------|----------|
| `begin` | `MAX7357` | `TwoWire&` | `Status` | 0 |
| `openChannel` | `MAX7357` | `ch` | `Status` | 1 |
| `closeChannel` | `MAX7357` | `ch` | `Status` | 1 |
| `selectChannel` | `MAX7357` | `ch` | `Status` | 1 |
| `openAll` | `MAX7357` | — | `Status` | 1 |
| `closeAll` | `MAX7357` | — | `Status` | 1 |
| `writeChannels` | `MAX7357` | `v` | `Status` | 1 |
| `writeConfiguration` | `MAX7357` | `v` | `Status` | 3 |
| `writeFlushOutSequence` | `MAX7357` | `v` | `Status` | 3 |
| `read` | `MAX7357` | `uint8_t&` | `Status` | 1 (чтение) |
| `getRegisters` | `MAX7357` | `Registers&` | `Status` | 7 (чтение) |
| `readRegisters` | `MAX7357` | — | `Status` | 7 + Serial |
| `addMux` | `Cascade` | `addr` | `int8_t` | 0 |
| `begin` | `Cascade` | `TwoWire&` | `Status` | 0 |
| `closeAll` | `Cascade` | — | `Status` | N×1 |
| `selectPath` | `Cascade` | `muxIndex, ch` | `Status` | N×1 |
| `openChannelOn` | `Cascade` | `muxIndex, ch` | `Status` | 1 |
| `closeChannelOn` | `Cascade` | `muxIndex, ch` | `Status` | 1 |
| `count` | `Cascade` | — | `size_t` | 0 |
| `at` | `Cascade` | `index` | `MAX7357*` | 0 |

---

*Документ соответствует реализации v1.0.