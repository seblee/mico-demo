/**
 ******************************************************************************
 * @file    hello_world.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   First MiCO application to say hello world!
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include "mico.h"

#define os_helloworld_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)

static void _pwm_cmd_handler(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
  mico_pwm_t id = (mico_pwm_t)atoi(argv[1]);
  uint32_t freq = (uint32_t)atoi(argv[2]);
  float duty = (float)atof(argv[3]);

  printf("PWM ID:%u, freq:%dHz, duty:%8.4f%\r\n", id, freq, duty);

  MicoPwmStop(id);
  MicoPwmInitialize(id, freq, duty);
  MicoPwmStart(id);
}

static void _adc_cmd_handler(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
  static uint8_t _inited = 0;
  if(_inited == 0){
    MicoAdcInitialize(MICO_ADC_1, 1);
    _inited = 1;
  }

  uint16_t raw_value;
  MicoAdcTakeSample(MICO_ADC_1, &raw_value);
  printf("ADC raw value = 0x%04x\r\n", raw_value);
}

static const struct cli_command _test_cmds[] = {
  {"pwm", "<id> <freq> <duty>", _pwm_cmd_handler},
  {"adc", "get ADC raw value", _adc_cmd_handler},
};

int application_start( void )
{
  /* Start MiCO system functions according to mico_config.h*/
  mico_system_init( mico_system_context_init( 0 ) );

  cli_register_commands(&_test_cmds, sizeof(_test_cmds)/sizeof(struct cli_command));
}


