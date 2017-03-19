/**
 ******************************************************************************
 * @file    mico_app_define
 * @author  William Xu
 * @version V1.0.0
 * @date    05-May-2014
 * @brief   Parameters for HomeKit demonstration application.
 ******************************************************************************
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of MXCHIP Corporation.
 ******************************************************************************
 */

#pragma once

#include "mico.h"
#include "HomeKit.h"

#ifdef __cplusplus
extern "C" {
#endif

/*User provided configurations*/
#define CONFIGURATION_VERSION               0x20000002 // if default configuration is changed, update this number


/*Application's configuration stores in flash, and loaded to ram when system boots up*/
typedef struct
{
  uint32_t          configDataVer;
  
  /*Homekit*/
  bool              paired; 
  uint8_t           LTSK[32]; 
  uint8_t           LTPK[32]; 
  int               config_number; /**< Add this number when HomeKit config is changed to enable a re-discover on controller */
  pair_list_in_flash_t pairList;

} application_config_t;

typedef struct _lightbulb_service_t {

  bool              on;
  bool              on_new;
  HkStatus          on_status;

  int               brightness;
  int               brightness_new;
  HkStatus          brightness_status;

  float             hue;
  float             hue_new;
  HkStatus          hue_status;

  float             saturation;
  float             saturation_new;
  HkStatus          saturation_status;

} lightbulb_service;

typedef struct _fan_service_t {
  bool              on;
  bool              on_new;
  HkStatus          on_status;
} fan_service;

typedef struct _temp_service_t {
  float             temp;
  HkStatus          temp_status;
} temp_service;

typedef struct _humidity_service_t {
  float             humidity;
  HkStatus          humidity_status;
} humidity_service;

typedef struct _occupancy_service_t {
  int             occupancy;
  HkStatus        occupancy_status;
} occupancy_service;

typedef struct _app_context_t
{
  /*Flash content*/
  application_config_t*     appConfig;

  /*Running status*/
  lightbulb_service         lightbulb;
  fan_service               fan;
  temp_service              temp;
  humidity_service          humidity;
  occupancy_service         occupancy;
} app_context_t;

#ifdef __cplusplus
} /*extern "C" */
#endif









