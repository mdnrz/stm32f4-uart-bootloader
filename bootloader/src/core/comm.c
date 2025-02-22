#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "comm.h"
#include "ser.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include "jsmn-helper.h"

#define DMA_RX_BUFFER_SIZE = 128
#define PACKET_MAX_DATA_LEN = DMA_RX_BUFFER_SIZE - 1

#define PACK_TAG_INDEX = 0
#define PACK_LENGTH_INDEX = 1

static int port_fd;

typedef enum __attribute__((packed)) {
    PR = 0, 
    PW = 1,
    FIR = 2,
    RST = 3,
    ACK = 4,
    NACK = 5,
    LOCK = 6,
    UNLOCK = 7,
    DIAG = 8,
    SYNC = 9,
    UPDATE_REQ = 10,
    DEV_ID_CHECK = 11,
    UPDATE_SIZE = 12,
    ERASE_REQ = 13,
    FW_NEW = 14,
    FW_REP = 15,
} Tag_t;

enum results {
    SUCCESS,
    TIMEOUT,
    TX_CORRUPT,
    RX_CORRUPT,
};

typedef struct {
    uint8_t code;
    char *desc;
} Result_t;

Result_t resultTable[4] = {
    { .code = SUCCESS, .desc = "Success" },
    { .code = TIMEOUT, .desc = "Timeout" },
    { .code = TX_CORRUPT, .desc = "NACK received" },
    { .code = RX_CORRUPT, .desc = "RX packet Corruption" },
};


typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t sha1[5];
} FWVersion_t;

typedef struct __attribute__((packed)) {
    uint32_t sentinel;
    uint32_t devID;
    FWVersion_t fwVersion;
    uint32_t fwLength;
    uint32_t crc32;
} FWInfo_t;

typedef enum {
    BOOTLOADER,
    APPLICATION,
} CurrentInstance_t;

typedef struct {
    uint8_t active;
    uint8_t ch[2];
} Lane_t;

typedef struct __attribute__((packed)) {
    bool switchingActive;
    uint32_t TB_SW_frequency;
} CaptureConfig_t;

typedef struct {
    Lane_t lane0;
    Lane_t lane1;
    Lane_t lane2;
    Lane_t lane3;
} Layout_t;

typedef struct __attribute__((packed)) {
    CurrentInstance_t currentInstance;
    float entranceTh;
    float exitTh;
    uint32_t entranceDB;
    uint32_t exitDB;
    Layout_t layout;
    CaptureConfig_t captureConfig;
    uint8_t filterAvgWindow;
} Param_t;

typedef struct {
    Tag_t tag;
    char desc[30];
} TagDesc_t;

static uint16_t crc16_calculate(const uint8_t *data, uint16_t size) {
#define POLY16 (uint16_t)0x8005
    uint16_t out = 0;
    int bits_read = 0, bit_flag;
    /* Sanity check: */
    if (data == NULL)
        return 0;
    while (size > 0) {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> (7 - bits_read)) & 1;
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7) {
            bits_read = 0;
            data++;
            size--;
        }
        /* Cycle check: */
        if (bit_flag) {
            out ^= POLY16;
        }
    }
    return out;
}

static uint32_t crc32_calculate(const uint8_t *data, uint16_t size) {
#define POLY32 (uint32_t)0xEDB88320
    uint32_t out = 0;
    int bits_read = 0, bit_flag;
    /* Sanity check: */
    if (data == NULL)
        return 0;
    while (size > 0) {
        bit_flag = out >> 31;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> (7 - bits_read)) & 1;
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7) {
            bits_read = 0;
            data++;
            size--;
        }
        /* Cycle check: */
        if (bit_flag) {
            out ^= POLY32;
        }
    }
    return out;
}

