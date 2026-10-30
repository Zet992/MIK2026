#ifndef MAX7357_H
#define MAX7357_H

#include <Arduino.h>
#include <Wire.h>
#include <array>
#include <stdint.h>

/* Channel bit masks (Switch Control register) */
#define MAX_CHANNEL_0 0x01
#define MAX_CHANNEL_1 0x02
#define MAX_CHANNEL_2 0x04
#define MAX_CHANNEL_3 0x08
#define MAX_CHANNEL_4 0x10
#define MAX_CHANNEL_5 0x20
#define MAX_CHANNEL_6 0x40
#define MAX_CHANNEL_7 0x80

/* Configuration register bits */
#define ENABLE_FLUSH_OUT                 0x02
#define FREEZE_LOCK_UP_REGISTER          0x08
#define DISCONNECT_ONLY_LOCKED_UP_BUS    0x10
#define DISABLE_LOCK_UP_DETECTION        0x20
#define ENABLE_BASIC_MODE                0x40
#define ENABLE_PRECONNECT_TEST           0x80

#define MAX7357_DEFAULT_ADDRESS 0x77
#define MAX7357_REGISTER_COUNT  7
#define MAX7357_MAX_CHANNEL     7

/**
 * Статус выполнения команды.
 * Все публичные методы возвращают одно из этих значений.
 */
enum class MAX7357Status : uint8_t {
    Ok = 0,
    NotBegun,        // begin() не вызывался
    BadChannel,      // channel > MAX7357_MAX_CHANNEL
    I2cWriteError,   // endTransmission() != 0
    I2cReadError,    // прочитано байт != запрошено
    NullBuffer       // передан nullptr в out-параметр
};

/** Карта регистров, возвращаемая getRegisters(). */
struct MAX7357Registers {
    uint8_t switchControl;       // [0]
    uint8_t configuration;       // [1]
    uint8_t flushOutSequence;    // [2]
    uint8_t lockUpIndication;    // [3]
    uint8_t trafficHigh;         // [4]
    uint8_t trafficLow;          // [5]
    uint8_t stuckHighFault;      // [6]
};

class MAX7357
{
public:
    explicit MAX7357(uint8_t address = MAX7357_DEFAULT_ADDRESS);

    MAX7357Status begin(TwoWire &inWire = Wire);

    /* Управление каналами */
    MAX7357Status openChannel(uint8_t channel);
    MAX7357Status closeChannel(uint8_t channel);
    MAX7357Status selectChannel(uint8_t channel);
    MAX7357Status openAll();
    MAX7357Status closeAll();
    MAX7357Status writeChannels(uint8_t value);

    /* Регистры конфигурации */
    MAX7357Status writeConfiguration(uint8_t value);
    MAX7357Status writeFlushOutSequence(uint8_t value);

    /* Чтение */
    MAX7357Status read(uint8_t &out);
    MAX7357Status getRegisters(MAX7357Registers &out);
    MAX7357Status readRegisters();   // читает и печатает в Serial

    /* Геттеры состояния */
    uint8_t channels()         const { return _channels; }
    uint8_t configuration()    const { return _configuration; }
    uint8_t flushOutSequence() const { return _flushOutSequence; }
    bool    isBasicMode()      const { return _isBasicMode; }
    uint8_t address()          const { return _address; }

private:
    TwoWire *_wire = nullptr;
    uint8_t  _address;
    uint8_t  _channels         = 0x00;
    uint8_t  _configuration    = 0x01;
    uint8_t  _flushOutSequence = 0xFF;
    bool     _isBasicMode      = false;
    bool     _begun            = false;

    MAX7357Status writeByte(uint8_t data);
    MAX7357Status writeRegisters3(uint8_t a, uint8_t b, uint8_t c);
    bool ensureBegun() const;
};

const char *MAX7357StatusToString(MAX7357Status s);

#endif // MAX7357_H