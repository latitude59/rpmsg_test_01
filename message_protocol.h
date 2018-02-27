#ifndef __MESSAGE_PROTOCOL_H__
#define __MESSAGE_PROTOCOL_H__

#include <stdint.h>

typedef struct __attribute__((__packed__))
{
    uint8_t start_byte;   // 0x55
    uint8_t ack_req;
    uint16_t msg_count;
    uint16_t data_len;
} packet_hdr_t;


typedef struct
{
    uint32_t cmd:4;
    uint32_t dev_addr:8;
    uint32_t ack:1;
    uint32_t reserved:19;
    uint32_t timestamp;
    uint32_t value;
} message_t;


#endif  // __MESSAGE_PROTOCOL_H__