static int load_param_from_json(Param_t *prm)
{

    if (jsmn_load_json_to_memory("param.json") == -1) return -1;
    Value_t value = {0};
    value = jsmn_get_value(UINT, "currentInstance", NULL);
    prm->currentInstance = value.uvalue;
    value = jsmn_get_value(FLOAT, "entranceTh", NULL);
    prm->entranceTh = value.fvalue;
    value = jsmn_get_value(FLOAT, "exitTh", NULL);
    prm->exitTh = value.fvalue;
    value = jsmn_get_value(UINT, "entranceDB", NULL);
    prm->entranceDB = value.uvalue;
    value = jsmn_get_value(UINT, "exitDB", NULL);
    prm->exitDB = value.uvalue;
    value = jsmn_get_value(UINT, "layout", "lane0", "active", NULL);
    prm->layout.lane0.active = value.uvalue;
    value = jsmn_get_value(UINT, "layout", "lane1", "active", NULL);
    prm->layout.lane1.active = value.uvalue;
    value = jsmn_get_value(UINT, "layout", "lane2", "active", NULL);
    prm->layout.lane2.active = value.uvalue;
    value = jsmn_get_value(UINT, "layout", "lane3", "active", NULL);
    prm->layout.lane3.active = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane0", "ch", 0, NULL);
    prm->layout.lane0.ch[0] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane0", "ch", 1, NULL);
    prm->layout.lane0.ch[1] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane1", "ch", 0, NULL);
    prm->layout.lane1.ch[0] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane1", "ch", 1, NULL);
    prm->layout.lane1.ch[1] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane2", "ch", 0, NULL);
    prm->layout.lane2.ch[0] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane2", "ch", 1, NULL);
    prm->layout.lane2.ch[1] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane3", "ch", 0, NULL);
    prm->layout.lane3.ch[0] = value.uvalue;
    value = jsmn_get_value(ARRAY, "layout", "lane3", "ch", 1, NULL);
    prm->layout.lane3.ch[1] = value.uvalue;
    value = jsmn_get_value(UINT, "captureConfig", "switchingActive", NULL);
    prm->captureConfig.switchingActive = value.uvalue;
    value = jsmn_get_value(UINT, "captureConfig", "TB_SW_frequency", NULL);
    prm->captureConfig.TB_SW_frequency = value.uvalue;
    value = jsmn_get_value(UINT, "filterAvgWindow", NULL);
    prm->filterAvgWindow = value.uvalue;
    return 0;
}

void create_packet(uint8_t *packet, Tag_t tag, uint8_t *data, uint16_t data_sz)
{
    memset(packet, 0, 1024);
    uint8_t packetSize = data_sz + 4;
    packet[0] = tag;
    packet[1] = data_sz;
    if (data != NULL) memcpy(&packet[2], data, data_sz);
    uint16_t crc = crc16_calculate(packet, packetSize);
    packet[2+data_sz] = (crc >> 8) & 0xFF;
    packet[3+data_sz] = crc & 0xFF;
}

static void create_fw_packet(uint8_t *packet, Tag_t tag, uint8_t *data, size_t data_sz)
{
    memset(packet, 0, 1024);
    uint8_t packetSize = data_sz + 6;
    packet[0] = tag;
    packet[1] = data_sz;
    if (data != NULL) memcpy(&packet[2], data, data_sz);
    uint32_t crc = crc32_calculate(packet, packetSize);
    packet[2+data_sz] = (crc >> 24) & 0xFF;
    packet[3+data_sz] = (crc >> 16) & 0xFF;
    packet[4+data_sz] = (crc >> 8) & 0xFF;
    packet[5+data_sz] = crc & 0xFF;
}

