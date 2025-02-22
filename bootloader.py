#!/usr/bin/python

from construct import *
import serial
import time
import ast
import os
import sys

DMA_RX_BUFFER_SIZE = 128
PACKET_MAX_DATA_LEN = DMA_RX_BUFFER_SIZE - 1

PACK_TAG_INDEX = 0
PACK_LENGTH_INDEX = 1

fwInfoStruct = Struct(
        "sentinel" / Int32ul,
        "devID" / Int32ul,
        "fwVersion" / Struct(
                "major" / Int8ul,
                "minor" / Int8ul,
                "patch" / Int8ul,
                "sha1" / Array(5, Int8ul)
                ),
        "fwLength" / Int32ul,
        "crc32" / Int32ul,
        )

paramStruct = Struct(
        "data" / Struct(
            "float_param_1" / Float32l,
            "float_param_2" / Float32l,
            "dw_param" / Int32ul,
            "w_param" / Int16ul,
            "bool_param_1" / Int8ul,
            "bool_param_2" / Int8ul,
            )
        )

packetStruct = Struct(
        "tag" / Enum(Byte, 
                     PR=0, 
                     PW=1,
                     FIR=2,
                     RST=3,
                     ACK=4,
                     NACK=5,
                     LOCK=6,
                     UNLOCK=7,
                     DIAG=8,
                     SYNC=9,
                     UPDATE_REQ=10,
                     DEVID_CHECK=11,
                     UPDATE_SIZE=12,
                     ERASE_REQ=13,
                     ),
        "len" / Int8ul,
        "data" / Array(this.len, Byte),
        "crc" / Byte[2],
        )

fwChunkStruct = Struct(
        "tag" / Enum(Byte,
                     FW_NEW=14,
                     FW_REP=15,
                     ),
        "len" / Int8ul,
        "data" / Array(this.len, Byte),
        "crc32" / Byte[4],
        )

port = input('\nEnter the serial port:\nInput: ')
# port = '/dev/ttyUSB0'
ser = serial.Serial(port)
ser.baudrate = 921600
action = 0
waitForPacket = False

packetType = Enum(Byte, readInfo=0, readParam=1, reset=2, pUpdate=3, serialLock=4, serialFree=5, readDiag=6)

def calculate_crc16(data: bytes, poly=0x8005):
    '''
    CRC-16-CCITT Algorithm
    '''
    data = bytearray(data)
    crc = 0x0000
    for b in data:
        cur_byte = 0xFF & b
        for i in range(0, 8):
            bit_flag = (crc >> 15) & 0x01
            crc = crc << 1
            crc = crc | (cur_byte >> (7 - i)) & 1
            if bit_flag:
                crc = crc ^ poly
    return crc & 0xFFFF

def calculate_crc32(data: bytes, poly=0xEDB88320):
    '''
    CRC-16-CCITT Algorithm
    '''
    data = bytearray(data)
    crc = 0x00000000
    for b in data:
        cur_byte = 0xFF & b
        for i in range(0, 8):
            bit_flag = (crc >> 31) & 0x01
            crc = crc << 1
            crc = crc | (cur_byte >> (7 - i)) & 1
            if bit_flag:
                crc = crc ^ poly
    return crc & 0xFFFFFFFF

def create_packet(tag: Enum, data = [], isFirmware = False):
    if not isFirmware:
        tempPacket = packetStruct.build(dict(tag = tag, len = len(data), data = data, crc = [0, 0]))
        crcVal = calculate_crc16(tempPacket)
        packet = packetStruct.build(dict(tag = tag, len = len(data), data = data, 
                                         crc = [(crcVal >> 8) & 0xff, crcVal & 0xff]))
    elif isFirmware:
        fwChunk = fwChunkStruct.build(dict(tag = tag, len = len(data), data = data, crc32 = [0, 0, 0, 0]))
        crc32Val = calculate_crc32(fwChunk)
        packet = fwChunkStruct.build(dict(tag = tag, len = len(data), data = data, 
            crc32 = [(crc32Val >> 24) & 0xff, (crc32Val >> 16) & 0xff, (crc32Val >> 8) & 0xff, crc32Val & 0xff]))
    return packet

