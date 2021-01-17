/* 
Author: chinesebear
Email: swubear@163.com
Website: http://chinesebear.github.io
Date: 2020/11/3
Description: DSTP, deepvm serial transfer protocol
            PC ------serial--------> IoT device
            PC <------serial-------- IoT device
*/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "deep_common.h"
#include "dstp.h"
#define RING_BUF_SIZE 512
#define DSTP_DUMMY 0xFF
#define CMD_STR_LEN (120)

static unsigned char RingDataBuffer[RING_BUF_SIZE] = {0};
static volatile int DataInIndex = 0;
static volatile int DataOutIndex = 0;
static volatile int ProcessMode = DSTP_ASCII_MODE;
static volatile int ProcessState = DSTP_FRAME_HEAD; /* only for DSTP_FRAME_MODE */
static dstp_frame_t dstp = {0};

static unsigned char get_dstp_sum (dstp_frame_t *frame) {
    if (frame == NULL) {
        return 0xFF;
    }
    unsigned char ret = 0;
    unsigned int sum = 0;
    sum += frame->head[0];
    sum += frame->head[1];
    sum += frame->cmd;
    sum += ((frame->len & 0xFF00) >> 8);
    sum += (frame->len & 0xFF);
    if (frame->len != 0 && frame->payload != NULL) {
        for (int i = 0; i < frame->len; i++) {
            sum += frame->payload[i];
        }
    }
    sum += frame->tail[0];
    sum += frame->tail[1];
    ret = sum & 0xff;
    return ret;
}

static void ring_buf_clean (void) {
    DataInIndex = 0;
    DataOutIndex = 0;
    memset (RingDataBuffer, 0x00, RING_BUF_SIZE);
}

static bool ring_buf_empty (void) {
    if (DataInIndex == DataOutIndex) {
        return true;
    }
    return false;
}

static void ring_buf_datain(unsigned char data) {
    RingDataBuffer[DataInIndex] = data;
    if (DataInIndex >= (RING_BUF_SIZE - 1)) {
        DataInIndex = 0;
    } else {
        DataInIndex++;
    }
}

static unsigned char ring_buf_dataout (void) {
    unsigned char data = DSTP_DUMMY;
    if (ring_buf_empty()) {
        return DSTP_DUMMY;
    }
    data = RingDataBuffer[DataOutIndex];
    if (DataOutIndex >= (RING_BUF_SIZE - 1)) {
        DataOutIndex = 0;
    } else {
        DataOutIndex++;
    }
    return data;
}

static int get_process_mode (void) {
    return ProcessMode;
}

static void set_process_mode (int mode) {
    if (mode != DSTP_ASCII_MODE && mode != DSTP_FRAME_MODE) {
        return;
    }
    ProcessMode = mode;
}

static int get_process_state (void) {
    return ProcessState;
}

static void set_process_state (int state) {
    if (state > DSTP_STATE_END || state < DSTP_STATE_START) {
        return;
    }
    ProcessState = state;
}

static void reset_process_state (void) {
    set_process_state (DSTP_FRAME_HEAD);
    if (dstp.payload != NULL) {
        deep_free(dstp.payload);
    }
    memset ((unsigned char *) &dstp, 0x00, sizeof(dstp));
}

static int process_read_data (unsigned char *data, int len, int timeout_tick) {
    if (data == NULL) {
        return DEEP_FAIL;
    }
    for (int i = 0; i < len; i++) {
        int time = 0;
        while (ring_buf_empty() && timeout_tick > 0) {
            vTaskDelay(1);
            time++;
            if (time > timeout_tick) {
                return DEEP_TIMEOUT;
            }
        }
        data[i] = ring_buf_dataout();
    }
    return DEEP_OK;
}

