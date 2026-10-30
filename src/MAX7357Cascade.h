#ifndef MAX7357_CASCADE_H
#define MAX7357_CASCADE_H

#include "MAX7357.h"
#include <vector>
#include <stdint.h>

/**
 * Каскад MAX7357.
 * Каждый элемент массива _muxes соответствует физическому чипу
 * с уникальным адресом на общей шине.
 */
class MAX7357Cascade
{
public:
    MAX7357Cascade() = default;

    /**
     * Добавить мультиплексор в каскад.
     * address — 7-битный адрес (0x70..0x77).
     * Возвращает индекс добавленного мультиплексора или -1 при переполнении.
     */
    int8_t addMux(uint8_t address);

    /** Инициализировать все мультиплексоры на общей шине. */
    MAX7357Status begin(TwoWire &inWire = Wire);

    /** Закрыть все каналы на всех мультиплексорах. */
    MAX7357Status closeAll();

    /**
     * Выбрать один канал на одном мультиплексоре.
     * На выбранном мультиплексоре вызывается selectChannel(channel),
     * на остальных — closeAll().
     */
    MAX7357Status selectPath(uint8_t muxIndex, uint8_t channel);

    /**
     * Открыть канал на выбранном мультиплексоре, не трогая остальные.
     * Полезно, если нужно несколько одновременно открытых каналов
     * на разных мультиплексорах.
     */
    MAX7357Status openChannelOn(uint8_t muxIndex, uint8_t channel);

    /** Закрыть канал на выбранном мультиплексоре. */
    MAX7357Status closeChannelOn(uint8_t muxIndex, uint8_t channel);

    /** Количество мультиплексоров в каскаде. */
    size_t count() const { return _muxes.size(); }

    /** Доступ к мультиплексору по индексу (для чтения регистров и т.п.). */
    MAX7357* at(size_t index);
    const MAX7357* at(size_t index) const;

private:
    std::vector<MAX7357> _muxes;
    TwoWire *_wire = nullptr;
    bool _begun = false;

    bool validIndex(uint8_t index) const { return index < _muxes.size(); }
};

#endif // MAX7357_CASCADE_H