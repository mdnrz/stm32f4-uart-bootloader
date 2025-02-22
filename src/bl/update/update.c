#include "update.h"
#include "bl/packet/bl-packet.h"
#include "bl/stimer/stimer.h"
#include "shared/comms/protocol/protocol.h"
#include "shared/memory/flash-io.h"
#include "shared/drivers/drv-gpio.h"
#include <string.h>

typedef enum {
    US_INFINITE_LOOP,
    US_WAIT_FOR_SYNC_REQ,
    US_WAIT_FOR_UPDATE_REQ,
    US_WAIT_FOR_DEV_ID,
    US_WAIT_FOR_FW_LENGTH,
    US_ERASE_OLD_FW,
    US_RECEIVE_UPDATE,
    US_LAST_CHECK,
    US_DONE,
} UpdateState_t;

static uint8_t updateStatus = US_WAIT_FOR_SYNC_REQ;
static uint8_t updateStatusLast = US_WAIT_FOR_SYNC_REQ;
static uint32_t updateSize = 0;

static void update_refresh_indicators(const uint8_t status) {
    do {
        res_gpio_write_pin(ST_LED0_PORT, ST_LED0_PIN, (status >> 3) & 0x01);
        res_gpio_write_pin(ST_LED1_PORT, ST_LED1_PIN, (status >> 2) & 0x01);
        res_gpio_write_pin(ST_LED2_PORT, ST_LED2_PIN, (status >> 1) & 0x01);
        res_gpio_write_pin(ST_LED3_PORT, ST_LED3_PIN, (status >> 0) & 0x01);
    } while (0);
}

void update_state_machine(void) {
    Stimer_t timeout, blink;
    stimer_start(&timeout, 5000, false);
    stimer_start(&blink, 1000, true);
    uint8_t *packetAddr = 0; // NULL
    while (updateStatus != US_DONE) {
        if (stimer_ping(&timeout)) updateStatus = US_LAST_CHECK;
        packetAddr = bl_packet_received();
        if (updateStatus != US_INFINITE_LOOP) update_refresh_indicators(updateStatus);
        switch (updateStatus) {
            case US_INFINITE_LOOP: {
                static uint8_t ledStatus = 0x0F;
                if (packetAddr != 0) {
                    stimer_start(&timeout, 5000, false);
                    updateStatusLast = updateStatus;
                    updateStatus++;
                } else {
                    if (stimer_ping(&blink)) {
                        ledStatus ^= 0x0F;
                        update_refresh_indicators(ledStatus);
                    }
                }
            } break;
            case US_WAIT_FOR_SYNC_REQ: {
                if (packetAddr != 0) {
                    ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetAddr, false);
                    if (attr.tag == PROTOCOL_TAG_SYNC_REQ) {
                        packet_send_ack();
                        updateStatusLast = updateStatus;
                        updateStatus++;
                        stimer_reload(&timeout);
                    } else if (attr.tag == PROTOCOL_TAG_RST) packet_send_ack();
                    else packet_send_nack();
                }
            } break;
            case US_WAIT_FOR_UPDATE_REQ: {
                if (packetAddr != 0) {
                    ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetAddr, false);
                    if (attr.tag == PROTOCOL_TAG_UPDATE_REQ) {
                        packet_send_ack();
                        updateStatusLast = updateStatus;
                        updateStatus++;
                        stimer_reload(&timeout);
                    }
                }
            } break;
            case US_WAIT_FOR_DEV_ID: {
                if (packetAddr != 0) {
                    ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetAddr, false);
                    if (attr.tag == PROTOCOL_TAG_DEV_ID_CHECK &&
                        flash_io_check_dev_id((uint32_t *)(packetAddr + PACK_DATA_INDEX))) {
                        packet_send_ack();
                        updateStatusLast = updateStatus;
                        updateStatus++;
                        stimer_reload(&timeout);
                    } else packet_send_nack();
                }
            } break;
            case US_WAIT_FOR_FW_LENGTH: {
                if (packetAddr != 0) {
                    ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetAddr, false);
                    if (attr.tag == PROTOCOL_TAG_FW_UPDATE_SIZE) {
                        updateSize = flash_io_check_update_size((uint32_t *)(packetAddr + PACK_DATA_INDEX));
                        if (updateSize != 0) {
                            packet_send_ack();
                            updateStatusLast = updateStatus;
                            updateStatus++;
                            stimer_reload(&timeout);
                        } else packet_send_nack();
                    } else packet_send_nack();
                }
            } break;
            case US_ERASE_OLD_FW: {
                if (packetAddr != 0) {
                    ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetAddr, false);
                    if (attr.tag == PROTOCOL_TAG_ERASE_REQ) {
                        if (flash_io_erase_fw_partition()) {
                            packet_send_ack();
                            updateStatusLast = updateStatus;
                            updateStatus++;
                            stimer_start(&timeout, 10000, false);
                        } else packet_send_nack();
                    }
                }
            } break;
            case US_RECEIVE_UPDATE: {
                static uint32_t writtenBytesNo = 0;
                static bool lastWriteSuccess = false;
                if (updateStatusLast == updateStatus - 1) {
                    updateStatusLast = updateStatus;
                    writtenBytesNo = 0;
                    lastWriteSuccess = false;
                }
                if (packetAddr != 0 && writtenBytesNo < updateSize) {
                    ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetAddr, true);
                    if (attr.tag == PROTOCOL_TAG_FW_NEW) {
                        if ((lastWriteSuccess = (flash_io_write_new_firmware(packetAddr + 2, attr.length) == attr.length))) {
                            writtenBytesNo += attr.length;
                            packet_send_ack();
                            stimer_reload(&timeout);
                        } else packet_send_nack();
                    } else if (attr.tag == PROTOCOL_TAG_FW_REP) {
                        if (lastWriteSuccess) {
                            packet_send_ack();
                            stimer_reload(&timeout);
                        } else if ((lastWriteSuccess =
                                    (flash_io_write_new_firmware(packetAddr + 2, attr.length) == attr.length))) {
                            writtenBytesNo += attr.length;
                            packet_send_ack();
                            stimer_reload(&timeout);
                        } else packet_send_nack();
                    } else packet_send_nack();
                } else if (writtenBytesNo == updateSize) {
                    flash_io_write_main_entry();
                    updateStatusLast = updateStatus;
                    updateStatus++;
                }
            } break;
            case US_LAST_CHECK: {
                if (!flash_io_check_fw_integrity()) {
                    updateStatus = US_INFINITE_LOOP;
                } else updateStatus++;
            } break;
            case US_DONE: {
            } break;
        }
    }
    gpio_clear(ST_LED0_PORT, ST_LED0_PIN | ST_LED1_PIN);
    gpio_clear(ST_LED2_PORT, ST_LED2_PIN | ST_LED3_PIN);
}
