#include "protocol.h"
#include "shared/crc/crc16.h"
#include <stdbool.h>

static bool protocol_validate_packet(uint8_t *packet, bool isFirmware) {
    if (isFirmware && crc32_calculate((uint8_t *)packet, packet[PACK_LENGTH_INDEX] + 6) == 0) {
        return true;
    }
    if (!isFirmware && crc16_calculate((uint8_t *)packet, packet[PACK_LENGTH_INDEX] + 4) == 0) {
        return true;
    }
    return false;
}

ProtocolPacketAttr_t protocol_extract_packet_attr(uint8_t *packet, bool isFirmware) { 
    ProtocolPacketAttr_t attr = {
        .offset = 0,
        .length = 0,
    };
    if (isFirmware) {
        if (protocol_validate_packet(packet, isFirmware)) {
            attr.tag = packet[PACK_TAG_INDEX];
            attr.length = packet[PACK_LENGTH_INDEX];
        } else attr.tag = PROTOCOL_TAG_INVALID;
        return attr;
    }
    if (!protocol_validate_packet(packet, isFirmware)) {
        attr.tag = PROTOCOL_TAG_INVALID;
        return attr;
    }
    attr.tag = packet[PACK_TAG_INDEX];
    if (attr.tag == PROTOCOL_TAG_PW) {
        attr.offset = 0;
        attr.length = packet[PACK_LENGTH_INDEX];
        return attr;
    }
    attr.offset = packet[PACK_DATA_INDEX];
    attr.length = packet[PACK_DATA_INDEX + 1];
    return attr;
}

void protocol_create_packet(uint8_t *buffer, ProtocolTag_t tag, uint8_t length) {
    buffer[PACK_TAG_INDEX] = tag;
    buffer[PACK_LENGTH_INDEX] = length;
    uint16_t crcMsbIndex = length + 4 - 2;
    uint16_t crcLsbIndex = length + 4 - 1;
    buffer[crcMsbIndex] = 0;
    buffer[crcLsbIndex] = 0;
    uint16_t crcVal = crc16_calculate(buffer, length + 4);
    buffer[crcMsbIndex] = (crcVal >> 8 ) & 0xFF;
    buffer[crcLsbIndex] = crcVal & 0xFF;
}
