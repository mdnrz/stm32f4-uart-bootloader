#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

#define PACKET_MAX_DATA_LEN (DMA_RX_BUFFER_SIZE - 5)

/*************************************************************
 * TLV: Tag, Length, Value
 * Packet Structure:
 *  |------|------|------|------|-----|------|------|------|
 *  | tag  | len* | dat1 | dat2 | ... | datn | crcM | crcL |
 *  |______|______|______|______|_____|______|______|______|
 *
 * Note: len specifies the number of data bytes, NOT the packet length.

 *
 * ****** Read parameters (PR) Command structure:
 *  | tag | len | addr |  n  | crcM | crcL |
 *  len  = 2
 *  addr = address to read from
 *  n    = number of bytes to read
 *
 *  Response structure:
 *  | tag | len | data1 ... datan | crcM | crcL
 *  tag = ACK or NACK
 *  len = n(if ACK) or 0(if NACK)
 * 

 *  ****** Write parameters (PW) Command structure:
 *  | tag | len | dat0 | dat1 |...| datn | crcM | crcL |
 *  len  = (number of data bytes)
 *  NOTE: write address is always zero. Because in the process
 *  of updating parameters stored in flash memeory, the entire
 *  sector should be wiped out anyway.
 *
 *  Response structure:
 *  | tag | len | crcM | crcL
 *  tag = ACK or NACK
 *  len = 0
 * 
 * ****** Read firmware info (FIR) Command structure:
 *  | tag | len | addr |  n  | crcM | crcL |
 *  len  = 2
 *  addr = address to read from
 *  n    = number of bytes to read
 *
 *  Response structure:
 *  | tag | len | data1 ... datan | crcM | crcL
 *  tag = ACK or NACK
 *  len = n(if ACK) or 0(if NACK)
 * 
 ************************************************************/

#define PACK_TAG_INDEX 0U
#define PACK_LENGTH_INDEX 1U
#define PACK_DATA_INDEX 2

typedef enum {
    PROTOCOL_TAG_PR,
    PROTOCOL_TAG_PW,
    PROTOCOL_TAG_FIR,
    PROTOCOL_TAG_RST,
    PROTOCOL_TAG_ACK,
    PROTOCOL_TAG_NACK,
    PROTOCOL_TAG_LOCK,
    PROTOCOL_TAG_UNLOCK,
    PROTOCOL_TAG_DIAG,

    // Frimware update tags 
    PROTOCOL_TAG_SYNC_REQ,
    PROTOCOL_TAG_UPDATE_REQ,
    PROTOCOL_TAG_DEV_ID_CHECK,
    PROTOCOL_TAG_FW_UPDATE_SIZE,
    PROTOCOL_TAG_ERASE_REQ,
    PROTOCOL_TAG_FW_NEW,
    PROTOCOL_TAG_FW_REP,

    PROTOCOL_TAG_INVALID,
} ProtocolTag_t;

typedef struct {
    ProtocolTag_t tag;
    uint8_t offset;
    uint8_t length;
} ProtocolPacketAttr_t;

typedef struct __attribute__((__packed__)) {
    ProtocolTag_t tag;
    uint8_t length;
    uint16_t crc;
} ProtocolPacket_t;

ProtocolPacketAttr_t protocol_extract_packet_attr(uint8_t *packet, bool isFirmware);
void protocol_create_packet(uint8_t *buffer, ProtocolTag_t tag, uint8_t length);

#endif // __PROTOCOL_H__
