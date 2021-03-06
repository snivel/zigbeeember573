// Copyright 2016 Silicon Laboratories, Inc.                                *80*

// this file contains all the common includes for clusters in the util
#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#define FALSE_UINT8_T 0
#define TRUE_UINT8_T  1
#define TOKEN_DATA_OFFSET 1
#define TOKEN_DATA_LENGTH 0
#define STRING_TERMINATOR 0


void emAfPluginConfigurationServerReadTokenDataFromCreator(uint16_t creator, 
                                                           uint8_t *data);

bool emAfPluginConfigurationServerLocked(void)
{
  uint8_t lockTokenWrite;

  halCommonGetToken((int8u *) (&lockTokenWrite), TOKEN_OTA_CONFIG_LOCK);

  if (lockTokenWrite == FALSE_UINT8_T) {
    return false;
  } else {
    return true;
  }
}

void emberAfOtaConfigurationClusterServerInitCallback(int8u endpoint)
{
  uint8_t returnData[OTA_CONFIG_MAX_TOKEN_LENGTH + 1];
  // configure model name here.  
  MEMSET(returnData, 0, OTA_CONFIG_MAX_TOKEN_LENGTH + 1);

  halCommonGetToken((returnData+TOKEN_DATA_OFFSET), 
                    TOKEN_OTA_CONFIG_MODEL_NAME);

  returnData[0] = strlen(returnData+TOKEN_DATA_OFFSET);

  if (returnData[1] != 0xff && returnData[0] != 0) {
    emberAfWriteServerAttribute(endpoint,
                                ZCL_BASIC_CLUSTER_ID,
                                ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID,
                                returnData,
                                ZCL_CHAR_STRING_ATTRIBUTE_TYPE);
  }

  halCommonGetToken((returnData+TOKEN_DATA_OFFSET), 
                    TOKEN_OTA_CONFIG_MANUFACTURER_NAME);

  returnData[0] = strlen(returnData+TOKEN_DATA_OFFSET);

  if (returnData[1] != 0xff && returnData[0] != 0) {
    emberAfWriteServerAttribute(endpoint,
                                ZCL_BASIC_CLUSTER_ID,
                                ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID,
                                returnData,
                                ZCL_CHAR_STRING_ATTRIBUTE_TYPE);
  }

  halCommonGetToken(returnData,
                    TOKEN_OTA_CONFIG_HW_VERSION);

  if (returnData[0] != 0xff) {
    emberAfWriteServerAttribute(endpoint,
                                ZCL_BASIC_CLUSTER_ID,
                                ZCL_HW_VERSION_ATTRIBUTE_ID,
                                returnData,
                                ZCL_INT8U_ATTRIBUTE_TYPE);
  }
}


bool emberAfOtaConfigurationClusterLockTokensCallback(void)
{
  uint8_t lockTokenWrite = (uint8_t) TRUE_UINT8_T;

  halCommonSetToken(TOKEN_OTA_CONFIG_LOCK, (int8u *) (&lockTokenWrite));

  return true;

}

/** @brief Configuration Cluster Cluster Read Tokens
 *
 * This function will read the data specified by the 16-bit token value and 
 * generate the read response command.
 *
 * @param token   Ver.: always
 */
bool emberAfOtaConfigurationClusterReadTokensCallback(int16u token)
{
  uint8_t returnData[OTA_CONFIG_MAX_TOKEN_LENGTH + 1];
  emAfPluginConfigurationServerReadTokenDataFromCreator(token, returnData);

  // First, set up the outgoing command
  emberAfFillCommandOtaConfigurationClusterReturnToken(token, returnData);

  // now send to the current incoming device
  emberAfSendResponse();
  
  return true;
}

