#include "MAX7357Cascade.h"

#ifndef MAX7357_CASCADE_DEBUG
#define MAX7357_CASCADE_DEBUG 0
#endif

int8_t MAX7357Cascade::addMux(uint8_t address)
{
    if (address < 0x70 || address > 0x77) {
        return -1;   // недопустимый адрес для MAX7357
    }
    if (_muxes.size() >= 8) {
        return -1;   // больше 8 чипов на одной шине быть не может
    }
    _muxes.emplace_back(address);
    return static_cast<int8_t>(_muxes.size() - 1);
}

MAX7357Status MAX7357Cascade::begin(TwoWire &inWire)
{
    _wire = &inWire;
    _wire->begin();

    MAX7357Status last = MAX7357Status::Ok;
    for (auto &m : _muxes) {
        MAX7357Status st = m.begin(inWire);
        if (st != MAX7357Status::Ok) last = st;
    }
    _begun = true;
    return last;
}

MAX7357Status MAX7357Cascade::closeAll()
{
    if (!_begun) return MAX7357Status::NotBegun;

    MAX7357Status last = MAX7357Status::Ok;
    for (auto &m : _muxes) {
        MAX7357Status st = m.closeAll();
        if (st != MAX7357Status::Ok) last = st;
    }
    return last;
}

MAX7357Status MAX7357Cascade::selectPath(uint8_t muxIndex, uint8_t channel)
{
    if (!_begun) return MAX7357Status::NotBegun;
    if (!validIndex(muxIndex)) return MAX7357Status::BadChannel; // или отдельный код
    if (channel > MAX7357_MAX_CHANNEL) return MAX7357Status::BadChannel;

    MAX7357Status last = MAX7357Status::Ok;

    for (uint8_t i = 0; i < _muxes.size(); ++i) {
        MAX7357Status st;
        if (i == muxIndex) {
            st = _muxes[i].selectChannel(channel);
        } else {
            st = _muxes[i].closeAll();
        }
        if (st != MAX7357Status::Ok) last = st;
    }
    return last;
}

MAX7357Status MAX7357Cascade::openChannelOn(uint8_t muxIndex, uint8_t channel)
{
    if (!_begun) return MAX7357Status::NotBegun;
    if (!validIndex(muxIndex)) return MAX7357Status::BadChannel;
    return _muxes[muxIndex].openChannel(channel);
}

MAX7357Status MAX7357Cascade::closeChannelOn(uint8_t muxIndex, uint8_t channel)
{
    if (!_begun) return MAX7357Status::NotBegun;
    if (!validIndex(muxIndex)) return MAX7357Status::BadChannel;
    return _muxes[muxIndex].closeChannel(channel);
}

MAX7357* MAX7357Cascade::at(size_t index)
{
    return index < _muxes.size() ? &_muxes[index] : nullptr;
}

const MAX7357* MAX7357Cascade::at(size_t index) const
{
    return index < _muxes.size() ? &_muxes[index] : nullptr;
}
