#include "MAX7357.h"

#ifndef MAX7357_DEBUG
#define MAX7357_DEBUG 0
#endif

#if MAX7357_DEBUG
  #define DBG(...)   Serial.print(__VA_ARGS__)
  #define DBGLN(...) Serial.println(__VA_ARGS__)
#else
  #define DBG(...)   do {} while (0)
  #define DBGLN(...) do {} while (0)
#endif

/* ---------- helpers ---------- */

const char *MAX7357StatusToString(MAX7357Status s)
{
    switch (s) {
        case MAX7357Status::Ok:            return "Ok";
        case MAX7357Status::NotBegun:      return "NotBegun";
        case MAX7357Status::BadChannel:    return "BadChannel";
        case MAX7357Status::I2cWriteError: return "I2cWriteError";
        case MAX7357Status::I2cReadError:  return "I2cReadError";
        case MAX7357Status::NullBuffer:    return "NullBuffer";
    }
    return "Unknown";
}

/* ---------- ctor / init ---------- */

MAX7357::MAX7357(uint8_t address) : _address(address) {}

MAX7357Status MAX7357::begin(TwoWire &inWire)
{
    _wire  = &inWire;
    _wire->begin();
    _begun = true;
    return MAX7357Status::Ok;
}

bool MAX7357::ensureBegun() const
{
#if MAX7357_DEBUG
    if (!_begun) { Serial.println("MAX7357: begin() not called"); }
#endif
    return _begun && _wire != nullptr;
}

/* ---------- low-level I2C ---------- */

MAX7357Status MAX7357::writeByte(uint8_t data)
{
    if (!ensureBegun()) return MAX7357Status::NotBegun;

    DBG("W addr=0x"); DBG(_address, HEX); DBG(" data=0x"); DBGLN(data, HEX);

    _wire->beginTransmission(_address);
    _wire->write(data);
    uint8_t err = _wire->endTransmission(true);
    return err == 0 ? MAX7357Status::Ok : MAX7357Status::I2cWriteError;
}

MAX7357Status MAX7357::writeRegisters3(uint8_t a, uint8_t b, uint8_t c)
{
    if (!ensureBegun()) return MAX7357Status::NotBegun;

    DBG("W3 addr=0x"); DBG(_address, HEX);
    DBG(" "); DBG(a, HEX); DBG(" "); DBG(b, HEX); DBG(" "); DBGLN(c, HEX);

    _wire->beginTransmission(_address);
    _wire->write(a);
    _wire->write(b);
    _wire->write(c);
    uint8_t err = _wire->endTransmission(true);
    return err == 0 ? MAX7357Status::Ok : MAX7357Status::I2cWriteError;
}

/* ---------- channel control ---------- */

MAX7357Status MAX7357::openChannel(uint8_t channel)
{
    if (channel > MAX7357_MAX_CHANNEL) return MAX7357Status::BadChannel;
    _channels |= static_cast<uint8_t>(1u << channel);
    DBGLN("openChannel");
    return writeByte(_channels);
}

MAX7357Status MAX7357::closeChannel(uint8_t channel)
{
    if (channel > MAX7357_MAX_CHANNEL) return MAX7357Status::BadChannel;
    _channels &= static_cast<uint8_t>(~(1u << channel));
    DBGLN("closeChannel");
    return writeByte(_channels);
}

MAX7357Status MAX7357::selectChannel(uint8_t channel)
{
    if (channel > MAX7357_MAX_CHANNEL) return MAX7357Status::BadChannel;
    _channels = static_cast<uint8_t>(1u << channel);
    DBGLN("selectChannel");
    return writeByte(_channels);
}

MAX7357Status MAX7357::openAll()
{
    _channels = 0xFF;
    return writeByte(_channels);
}

MAX7357Status MAX7357::closeAll()
{
    _channels = 0x00;
    return writeByte(_channels);
}

MAX7357Status MAX7357::writeChannels(uint8_t value)
{
    _channels = value;
    return writeByte(_channels);
}

/* ---------- configuration registers ---------- */

MAX7357Status MAX7357::writeConfiguration(uint8_t value)
{
    _configuration = value;
    _isBasicMode   = (_configuration & ENABLE_BASIC_MODE) != 0;
    return writeRegisters3(_channels, _configuration, _flushOutSequence);
}

MAX7357Status MAX7357::writeFlushOutSequence(uint8_t value)
{
    _flushOutSequence = value;
    return writeRegisters3(_channels, _configuration, _flushOutSequence);
}

/* ---------- read ---------- */

MAX7357Status MAX7357::read(uint8_t &out)
{
    if (!ensureBegun()) return MAX7357Status::NotBegun;

    uint8_t got = _wire->requestFrom(static_cast<uint16_t>(_address),
                                     static_cast<uint8_t>(1));
    if (got != 1 || !_wire->available()) {
        return MAX7357Status::I2cReadError;
    }
    out = _wire->read();
    DBG("R addr=0x"); DBG(_address, HEX); DBG(" data=0x"); DBGLN(out, HEX);
    return MAX7357Status::Ok;
}

MAX7357Status MAX7357::getRegisters(MAX7357Registers &out)
{
    if (!ensureBegun()) return MAX7357Status::NotBegun;

    uint8_t got = _wire->requestFrom(static_cast<uint16_t>(_address),
                                     static_cast<uint8_t>(MAX7357_REGISTER_COUNT));
    if (got != MAX7357_REGISTER_COUNT) {
        return MAX7357Status::I2cReadError;
    }

    uint8_t buf[MAX7357_REGISTER_COUNT];
    _wire->readBytes(buf, MAX7357_REGISTER_COUNT);

    out.switchControl    = buf[0];
    out.configuration    = buf[1];
    out.flushOutSequence = buf[2];
    out.lockUpIndication = buf[3];
    out.trafficHigh      = buf[4];
    out.trafficLow       = buf[5];
    out.stuckHighFault   = buf[6];
    return MAX7357Status::Ok;
}

MAX7357Status MAX7357::readRegisters()
{
    MAX7357Registers r;
    MAX7357Status st = getRegisters(r);
    if (st != MAX7357Status::Ok) {
        Serial.print("READ REGISTERS ERROR: ");
        Serial.println(MAX7357StatusToString(st));
        return st;
    }

    Serial.print("Switch Control: ");           Serial.println(r.switchControl);
    Serial.print("Configuration: ");            Serial.println(r.configuration);
    Serial.print("Flush-Out Sequence: ");       Serial.println(r.flushOutSequence);
    Serial.print("Lock-Up Indication: ");       Serial.println(r.lockUpIndication);
    Serial.print("Traffic Prior (H): ");        Serial.println(r.trafficHigh);
    Serial.print("Traffic Prior (L): ");        Serial.println(r.trafficLow);
    Serial.print("Stuck High Fault: ");         Serial.println(r.stuckHighFault);
    return MAX7357Status::Ok;
}