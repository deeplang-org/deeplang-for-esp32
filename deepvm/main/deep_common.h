/* 
Author: chinesebear
Email: swubear@163.com
Website: http://chinesebear.github.io
Date: 2020/11/10
Description: deep common functions
*/

#ifndef _DEEP_COMMON_H
#define _DEEP_COMMON_H

void deep_printf (const char *format, ...);
void log_printf (const char* pFileName, unsigned int uiLine, const char* pFuncName,char *LogFmtBuf, ...);
void log_data(const char *pFileName, unsigned int uiLine, const char* pFuncName, const char *pcStr,unsigned char *pucBuf,unsigned int usLen);
#define debug(...) 							log_printf(__FILE__, __LINE__,__FUNCTION__,__VA_ARGS__)
#define dump(pcStr,pucBuf,usLen)			log_data(__FILE__, __LINE__,__FUNCTION__,pcStr,pucBuf,usLen)

void deep_free (void *p);
void * deep_malloc (int n);

#endif