void parse_parameters(uint8_t *raw, char *response)
{
    Param_t *params = (Param_t *)(raw + 2);
    sprintf(response, "current instance = %u\nentranceTh = %.0f\nexitTh = %.0f\nentranceDB = %u\nexitDB = %u\nlane0: {active: %u, ch = [%u, %u]}\nlane1: {active: %u, ch = [%u, %u]}\nlane2: {active: %u, ch = [%u, %u]}\nlane3: {active: %u, ch = [%u, %u]}\nswitching: %u, frequency: %u\nfilterAvgWindow: %u\n"
            , params->currentInstance, params->entranceTh, params->exitTh
            , params->entranceDB , params->exitDB
            , params->layout.lane0.active, params->layout.lane0.ch[0], params->layout.lane0.ch[1]
            , params->layout.lane1.active, params->layout.lane1.ch[0], params->layout.lane1.ch[1]
            , params->layout.lane2.active, params->layout.lane2.ch[0], params->layout.lane2.ch[1]
            , params->layout.lane3.active, params->layout.lane3.ch[0], params->layout.lane3.ch[1]
            , params->captureConfig.switchingActive, params->captureConfig.TB_SW_frequency
            , params->filterAvgWindow);
}

void parse_fw_info(uint8_t *raw, char *response)
{
    FWInfo_t *info = (FWInfo_t *)(raw + 2);
    sprintf(response, "sentinel = 0x%x\ndevice ID = %u\nVersion = v%u.%u-%x-%c%c%c%c%c\nFirmware Size = %u\nCRC32 = 0x%x",
            info->sentinel, info->devID, info->fwVersion.major, info->fwVersion.minor,
            info->fwVersion.patch, info->fwVersion.sha1[0], info->fwVersion.sha1[1],
            info->fwVersion.sha1[2], info->fwVersion.sha1[3], info->fwVersion.sha1[4],
            info->fwLength, info->crc32);
}

static Result_t update_check_response(uint8_t *rx_buffer, size_t len)
{
    if (len == 0) return resultTable[TIMEOUT];
    if (crc16_calculate(rx_buffer, len) != 0) return resultTable[RX_CORRUPT];
    if (rx_buffer[0] == NACK) return resultTable[TX_CORRUPT];
    return resultTable[SUCCESS];
}

