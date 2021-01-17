/* 
Author: chinesebear
Email: swubear@163.com
Website: http://chinesebear.github.io
Date: 2020/11/3
Description: DSTP, deepvm serial transfer protocol
             PC ------serial--------> IoT device
             PC <------serial-------- IoT device
             fe 5a 03 00 0a 00 f8 2e 2f 66 69 62 2e 64 00 fa e3 5a
*/

#ifndef _DSTP_H
#define _DSTP_H

#define DSTP_ASCII_MODE  0xA1   /* for repl */
#define DSTP_FRAME_MODE  0xA2   /* for dp file downloading */

#define DSTP_ASCII_TAIL '\n'
#define DSTP_MAGIC_HEAD0 0xFE
#define DSTP_MAGIC_HEAD1 0x5A
#define DSTP_MAGIC_TAIL0 0xFA
#define DSTP_MAGIC_TAIL1 0xE3

#define DSTP_STATE_START     0x0F
#define DSTP_FRAME_HEAD      0x10
#define DSTP_FRAME_CMD       0x11
#define DSTP_FRAME_LEN       0x12
#define DSTP_FRAME_PAYLOAD   0x13
#define DSTP_FRAME_TAIL      0x14
#define DSTP_FRAME_SUM       0x15
#define DSTP_STATE_END       0x16

#define DSTP_CMD_TRANS_CMD    0x01
#define DSTP_CMD_ACK          0x02
#define DSTP_CMD_FILE_PARAM   0x03
#define DSTP_CMD_FILE_PACKET  0x04
#define DSTP_CMD_CUSTOMIZE    0x05

#define DEEP_OK       0
#define DEEP_FAIL     -1
#define DEEP_TIMEOUT  -2

typedef struct dstp_frame {
    unsigned char head[2];
    unsigned char cmd;
    int len;
    unsigned char *payload;
    unsigned char tail[2];
    unsigned char sum;
} dstp_frame_t;

void deep_dstp_datain (unsigned char data);
void deep_dstp_process (void);


#endif