static void Process_send_frame(unsigned char cmd, unsigned char *payload, int len) {
    dstp_frame_t frame = {0};
    frame.head[0] = DSTP_MAGIC_HEAD0;
    frame.head[1] = DSTP_MAGIC_HEAD1;
    frame.cmd = cmd;
    frame.len = len;
    frame.payload = payload;
    frame.tail[0] = DSTP_MAGIC_TAIL0;
    frame.tail[1] = DSTP_MAGIC_TAIL1;
    frame.sum = get_dstp_sum (&frame);
    debug ("frame fill done\r\n");
    dump ("frame", (unsigned char *) &frame, sizeof (frame));
    // todo: esp32 send serial data
    // \0 is special byte for serial assistant, it will make serial windows stop displaying
    // so it is better that converse frame bytes to ascii without \0, eg 0x00-> 0 0, 0xFF-> 0 F
    deep_printf ("[pc->iot dev]frame:");
    deep_printf ("%02x%02x", frame.head[0], frame.head[1]);
    deep_printf ("%02x", frame.cmd);
    char data[2] = {0};
    data[0] = (((frame.len) >> 8) & 0xFF);
    data[1] = ((frame.len) & 0xFF);;
    deep_printf ("%02x%02x", data[0], data[1]);
    for (int i= 0; i < frame.len; i++) {
        deep_printf ("%02X", frame.payload[i]);
    }
    deep_printf ("%02x%02x", frame.tail[0], frame.tail[1]);
    deep_printf ("%02x", frame.sum);
    deep_printf ("\r\n");
    reset_process_state ();
}

static void process_send_ack (void) {
    debug ("send ack frame\r\n");
    Process_send_frame (DSTP_CMD_ACK, (unsigned char *) "ACK", 4);
    debug ("send ack frame done\r\n");
}

static void process_head_handle (void) {
    unsigned char data = 0;
    int ret = process_read_data (&data, 1, 0);
    if (ret != DEEP_OK) {
        return;
    }
    debug ("head0=0x%02x\r\n",data);
    if (data != DSTP_MAGIC_HEAD0) {
        reset_process_state ();
        return;
    }
    ret = process_read_data (&data, 1, 0);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    debug ("head1=0x%02x\r\n",data);
    if (data != DSTP_MAGIC_HEAD1) {
        reset_process_state ();
        return;
    }
    dstp.head[0] = DSTP_MAGIC_HEAD0;
    dstp.head[1] = DSTP_MAGIC_HEAD1;
    set_process_state (DSTP_FRAME_CMD);
}

static void process_cmd_handle (void) {
    unsigned char data = 0;
    int ret = process_read_data (&data, 1, 100);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    debug ("cmd=0x%02x\r\n", data);
    dstp.cmd = data;
    set_process_state (DSTP_FRAME_LEN);
}

static void process_len_handle (void){
    unsigned char data[2] = {0};
    int ret = process_read_data (data, 2, 10);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    dstp.len = data[0] * 256 + data[1];
    set_process_state (DSTP_FRAME_PAYLOAD);
    debug ("len=%d\r\n", dstp.len);
}

static void process_payload_handle (void) {
    unsigned char *data = deep_malloc (dstp.len);
    if (data == NULL) {
        reset_process_state ();
        return;
    }
    int ret = process_read_data (data, dstp.len, 100);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    dstp.payload = data;
    set_process_state (DSTP_FRAME_TAIL);
    dump ("payload", data, dstp.len);
}

static void process_tail_handle (void) {
    unsigned char data = 0;
    int ret = process_read_data (&data, 1, 0);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    debug ("tail0=0x%02X\r\n", data);
    if (data != DSTP_MAGIC_TAIL0) {
        reset_process_state ();
        return;
    }
    ret = process_read_data (&data, 1, 0);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    debug ("tail1=0x%02X\r\n", data);
    if (data != DSTP_MAGIC_TAIL1) {
        reset_process_state ();
        return;
    }
    dstp.tail[0] = DSTP_MAGIC_TAIL0;
    dstp.tail[1] = DSTP_MAGIC_TAIL1;
    set_process_state (DSTP_FRAME_SUM);
}