static void update_state_machine(char *response)
{
    sleep(1);
    uint8_t packet[1024] = {0};
    uint8_t rx_buffer[1024] = {0};
    uint8_t retry = 0;
    uint8_t state = SYNC;
    struct stat fwStat;
    memset(response, 0, 1024);
    while (state != 0) {
        if (retry > 0) {
            if (retry >= 5) state = 0;
        }
        switch(state) {
            case SYNC:
                {
                    create_packet(packet, SYNC, NULL, 0);
                    ser_send_packet(port_fd, packet, 4);
                    int len = ser_listen_and_receive_packet(port_fd, rx_buffer, 4, 10);
                    Result_t result = update_check_response(rx_buffer, len);
                    if (result.code == resultTable[SUCCESS].code) {
                        sleep(1);
                        state++;
                    } else {
                        sprintf(response, "\nFailed at Level SYNC: %s", result.desc); 
                        retry++;
                    }
                } break;
            case UPDATE_REQ:
                {
                    create_packet(packet, UPDATE_REQ, NULL, 0);
                    ser_send_packet(port_fd, packet, 4);
                    int len = ser_listen_and_receive_packet(port_fd, rx_buffer, 4, 10);
                    Result_t result = update_check_response(rx_buffer, len);
                    if (result.code == resultTable[SUCCESS].code) {
                        sleep(1);
                        state++;
                    } else {
                        sprintf(response, "\nFailed at Level UPDATE_REQ: %s", result.desc); 
                        retry++;
                    }
                } break;
            case DEV_ID_CHECK:
                {
                    FILE *devidFile = fopen("devID", "r");
                    if (devidFile == NULL) {
                        sprintf(response, "\nFailed at level DEV_ID_CHECK: opening devID file: %s", strerror(errno));
                        state = 0;
                        break;
                    }
                    char buffer[7]; // 0x[dddd][\n]
                    fgets(buffer, 7, devidFile);
                    uint32_t devid = (uint32_t)strtoul(buffer, NULL, 16);
                    create_packet(packet, DEV_ID_CHECK, (uint8_t *)&devid, 4);
                    ser_send_packet(port_fd, packet, 8);
                    int len = ser_listen_and_receive_packet(port_fd, rx_buffer, 4, 10);
                    Result_t result = update_check_response(rx_buffer, len);
                    if (result.code == resultTable[SUCCESS].code) {
                        state++;
                    } else {
                        sprintf(response, "\nFailed at Level DEV_ID_CHECK: %s", result.desc); 
                        retry++;
                    }
                } break;
            case UPDATE_SIZE:
                {
                    if (stat("fw.bin", &fwStat) != 0) {
                        sprintf(response, "\nFailed at level UPDATE_SIZE: Getting fw.bin file: %s", strerror(errno));
                        state = 0;
                        break;
                    }
                    uint32_t fwSize = (uint32_t)fwStat.st_size;
                    create_packet(packet, UPDATE_SIZE, (uint8_t *)&fwSize, 4);
                    ser_send_packet(port_fd, packet, 8);
                    int len = ser_listen_and_receive_packet(port_fd, rx_buffer, 4, 10);
                    Result_t result = update_check_response(rx_buffer, len);
                    if (result.code == resultTable[SUCCESS].code) {
                        state++;
                    } else {
                        sprintf(response, "\nFailed at Level UPDATE_SIZE: %s", result.desc); 
                        retry++;
                    }
                } break;
            case ERASE_REQ:
                {
                    create_packet(packet, ERASE_REQ, NULL, 0);
                    ser_send_packet(port_fd, packet, 4);
                    int len = ser_listen_and_receive_packet(port_fd, rx_buffer, 4, 10);
                    Result_t result = update_check_response(rx_buffer, len);
                    if (result.code == resultTable[SUCCESS].code) {
                        state++;
                    } else {
                        sprintf(response, "\nFailed at Level ERASE_REQ: %s", result.desc); 
                        retry++;
                    }
                } break;
            case FW_NEW:
                {
                    uint8_t chunk[128] = {0};
                    uint16_t pack = 0;
                    uint8_t bytes_read_into_chunk = 0;
                    FILE *fwFile = fopen("fw.bin", "rb");
                    if (fwFile == NULL) {
                        sprintf(response, "\nFailed at level FW_NEW: Opening fw.bin file: %s", strerror(errno));
                        state = 0;
                        break;
                    }
                    uint32_t totalChunk = (fwStat.st_size / 128) + 1;
                    pack = 1;
                    retry = 0;
                    while (retry <= 5) {
                        if (retry == 0) bytes_read_into_chunk = fread(chunk, 1, 128, fwFile);
                        if (bytes_read_into_chunk == 0) {
                            break;
                        }
                        if (retry == 0) {
                            pack++;
                            create_fw_packet(packet, FW_NEW, chunk, bytes_read_into_chunk);
                        } else create_fw_packet(packet, FW_REP, chunk, bytes_read_into_chunk);
                        ser_send_packet(port_fd, packet, bytes_read_into_chunk + 6);
                        int len = ser_listen_and_receive_packet(port_fd, rx_buffer, 4, 1);
                        if (len != 0 && crc16_calculate(rx_buffer, len) == 0) {
                            if (rx_buffer[0] == ACK) retry = 0;
                            else retry++;
                        } else retry++;
                    }
                    fclose(fwFile);
                    if (pack == totalChunk + 1) sprintf(response + strlen(response), "\nAll good!");
                    else sprintf(response + strlen(response), "\nFailed at level FW_NEW: chunks: %u/%u", pack, totalChunk);
                    state = 0;
                } break;
        }
    }
}
static void protocol_state_machine(uint8_t *rx_buffer, size_t len, Command_t command, char *response)
{
    memset(response, 0, 1024);
    if (crc16_calculate(rx_buffer, len) != 0) {
        sprintf(response, "CRC Error\n");
    }
    switch(rx_buffer[0]) {
        case ACK:
            {
                if (command == COMMAND_RFI) {
                    parse_fw_info(rx_buffer, response);
                } else if (command == COMMAND_PR) {
                    parse_parameters(rx_buffer, response);
                } else if (command == COMMAND_PU) {
                    sprintf(response, "Parameters updated successfully.");
                } else if (command == COMMAND_RST) {
                    sprintf(response, "Core rebooted. Sending Sync command ...");
                    update_state_machine(response);
                } else if (command == COMMAND_LOCK || command == COMMAND_UNLOCK) {
                    sprintf(response, "Process successfull");
                } else if (command == COMMAND_PU) {
                    sprintf(response, "Not implemented");
                } else if (command == COMMAND_RDIAG) {
                    if (rx_buffer[1] == 0) sprintf(response, "No diagnostics saved");
                    else {
                        sprintf(response, "Diagnostics:\n");
                        for (uint16_t i = 0; i < len; i++) {
                            sprintf(response + strlen(response), "%u -> %x\n", i+1, rx_buffer[i+2]);
                        }
                    }
                }
            } break;
        case NACK:
            {
                sprintf(response, "Nack received");
            } break;
    }
}