bool emberAfOtaConfigurationClusterSetTokenCallback(int16u token,
                                                       int8u* data)
{
  if (emAfPluginConfigurationServerLocked()) {
    emberAfCorePrintln("Locked");
    return true;
  } else {
    emberAfCorePrintln("Unlocked");
  }
  
  switch (token) {
  case CREATOR_OTA_CONFIG_TX_POWER:
    halCommonSetToken(TOKEN_OTA_CONFIG_TX_POWER, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_OTA_CONFIG_TX_POWER25:
    halCommonSetToken(TOKEN_OTA_CONFIG_TX_POWER25, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_OTA_CONFIG_TX_POWER26:
    halCommonSetToken(TOKEN_OTA_CONFIG_TX_POWER26, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_OTA_CONFIG_MODEL_NAME:
    if (data[TOKEN_DATA_LENGTH] > OTA_CONFIG_MAX_TOKEN_LENGTH) {
      data[TOKEN_DATA_LENGTH] = OTA_CONFIG_MAX_TOKEN_LENGTH;
    }
    data[data[TOKEN_DATA_LENGTH]+1] = STRING_TERMINATOR;
    halCommonSetToken(TOKEN_OTA_CONFIG_MODEL_NAME, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_OTA_CONFIG_MANUFACTURER_NAME:
    if (data[TOKEN_DATA_LENGTH] > OTA_CONFIG_MAX_TOKEN_LENGTH) {
      data[TOKEN_DATA_LENGTH] = OTA_CONFIG_MAX_TOKEN_LENGTH;
    }
    data[data[TOKEN_DATA_LENGTH]+1] = STRING_TERMINATOR;
    halCommonSetToken(TOKEN_OTA_CONFIG_MANUFACTURER_NAME, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_OTA_CONFIG_HW_VERSION:
    halCommonSetToken(TOKEN_OTA_CONFIG_HW_VERSION, (data + TOKEN_DATA_OFFSET));
    break;

#if defined(EMBER_AF_PLUGIN_LED_RGB_PWM) || defined(EMBER_AF_PLUGIN_LED_TEMP_PWM) || defined(EMBER_AF_PLUGIN_LED_DIM_PWM)
  case CREATOR_BULB_PWM_FREQUENCY_HZ:
    halCommonSetToken(TOKEN_BULB_PWM_FREQUENCY_HZ, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_BULB_PWM_MIN_ON_US:
    halCommonSetToken(TOKEN_BULB_PWM_MIN_ON_US, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_BULB_PWM_MAX_ON_US:
    halCommonSetToken(TOKEN_BULB_PWM_MAX_ON_US, (data + TOKEN_DATA_OFFSET));
    break;
#endif
#ifdef EMBER_AF_PLUGIN_BULB_UI
  case CREATOR_BULB_UI_MIN_ON_TIME:
    halCommonSetToken(TOKEN_BULB_UI_MIN_ON_TIME, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_BULB_UI_TIMEOUT:
    halCommonSetToken(TOKEN_BULB_UI_TIMEOUT, (data + TOKEN_DATA_OFFSET));
    break;
  case CREATOR_BULB_UI_POWER_UP_BEHAVIOR:
    halCommonSetToken(TOKEN_BULB_UI_POWER_UP_BEHAVIOR, (data + TOKEN_DATA_OFFSET));
    break;
#endif
  default:
    emberAfCorePrintln("Configuration Server:  Unsupported Token %2x",
                       token);
    break;
  }

  return true;
}


bool emberAfOtaConfigurationClusterUnlockTokensCallback(int8u* data)
{
  uint8_t lockTokenWrite = (uint8_t) FALSE_UINT8_T;
  uint8_t *eui64;
  uint8_t hashData[EUI64_SIZE];
  uint8_t i;
  
  eui64 = emberGetEui64();

  if (data[0] != EUI64_SIZE) {
    return false;
  }

  // simple hash of the EUI64;
  for (i=0; i<EUI64_SIZE; i++) {
    hashData[EUI64_SIZE-i-1] = data[i+1] + 0x80;
  }

  if (MEMCOMPARE(eui64,hashData,8)==0) {
    halCommonSetToken(TOKEN_OTA_CONFIG_LOCK, (int8u *) (&lockTokenWrite));
  }

  return true;
}

void emAfPluginConfigurationServerReadTokenDataFromCreator(uint16_t creator, 
                                                           uint8_t *data)
{
  uint8_t returnData[OTA_CONFIG_MAX_TOKEN_LENGTH + 1];
  switch(creator) {
  case CREATOR_OTA_CONFIG_TX_POWER:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_OTA_CONFIG_TX_POWER);
    break;
  case CREATOR_OTA_CONFIG_TX_POWER25:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_OTA_CONFIG_TX_POWER25);
    break;
  case CREATOR_OTA_CONFIG_TX_POWER26:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_OTA_CONFIG_TX_POWER26);
    break;
  case CREATOR_OTA_CONFIG_MODEL_NAME:
    returnData[data[0]+1] = 0;
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_OTA_CONFIG_MODEL_NAME);
    break;
  case CREATOR_OTA_CONFIG_MANUFACTURER_NAME:
    returnData[data[0]+1] = 0;
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_OTA_CONFIG_MANUFACTURER_NAME);
    break;
  case CREATOR_OTA_CONFIG_HW_VERSION:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_OTA_CONFIG_HW_VERSION);
    break;

#if defined(EMBER_AF_PLUGIN_LED_RGB_PWM) || defined(EMBER_AF_PLUGIN_LED_TEMP_PWM) || defined(EMBER_AF_PLUGIN_LED_DIM_PWM)
  case CREATOR_BULB_PWM_FREQUENCY_HZ:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_BULB_PWM_FREQUENCY_HZ);
    break;
  case CREATOR_BULB_PWM_MIN_ON_US:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_BULB_PWM_MIN_ON_US);
    break;
  case CREATOR_BULB_PWM_MAX_ON_US:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_BULB_PWM_MAX_ON_US);
    break;
#endif
#ifdef EMBER_AF_PLUGIN_BULB_UI
  case CREATOR_BULB_UI_MIN_ON_TIME:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_BULB_UI_MIN_ON_TIME);
    break;
  case CREATOR_BULB_UI_TIMEOUT:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_BULB_UI_TIMEOUT);
    break;
  case CREATOR_BULB_UI_POWER_UP_BEHAVIOR:
    halCommonGetToken((uint8_t *) (data + TOKEN_DATA_OFFSET), 
                      TOKEN_BULB_UI_POWER_UP_BEHAVIOR);
    break;