def update_state_machine():
    time.sleep(1)
    retry = 0
    state = "SYNC"
    while state != 'DONE':
        if retry > 0:
            print("\nRetrying...",retry)
            if retry >= 5:
                state = 'DONE'
        match state:
            case 'SYNC':
                print("[INFO] Sending sync request...")
                packet = create_packet("SYNC", [])
                ser.write(packet)
                ser.timeout = 1
                response = ser.read(5)
                if len(response) != 0 and calculate_crc16(response) == 0:
                    packet = packetStruct.parse(response)
                    if packet.tag == 'ACK':
                        retry = 0
                        print("[SUCCESS] Synced")
                        state = 'UPDATE_REQ'
                    else: 
                        print("[ERROR] NACK received")
                        retry += 1
                else:
                    print("[TIMEOUT] No response received")
                    retry += 1
            case 'UPDATE_REQ':
                print("[INFO] Sending update request...")
                packet = create_packet("UPDATE_REQ", [])
                ser.write(packet)
                ser.timeout = 1
                response = ser.read(5)
                if len(response) != 0 and calculate_crc16(response) == 0:
                    packet = packetStruct.parse(response)
                    if packet.tag == 'ACK':
                        retry = 0
                        print("[SUCCESS] Update request acknowledged")
                        state = 'DEVID_CHECK'
                    else: 
                        print("[ERROR] NACK received")
                        retry += 1
                else:
                    print("[TIMEOUT] No response received")
                    retry += 1
            case 'DEVID_CHECK':
                try: 
                    devIDFile = open('devID', 'r')
                except OSError:
                    print("[ERROR] devID file not found.")
                    print("[INFO] Crete a file named \"devID\" in the current directory and write target device id in it.")
                    sys.exit()
                with devIDFile:
                    devID = ast.literal_eval(devIDFile.read())
                    data = devID.to_bytes(4, byteorder = 'little')
                print("[INFO] Checking device id...")
                packet = create_packet("DEVID_CHECK", data)
                ser.write(packet)
                ser.timeout = 1
                response = ser.read(5)
                if len(response) != 0 and calculate_crc16(response) == 0:
                    packet = packetStruct.parse(response)
                    if packet.tag == 'ACK':
                        retry = 0
                        print("[SUCCESS] Device ID is correct")
                        state = 'UPDATE_SIZE'
                    else: 
                        print("[ERROR] NACK received")
                        retry += 1
                else:
                    print("[TIMEOUT] No response received")
                    retry += 1
            case 'UPDATE_SIZE':
                try: 
                    firmware = open('./build/fw.bin', 'r')
                except OSError:
                    print("[ERROR] File \"./build/fw.bin\" not found.")
                    sys.exit()
                with firmware:
                    FWSize = os.path.getsize('./build/fw.bin')
                    data = FWSize.to_bytes(4, byteorder = 'little')
                print("[INFO] Sending update size...")
                packet = create_packet("UPDATE_SIZE", data)
                ser.write(packet)
                ser.timeout = 1
                response = ser.read(5)
                if len(response) != 0 and calculate_crc16(response) == 0:
                    packet = packetStruct.parse(response)
                    if packet.tag == 'ACK':
                        retry = 0
                        print("[SUCCESS] Update size deliviered")
                        state = 'ERASE_REQ'
                    else: 
                        print("[ERROR] NACK received")
                        retry += 1
                else:
                    print("[TIMEOUT] No response received")
                    retry += 1
            case 'ERASE_REQ':
                print("[INFO] Sending erase request...")
                packet = create_packet("ERASE_REQ", [])
                ser.write(packet)
                ser.timeout = 4
                response = ser.read(4)
                if len(response) != 0 and calculate_crc16(response) == 0:
                    packet = packetStruct.parse(response)
                    if packet.tag == 'ACK':
                        retry = 0
                        print("[SUCCESS] Chip erase done")
                        state = 'FIRMWARE_UPLOAD'
                    else: 
                        print("[ERROR] NACK received")
                        retry += 1
                else:
                    print("[TIMEOUT] No response received")
                    retry += 1
            case 'FIRMWARE_UPLOAD':
                try: 
                    firmware = open('./build/fw.bin', 'rb')
                except OSError:
                    print("[ERROR] File \"./build/fw.bin\" not found.")
                    sys.exit()
                with firmware:
                    totalChunk = (FWSize // 128) + 1
                    pack = 1
                    while True and retry <= 5:
                        if retry == 0:
                            chunk = firmware.read(128)
                        if not chunk: 
                            break
                        persent = pack * 100 // totalChunk
                        progress = persent // 2
                        print("Writing chunk",pack,"/",totalChunk,"[",
                              ''.join('#'*progress),
                              ''.join(' '*(50 - progress)),"]",persent,"%",end = "\r")
                        if retry == 0:
                            pack += 1
                            packet = create_packet("FW_NEW", chunk, True)
                        else:
                            packet = create_packet("FW_REP", chunk, True)
                        ser.write(packet)
                        ser.timeout = 4
                        response = ser.read(4)
                        if len(response) != 0 and calculate_crc16(response) == 0:
                            packet = packetStruct.parse(response)
                            if packet.tag == 'ACK':
                                retry = 0
                            else: 
                                print("\n[ERROR] NACK received. Resending chunk:",pack)
                                retry += 1
                        else:
                            print("[TIMEOUT] No response received. Resending chunk:",pack)
                            retry += 1
                if pack == totalChunk + 1:
                    print("\n[INFO] Firmware update was successfull")
                else: 
                    print("\n[ERROR] Firmware upload failed")
                state = 'DONE'

def protocol_state_machine(prx: Bytes, type: Enum):
    # print("received: {:x}",prx)
    # print (''.join('{:02x}, '.format(x) for x in prx))
    if calculate_crc16(prx) != 0:
        print("CRC Error\n")
    else:
        packet = packetStruct.parse(prx)
        match packet.tag:
            case 'ACK':
                if type == 'readInfo':
                    fwInfo = fwInfoStruct.parse(prx[2:len(packet.data)+2])
                    print("####################################")
                    print("Sentinel: %#4x"% fwInfo.sentinel)
                    print("Device ID: %d"% fwInfo.devID)
                    print("FW Version: v{0:1d}.{1:1d}-{2:1d}-{3:1}{4:1}{5:1}{6:1}{7:1}".format(fwInfo.fwVersion.major, fwInfo.fwVersion.minor, fwInfo.fwVersion.patch, chr(fwInfo.fwVersion.sha1[0]), chr(fwInfo.fwVersion.sha1[1]), chr(fwInfo.fwVersion.sha1[2]), chr(fwInfo.fwVersion.sha1[3]), chr(fwInfo.fwVersion.sha1[4])))
                    print("FW Size: %4d"% fwInfo.fwLength)
                    print("CRC-32: %#4x"% fwInfo.crc32)
                    print("####################################")
                elif type == 'readParam':
                    print("####################################")
                    params = paramStruct.parse(prx[2:len(packet.data)+2])
                    print("Current Instance: ",params.currentInstance)
                    print("Entrance Threshold: %d"% params.entranceTh)
                    print("Exit Threshold: %d"% params.exitTh)
                    print("Entrance Debounce: %d"% params.entranceDB)
                    print("Exit Debounce: %d"% params.exitDB)
                    print("Layout: ",params.layout)
                    print("capture config: ",params.captureConfig)
                    print("Filter Avg Window: ",params.filterAvgWindow)
                    print("####################################")
                elif type == 'pUpdate':
                    print("####################################")
                    print("Parameters updated successfully.")
                    print("####################################")
                elif type == 'reset':
                    print("Core rebooted. Sending sync command ...")
                    update_state_machine()
                elif type == 'serialLock' or type == 'serialFree':
                    print("####################################")
                    print("Process successfull\n")
                    print("####################################")
                elif type == 'readDiag':
                    if packet.len == 0:
                        print("####################################")
                        print("no diagnostics saved\n")
                        print("####################################")
                    else:
                        print("####################################")
                        print("Diagnostics:\n")
                        for i in range(0, packet.len):
                            print(i+1,"->","".join(hex(packet.data[i])))
                        print("####################################")

            case 'NACK':
                print("####################################")
                print("Nack received\n")
                print("####################################")


while (action != '0'):
    action = input('\nSelect an action to perform:\n\
            1. Read parameters\n\
            2. Update parameters\n\
            3. Read firmware info\n\
            4. Reset the core\n\
            5. Lock serial line\n\
            6. Unlock serial line\n\
            7. Read diagnostics\n\
            0. Abort\n\
            Input: ')
    match action:
        case '1':
            packet = create_packet("PR", [0, paramStruct.sizeof()])
            ser.write(packet)
            queryType = "readParam"
            waitForPacket = True
        case '2':
            print('\nLoading parameters from config file ...')
            time.sleep(0.2)
            fileParam = ast.literal_eval(open('param.txt', 'r').read())
            packet = create_packet("PW", paramStruct.build(fileParam))
            print('\nUpdating parameters ...\n')
            time.sleep(0.2)
            ser.write(packet)
            queryType = "pUpdate"
            waitForPacket = True

        case '3':
            packet = create_packet("FIR", [0, fwInfoStruct.sizeof()])
            ser.write(packet)
            queryType = "readInfo"
            waitForPacket = True

        case '4':
            print('Sending reboot command...\n')
            packet = create_packet("RST", [])
            queryType = "reset"
            ser.write(packet)
            waitForPacket = True

        case '5':
            print('Sending lock command...\n')
            packet = create_packet("LOCK", [])
            queryType = "serialLock"
            ser.write(packet)
            waitForPacket = True

        case '6':
            print('Unlocking serial line...\n')
            packet = create_packet("UNLOCK", [])
            queryType = "serialFree"
            ser.write(packet)
            waitForPacket = True

        case '7':
            print('Sending diag read command...\n')
            packet = create_packet("DIAG", [])
            queryType = "readDiag"
            ser.write(packet)
            waitForPacket = True

        case '0':
            print('Aborting ...')
            time.sleep(0.5)
            break

        case _:
            print('[ERROR] Invalid input')
            time.sleep(0.5)
        
    if waitForPacket:
        ser.timeout = 0.2
        prx = ser.read(1024)
        if len(prx) != 0:
            waitForPacket = False
            protocol_state_machine(prx, queryType)
        else:
            print("\n[TIMEOUT] No data received\n")