uint16_t execute_command(uint8_t command, char *response)
{
    uint8_t packet[1024];
    uint8_t rx_size = 0;
    switch(command) {
        case COMMAND_PR:
            {
                uint8_t data[2] = {0, sizeof(Param_t)};
                create_packet(packet, PR, data, sizeof(data)/sizeof(data[0]));
                ser_send_packet(port_fd, packet, 6);
                rx_size = sizeof(Param_t) + 4;
            } break;
        case COMMAND_PU:
            {
                Param_t prm = {0};
                if (load_param_from_json(&prm) == -1) {
                    sprintf(response, "Loading param.json: %s", strerror(errno));
                    return strlen(response);
                }
                create_packet(packet, PW, (uint8_t *)&prm, sizeof(Param_t));
                ser_send_packet(port_fd, packet, sizeof(Param_t) + 4);
                rx_size = 4;
            } break;
        case COMMAND_RFI:
            {
                uint8_t data[2] = {0, sizeof(FWInfo_t)};
                create_packet(packet, FIR, data, sizeof(data) / sizeof(data[0]));
                ser_send_packet(port_fd, packet, 6);
                rx_size = sizeof(FWInfo_t) + 4;
            } break;
        case COMMAND_RST:
            {
                create_packet(packet, RST, NULL, 0);
                ser_send_packet(port_fd, packet, 4);
                rx_size = 4;
            } break;
        case COMMAND_LOCK:
            {
                create_packet(packet, LOCK, NULL, 0);
                ser_send_packet(port_fd, packet, 4);
                rx_size = 4;
            } break;

        case COMMAND_UNLOCK:
            {
                create_packet(packet, UNLOCK, NULL, 0);
                ser_send_packet(port_fd, packet, 4);
                rx_size = 4;
            } break;

        case COMMAND_RDIAG:
            {
                create_packet(packet, DIAG, NULL, 0);
                ser_send_packet(port_fd, packet, 4);
                rx_size = 4;
            } break;
        default: return 0;;
    }
    uint8_t rx_buffer[1024] = {0};
    int len = ser_listen_and_receive_packet(port_fd, rx_buffer, rx_size, 2);
    if (len > 0) {
        protocol_state_machine(rx_buffer, len, command, response);
        return strlen(response);
    } else if (len == 0) {
        sprintf(response, "Timeout\n");
        return strlen(response);
    }
    sprintf(response, "Error reading from serial port: %s\nProbably disconnected USB-Serial converter", strerror(errno));
    return strlen(response);
}

int comm_open_serial_port(char *addr, char *response)
{
    port_fd = open(addr, O_RDWR | O_NOCTTY);
    if (port_fd == -1) {
        sprintf(response, "Error openning port %s: %s\n", addr, strerror(errno));
        return port_fd;
    }
    if (configure_serial_port(port_fd) == -1) {
        sprintf(response, "Error configuring port %s: %s\n", addr, strerror(errno));
        return port_fd;
    }
    return port_fd;
}

void comm_close_serial_port(void)
{
    close(port_fd);
}
