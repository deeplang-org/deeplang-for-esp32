/* 
Author: chinesebear
Email: swubear@163.com
Website: http://chinesebear.github.io
Date: 2020/11/10
Description: deep common functions
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "deep_common.h"

#define LINE_MAX (120)
void deep_send_buf (const char * buffer, int len) {
    return uart_write_bytes(UART_NUM_0, buffer, len);
}

void deep_printf (const char *format, ...)
{
    char buffer[LINE_MAX] = {0};
	va_list arg;
	va_start(arg, format);
    int len = vsnprintf(buffer, LINE_MAX, format, arg); 
	va_end(arg);
    uart_write_bytes(UART_NUM_0, (const char *) buffer, len);
}

void log_printf (const char* pFileName, unsigned int uiLine, const char* pFnucName, char *LogFmtBuf, ...)
{
	va_list args;
	if(pFileName == NULL || uiLine == 0 || LogFmtBuf == NULL)
	{
		return ;
	}
    char logbuf[256];
	memset (logbuf,'\0',256);
	sprintf (logbuf,"%s:%d, %s(), ", pFileName, uiLine, pFnucName);	
	deep_printf ("%s",logbuf);
	memset (logbuf,'\0',256);
	va_start (args, LogFmtBuf);
	vsnprintf (logbuf, 256, LogFmtBuf, args); 
	va_end (args);
	deep_printf ("%s\r\n",logbuf);
}

void log_data(const char *pFileName, unsigned int uiLine, const char* pFnucName, const char *pcStr,unsigned char *pucBuf,unsigned int usLen)
{
    unsigned int i;
    unsigned char acTmp[17];
    unsigned char *p;
    unsigned char *pucAddr = pucBuf;

    if(pcStr)
    {
        log_printf (pFileName, uiLine, pFnucName, "[%s]: length = %d (0x%X)\r\n",pcStr, usLen, usLen);
    }
    if(usLen == 0)
    {
        return;
    }
    p = acTmp;
    deep_printf ("    %p  ", pucAddr);
    for(i=0;i<usLen;i++)
    {

        deep_printf ("%02X ",pucBuf[i]);
        if((pucBuf[i] >= 0x20) && (pucBuf[i] < 0x7F))
        {
            *p++ = pucBuf[i];
        }
        else
        {
            *p++ = '.';
        }
        if((i+1)%16==0)
        {
            *p++ = 0;
            deep_printf ("        | %s", acTmp);
            p = acTmp;

            deep_printf ("\r\n");

            if((i+1) < usLen)
            {
                pucAddr += 16;
                deep_printf ("    %p  ", pucAddr);
            }
        }
        else if((i+1)%8==0)
        {
            deep_printf ("- ");
        }
    }
    if(usLen%16!=0)
    {
        for(i=usLen%16;i<16;i++)
        {
            deep_printf ("   ");
            if(((i+1)%8==0) && ((i+1)%16!=0))
            {
                deep_printf ("- ");
            }
        }
        *p++ = 0;
        deep_printf ("        | %s", acTmp);
        deep_printf ("\r\n");
    }
    deep_printf ("\r\n");
}
void * deep_malloc (int n) {
    if (n < 0) {
        return NULL;
    }
    void *p = malloc (n);
    if (p == NULL) {
        return NULL;
    }
    return p;
}

void deep_free (void *p) {
    if (p == NULL) {
        return;
    }
    free(p);
    p = NULL;
}