#endif
  default:
    break;
  }
}

// callbacks for reading configuration values.
/** @brief Get Radio Power For Channel
 *
 * This callback is called by the framework when it is setting the radio power
 * during the discovery process. The framework will set the radio power
 * depending on what is returned by this callback.
 *
 * @param channel   Ver.: always
 */
int8_t emberAfPluginNetworkFindGetRadioPowerForChannelCallback(uint8_t channel)
{
  int8_t powerDefault, power = OTA_CONFIG_INVALID_TX_POWER;

  halCommonGetToken((uint8_t *) &powerDefault, TOKEN_OTA_CONFIG_TX_POWER);

  if (channel == 25) {
    halCommonGetToken((uint8_t *) &power, TOKEN_OTA_CONFIG_TX_POWER25);
  } else if (channel == 26) {
    halCommonGetToken((uint8_t *) &power, TOKEN_OTA_CONFIG_TX_POWER26);
  }

  if (power == OTA_CONFIG_INVALID_TX_POWER) {
    if (powerDefault == OTA_CONFIG_INVALID_TX_POWER) {
      emberAfCorePrintln("Config TX Power:  Default %d %d", channel, 
                         EMBER_AF_PLUGIN_NETWORK_FIND_RADIO_TX_POWER);
      return EMBER_AF_PLUGIN_NETWORK_FIND_RADIO_TX_POWER;
    } else {
      emberAfCorePrintln("Config TX Power:  All Chan %d %d", channel, powerDefault);
      return powerDefault;
    }
  } else {
    emberAfCorePrintln("Config TX Power:  special %d %d", channel, power);
    return power;
  }
}

uint16_t emberAfPluginConfigurationServerReadMinOnTimeUs(void)
{
  uint16_t time;

  halCommonGetToken((uint8_t *) &time, TOKEN_BULB_PWM_MIN_ON_US);

  return time;
}

uint16_t emberAfPluginConfigurationServerReadMaxOnTimeUs(void)
{
  uint16_t time;

  halCommonGetToken((uint8_t *) &time, TOKEN_BULB_PWM_MAX_ON_US);

  return time;
}

uint16_t halBulbPwmDriverFrequencyCallback(void)
{
  uint16_t frequency;

  halCommonGetToken((uint8_t *) &frequency, TOKEN_BULB_PWM_FREQUENCY_HZ);

  // Note:  default value of token is the default value of the PWM frequency.
  return frequency;
}
