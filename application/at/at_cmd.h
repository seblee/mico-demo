
#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#include "common.h"
#include "MICO.h"
#include "stdarg.h"

/* AT command error number */
typedef enum {
  AT_SUCCESS   		= 0,
  INVALID_FORMAT        = -1,
  INVALID_COMMAND       = -2,
  INVALID_OPERATOR      = -3,
  INVALID_PARAMETER     = -4,
  INVALID_OPERATION     = -5,
  INVALID_SOCKET        = -6,
} ERROR_TYPE;

typedef struct{
  char requestName[20];
  char *help;
  void (*responseFunction)(int argc ,char *argv[]);
}_S_AT_CMD;

/****************************************************************/
/***************** AT Command Process function *****************/
/****************************************************************/

/* Check the input parameters string is digit */
int str_is_digit(const char * str, uint32_t max_num, uint32_t min_num);

/* check the validable of the IP string  */
int is_valid_ip(const char *ip);

/* 
* Decompose a string into a set of strings 
**/
char *strsep(char **stringp, const char *delim);

/* compare string between line with prefix.
**/
int strStartsWith(const char *line, const char *prefix);

int replaceCRCF(char *src, char *drc);

int at_printf(const char *msg, ...);

OSStatus AtUartCommandProcess(uint8_t *inBuf, int inLen, mico_Context_t * const inContext);

OSStatus mico_at_mode ( void *app_context_c );

#endif