static void process_sum_handle (void) {
    unsigned char data = 0;
    int ret = process_read_data (&data, 1, 0);
    if (ret != DEEP_OK) {
        reset_process_state ();
        return;
    }
    dstp.sum = data;
    unsigned char sum = get_dstp_sum(&dstp);
    debug ("sum=0x%02X\r\n",sum);
    if (sum != dstp.sum) {
        debug ("DSTP frame sum error,sum=0x%02X,sum\'=0x%02X\r\n", dstp.sum, sum);
        reset_process_state ();
        return;
    }
    /* DSTP frame done */
    switch (dstp.cmd) {
        case DSTP_CMD_TRANS_CMD:
            // todo: hanle cmd
            break;
        case DSTP_CMD_FILE_PARAM:
            // todo: handle file parameters
            break;
        case DSTP_CMD_FILE_PACKET:
            break;
        case DSTP_CMD_CUSTOMIZE:
            break;
        default:
            debug ("Wrong DSTP command: 0x%02X\r\n", dstp.cmd);
            break;
    }
    /* DSTP cmd handle done */
    process_send_ack ();
}

static void process_ascii_mode_with_repl (void) {
    char buf[CMD_STR_LEN] = {0};
    int i = 0;
    deep_printf ("deeplang prompt> ");
    while (1) {
        if (ring_buf_empty()) {
            vTaskDelay (1);
        }
        char ch = ring_buf_dataout ();
        if (ch == '\r' || ch == '\n') {
            break;
        }
        deep_printf ("%c", ch);
        buf[i++] = ch;
    }
    deep_printf ("\r\n");
    /* check buildin function and run repl */
    if (memcmp (":help", buf, strlen (":help")) == 0) {
        deep_printf (":help      help info\r\n");
        deep_printf (":exit      exit repl\r\n");
        deep_printf (":version   deeplang version\r\n");
        deep_printf (":memstat   memory status info\r\n");
        deep_printf (":mode      dstp mode\r\n");
    } else if (memcmp (":exit", buf, strlen (":exit")) == 0) {
        set_process_mode (DSTP_FRAME_MODE);
        set_process_state (DSTP_FRAME_HEAD);
        deep_printf ("exit repl\r\n");
    } else if (memcmp (":version", buf, strlen (":version")) == 0) {
        deep_printf ("deeplang v0.1\r\n");
    } else if (memcmp (":memstat", buf, strlen (":memstat")) == 0) {
        deep_printf ("Total 100KB, Left 10KB\r\n");
    } else if (memcmp (":mode", buf, strlen (":mode")) == 0) {
        deep_printf ("ascii mode\r\n");
    } else {
        deep_printf ("deep_eval (\"%s\")\r\n", buf);
    }
}

void deep_dstp_datain (unsigned char data) {
    return ring_buf_datain (data);
}

void deep_dstp_process (void) {
    if (ring_buf_empty ()) {
        vTaskDelay (5);
        return;
    }
    if (get_process_mode() == DSTP_ASCII_MODE) {
        process_ascii_mode_with_repl();
        return;
    }
    int state = get_process_state();
    debug ("state=0x%02x\r\n", state);
    switch (state) {
        case DSTP_FRAME_HEAD:
            process_head_handle ();
            break;
        case DSTP_FRAME_CMD:
            process_cmd_handle ();
            break;
        case DSTP_FRAME_LEN:
            process_len_handle ();
            break;
        case DSTP_FRAME_PAYLOAD:
            process_payload_handle ();
            break;
        case DSTP_FRAME_TAIL:
            process_tail_handle ();
            break;
        case DSTP_FRAME_SUM:
            process_sum_handle ();
            break;
        default:
            debug ("Wrong DSTP state:0x%02X\r\n", state);
            break;
    }
}

