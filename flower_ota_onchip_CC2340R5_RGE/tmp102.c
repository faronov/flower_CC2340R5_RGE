/*
  TMP102 Temperature sensor driver for CC2340R5, CC2652
  Product: https://www.ti.com/product/TMP102
  Written by Koptyakov Sergey
  From source https://github.com/sparkfun/SparkFun_TMP102_Arduino_Library
*/

#ifdef TMP102
#include <tmp102.h>
#include <ti/log/Log.h>
#include "ti_drivers_config.h"
// Import I2C Driver definitions
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CLPF3.h>

#include <math.h>

//Variables
extern I2C_Handle i2cHandle;
static bool status = false;
static uint8_t _address; // Address of Temperature sensor (0x48,0x49,0x4A,0x4B)
static bool setOneShot = 0;

static uint8_t tmp102_readRegister(bool registerNumber);  // reads 1 byte of from register
static void tmp102_openPointerRegister(uint8_t pointerReg); // Changes the pointer register

bool tmp102_begin(uint8_t deviceAddress)
{
  _address = deviceAddress; // If provided, store the I2C address from user

//  _i2cPort->beginTransmission(_address);

//  uint8_t error = _i2cPort->endTransmission();

//  if (error == 0)
//  {
//    return true; // Device online!
//  }
//  else
//    return false; // Device not attached?
  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[1];
  writeBuffer[0] = CONFIG_REGISTER;

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 1;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      Log_printf(LogModule_Zigbee_App, Log_INFO, "Unsuccessful I2C transfer %d address %x", status, _address);
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
          Log_printf(LogModule_Zigbee_App, Log_INFO, "I2C target address not acknowledged %d", i2cTransaction.status);
          return false; // Device not attached?
      }
  }
  Log_printf(LogModule_Zigbee_App, Log_INFO, "Successful I2C transfer status %d address %x" , status, _address);
  return true; // Device online!
}

void tmp102_openPointerRegister(uint8_t pointerReg)
{
/*
  _i2cPort->beginTransmission(_address); // Connect to TMP102
  _i2cPort->write(pointerReg);           // Open specified register
  _i2cPort->endTransmission();           // Close communication with TMP102
*/
  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[1];
  writeBuffer[0] = pointerReg;

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 1;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

static uint8_t tmp102_readRegister(bool registerNumber)
{
  I2C_Transaction i2cTransaction = {0};
  uint8_t registerByte[2]; // We'll store the data from the registers here

  // Read current configuration register value
//  _i2cPort->requestFrom(_address, 2);   // Read two bytes from TMP102
//  registerByte[0] = (_i2cPort->read()); // Read first byte
//  registerByte[1] = (_i2cPort->read()); // Read second byte

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = NULL;
  i2cTransaction.writeCount = 0;
  i2cTransaction.readBuf = registerByte;
  i2cTransaction.readCount = 2;

  status = I2C_transfer(i2cHandle, &i2cTransaction);

  if (status == false) {
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK) {
          // I2C target address not acknowledged
      }
  }

  return registerByte[registerNumber];
}

float tmp102_readTempC(void)
{
  uint8_t registerByte[2]; // Store the data from the register here
  int16_t digitalTemp;     // Temperature stored in TMP102 register

  // Read Temperature
  // Change pointer address to temperature register (0)
  tmp102_openPointerRegister(TEMPERATURE_REGISTER);
  // Read from temperature register
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);

  if (registerByte[0] == 0xFF && registerByte[1] == 0xFF)
  {
    return NAN;
  }

  // Bit 0 of second byte will always be 0 in 12-bit readings and 1 in 13-bit
  if (registerByte[1] & 0x01) // 13 bit mode
  {
    // Combine bytes to create a signed int
    digitalTemp = ((registerByte[0]) << 5) | (registerByte[1] >> 3);
    // Temperature data can be + or -, if it should be negative,
    // convert 13 bit to 16 bit and use the 2s compliment.
    if (digitalTemp > 0xFFF)
    {
      digitalTemp |= 0xE000;
    }
  }
  else // 12 bit mode
  {
    // Combine bytes to create a signed int
    digitalTemp = ((registerByte[0]) << 4) | (registerByte[1] >> 4);
    // Temperature data can be + or -, if it should be negative,
    // convert 12 bit to 16 bit and use the 2s compliment.
    if (digitalTemp > 0x7FF)
    {
      digitalTemp |= 0xF000;
    }
  }
  // Convert digital reading to analog temperature (1-bit is equal to 0.0625 C)
  return digitalTemp * 0.0625;
}

float tmp102_readTempF(void)
{
  return tmp102_readTempC() * 9.0 / 5.0 + 32.0;
}

void tmp102_setConversionRate(uint8_t rate)
{
  uint8_t registerByte[2]; // Store the data from the register here
  rate = rate & 0x03;      // Make sure rate is not set higher than 3.

  // Change pointer address to configuration register (0x01)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);

  // Load new conversion rate
  registerByte[1] &= 0x3F;      // Clear CR0/1 (bit 6 and 7 of second byte)
  registerByte[1] |= rate << 6; // Shift in new conversion rate

  // Set configuration registers
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte[0]); // Write first byte
//  _i2cPort->write(registerByte[1]); // Write second byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[3];
  writeBuffer[0] = CONFIG_REGISTER;
  writeBuffer[1] = registerByte[0];
  writeBuffer[2] = registerByte[1];

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 3;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

void tmp102_setExtendedMode(bool mode)
{
  uint8_t registerByte[2]; // Store the data from the register here

  // Change pointer address to configuration register (0x01)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);

  // Load new value for extention mode
  registerByte[1] &= 0xEF;      // Clear EM (bit 4 of second byte)
  registerByte[1] |= mode << 4; // Shift in new exentended mode bit

  // Set configuration registers
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte[0]); // Write first byte
//  _i2cPort->write(registerByte[1]); // Write second byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[3];
  writeBuffer[0] = CONFIG_REGISTER;
  writeBuffer[1] = registerByte[0];
  writeBuffer[2] = registerByte[1];

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 3;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

void tmp102_sleep(void)
{
  uint8_t registerByte; // Store the data from the register here

  // Change pointer address to configuration register (0x01)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(0);

  registerByte |= 0x01; // Set SD (bit 0 of first byte)

  // Set configuration register
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte);    // Write first byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

    I2C_Transaction i2cTransaction = {0};
    uint8_t writeBuffer[2];
    writeBuffer[0] = CONFIG_REGISTER;
    writeBuffer[1] = registerByte;

    i2cTransaction.targetAddress = _address;
    i2cTransaction.writeBuf = writeBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    status = I2C_transfer(i2cHandle, &i2cTransaction);
    if (status == false)
    {
        // Unsuccessful I2C transfer
        if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
        {
            // I2C target address not acknowledged
        }
    }
}

void tmp102_wakeup(void)
{
  uint8_t registerByte; // Store the data from the register here

  // Change pointer address to configuration register (1)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(0);

  registerByte &= 0xFE; // Clear SD (bit 0 of first byte)

  // Set configuration registers
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte);    // Write first byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[2];
  writeBuffer[0] = CONFIG_REGISTER;
  writeBuffer[1] = registerByte;

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 2;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

void tmp102_setAlertPolarity(bool polarity)
{
  uint8_t registerByte; // Store the data from the register here

  // Change pointer address to configuration register (1)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(0);

  // Load new value for polarity
  registerByte &= 0xFB;          // Clear POL (bit 2 of registerByte)
  registerByte |= polarity << 2; // Shift in new POL bit

  // Set configuration register
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte);    // Write first byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[2];
  writeBuffer[0] = CONFIG_REGISTER;
  writeBuffer[1] = registerByte;

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 2;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

bool tmp102_alert(void)
{
  uint8_t registerByte; // Store the data from the register here

  // Change pointer address to configuration register (1)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(1);

  registerByte &= 0x20; // Clear everything but the alert bit (bit 5)
  return registerByte >> 5;
}

bool tmp102_oneShot(bool setOneShot)
{
  uint8_t registerByte; // Store the data from the register here

  // Read the first byte of the configuration register
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER);
//  _i2cPort->requestFrom(_address, 1);
//  registerByte = _i2cPort->read();

  // Change pointer address to configuration register (1)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(0);

  if (setOneShot) // Enable one-shot by writing a 1 to the OS bit of the configuration register
  {
    registerByte |= (1 << 7);

    // Set configuration register
//    _i2cPort->beginTransmission(_address);
//    _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//    _i2cPort->write(registerByte);    // Write first byte
//    _i2cPort->endTransmission();      // Close communication with TMP102

    I2C_Transaction i2cTransaction = {0};
    uint8_t writeBuffer[2];
    writeBuffer[0] = CONFIG_REGISTER;
    writeBuffer[1] = registerByte;

    i2cTransaction.targetAddress = _address;
    i2cTransaction.writeBuf = writeBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    status = I2C_transfer(i2cHandle, &i2cTransaction);
    if (status == false)
    {
        // Unsuccessful I2C transfer
        if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
        {
            // I2C target address not acknowledged
        }
    }

    return 0;
  }
  else // Return OS bit of configuration register (0-not ready, 1-conversion complete)
  {
    registerByte &= (1 << 7);
    return (registerByte >> 7);
  }
}

void tmp102_setLowTempC(float temperature)
{
  uint8_t registerByte[2]; // Store the data from the register here
  bool extendedMode;       // Store extended mode bit here 0:-55C to +128C, 1:-55C to +150C

  // Prevent temperature from exceeding 150C or -55C
  if (temperature > 150.0f)
  {
    temperature = 150.0f;
  }
  if (temperature < -55.0)
  {
    temperature = -55.0f;
  }

  // Check if temperature should be 12 or 13 bits
  tmp102_openPointerRegister(CONFIG_REGISTER); // Read configuration register settings

  // Read current configuration register value
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);
  extendedMode = (registerByte[1] & 0x10) >> 4; // 0 - temp data will be 12 bits
                                                // 1 - temp data will be 13 bits

  // Convert analog temperature to digital value
  temperature = temperature / 0.0625;

  // Split temperature into separate bytes
  if (extendedMode) // 13-bit mode
  {
    registerByte[0] = (int)temperature >> 5;
    registerByte[1] = (int)temperature << 3;
  }
  else // 12-bit mode
  {
    registerByte[0] = (int)(temperature) >> 4;
    registerByte[1] = (int)(temperature) << 4;
  }

  // Write to T_LOW Register
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(T_LOW_REGISTER);  // Point to T_LOW
//  _i2cPort->write(registerByte[0]); // Write first byte
//  _i2cPort->write(registerByte[1]); // Write second byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[3];
  writeBuffer[0] = T_LOW_REGISTER;
  writeBuffer[1] = registerByte[0];
  writeBuffer[2] = registerByte[1];

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 3;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

void tmp102_setHighTempC(float temperature)
{
  uint8_t registerByte[2]; // Store the data from the register here
  bool extendedMode;       // Store extended mode bit here 0:-55C to +128C, 1:-55C to +150C

  // Prevent temperature from exceeding 150C
  if (temperature > 150.0f)
  {
    temperature = 150.0f;
  }
  if (temperature < -55.0)
  {
    temperature = -55.0f;
  }

  // Check if temperature should be 12 or 13 bits
  tmp102_openPointerRegister(CONFIG_REGISTER); // Read configuration register settings

  // Read current configuration register value
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);
  extendedMode = (registerByte[1] & 0x10) >> 4; // 0 - temp data will be 12 bits
                                                // 1 - temp data will be 13 bits

  // Convert analog temperature to digital value
  temperature = temperature / 0.0625;

  // Split temperature into separate bytes
  if (extendedMode) // 13-bit mode
  {
    registerByte[0] = (int)temperature >> 5;
    registerByte[1] = (int)temperature << 3;
  }
  else // 12-bit mode
  {
    registerByte[0] = (int)temperature >> 4;
    registerByte[1] = (int)temperature << 4;
  }

  // Write to T_HIGH Register
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(T_HIGH_REGISTER); // Point to T_HIGH register
//  _i2cPort->write(registerByte[0]); // Write first byte
//  _i2cPort->write(registerByte[1]); // Write second byte
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[3];
  writeBuffer[0] = T_HIGH_REGISTER;
  writeBuffer[1] = registerByte[0];
  writeBuffer[2] = registerByte[1];

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 3;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

void tmp102_setLowTempF(float temperature)
{
  temperature = (temperature - 32) * 5 / 9; // Convert temperature to C
  tmp102_setLowTempC(temperature);                 // Set T_LOW
}

void tmp102_setHighTempF(float temperature)
{
  temperature = (temperature - 32) * 5 / 9; // Convert temperature to C
  tmp102_setHighTempC(temperature);                // Set T_HIGH
}

float tmp102_readLowTempC(void)
{
  uint8_t registerByte[2]; // Store the data from the register here
  bool extendedMode;       // Store extended mode bit here 0:-55C to +128C, 1:-55C to +150C
  int16_t digitalTemp;     // Store the digital temperature value here

  // Check if temperature should be 12 or 13 bits
  tmp102_openPointerRegister(CONFIG_REGISTER); // Read configuration register settings
  // Read current configuration register value
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);
  extendedMode = (registerByte[1] & 0x10) >> 4; // 0 - temp data will be 12 bits
                                                // 1 - temp data will be 13 bits
  tmp102_openPointerRegister(T_LOW_REGISTER);
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);

  if (registerByte[0] == 0xFF && registerByte[1] == 0xFF)
  {
    return NAN;
  }

  if (extendedMode) // 13 bit mode
  {
    // Combine bytes to create a signed int
    digitalTemp = ((registerByte[0]) << 5) | (registerByte[1] >> 3);
    // Temperature data can be + or -, if it should be negative,
    // convert 13 bit to 16 bit and use the 2s compliment.
    if (digitalTemp > 0xFFF)
    {
      digitalTemp |= 0xE000;
    }
  }
  else // 12 bit mode
  {
    // Combine bytes to create a signed int
    digitalTemp = ((registerByte[0]) << 4) | (registerByte[1] >> 4);
    // Temperature data can be + or -, if it should be negative,
    // convert 12 bit to 16 bit and use the 2s compliment.
    if (digitalTemp > 0x7FF)
    {
      digitalTemp |= 0xF000;
    }
  }
  // Convert digital reading to analog temperature (1-bit is equal to 0.0625 C)
  return digitalTemp * 0.0625;
}

float tmp102_readHighTempC(void)
{
  uint8_t registerByte[2]; // Store the data from the register here
  bool extendedMode;       // Store extended mode bit here 0:-55C to +128C, 1:-55C to +150C
  int16_t digitalTemp;     // Store the digital temperature value here

  // Check if temperature should be 12 or 13 bits
  tmp102_openPointerRegister(CONFIG_REGISTER); // read configuration register settings
  // Read current configuration register value
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);
  extendedMode = (registerByte[1] & 0x10) >> 4; // 0 - temp data will be 12 bits
                                                // 1 - temp data will be 13 bits
  tmp102_openPointerRegister(T_HIGH_REGISTER);
  registerByte[0] = tmp102_readRegister(0);
  registerByte[1] = tmp102_readRegister(1);

  if (registerByte[0] == 0xFF && registerByte[1] == 0xFF)
  {
    return NAN;
  }

  if (extendedMode) // 13 bit mode
  {
    // Combine bytes to create a signed int
    digitalTemp = ((registerByte[0]) << 5) | (registerByte[1] >> 3);
    // Temperature data can be + or -, if it should be negative,
    // convert 13 bit to 16 bit and use the 2s compliment.
    if (digitalTemp > 0xFFF)
    {
      digitalTemp |= 0xE000;
    }
  }
  else // 12 bit mode
  {
    // Combine bytes to create a signed int
    digitalTemp = ((registerByte[0]) << 4) | (registerByte[1] >> 4);
    // Temperature data can be + or -, if it should be negative,
    // convert 12 bit to 16 bit and use the 2s compliment.
    if (digitalTemp > 0x7FF)
    {
      digitalTemp |= 0xF000;
    }
  }
  // Convert digital reading to analog temperature (1-bit is equal to 0.0625 C)
  return digitalTemp * 0.0625;
}

float tmp102_readLowTempF(void)
{
  return tmp102_readLowTempC() * 9.0 / 5.0 + 32.0;
}

float tmp102_readHighTempF(void)
{
  return tmp102_readHighTempC() * 9.0 / 5.0 + 32.0;
}

void tmp102_setFault(uint8_t faultSetting)
{
  uint8_t registerByte; // Store the data from the register here

  faultSetting = faultSetting & 3; // Make sure rate is not set higher than 3.

  // Change pointer address to configuration register (0x01)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(0);

  // Load new conversion rate
  registerByte &= 0xE7;              // Clear F0/1 (bit 3 and 4 of first byte)
  registerByte |= faultSetting << 3; // Shift new fault setting

  // Set configuration registers
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte);    // Write byte to register
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[2];
  writeBuffer[0] = CONFIG_REGISTER;
  writeBuffer[1] = registerByte;

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 2;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

void tmp102_setAlertMode(bool mode)
{
  uint8_t registerByte; // Store the data from the register here

  // Change pointer address to configuration register (1)
  tmp102_openPointerRegister(CONFIG_REGISTER);

  // Read current configuration register value
  registerByte = tmp102_readRegister(0);

  // Load new conversion rate
  registerByte &= 0xFD;      // Clear old TM bit (bit 1 of first byte)
  registerByte |= mode << 1; // Shift in new TM bit

  // Set configuration registers
//  _i2cPort->beginTransmission(_address);
//  _i2cPort->write(CONFIG_REGISTER); // Point to configuration register
//  _i2cPort->write(registerByte);    // Write byte to register
//  _i2cPort->endTransmission();      // Close communication with TMP102

  I2C_Transaction i2cTransaction = {0};
  uint8_t writeBuffer[2];
  writeBuffer[0] = CONFIG_REGISTER;
  writeBuffer[1] = registerByte;

  i2cTransaction.targetAddress = _address;
  i2cTransaction.writeBuf = writeBuffer;
  i2cTransaction.writeCount = 2;
  i2cTransaction.readBuf = NULL;
  i2cTransaction.readCount = 0;

  status = I2C_transfer(i2cHandle, &i2cTransaction);
  if (status == false)
  {
      // Unsuccessful I2C transfer
      if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
      {
          // I2C target address not acknowledged
      }
  }
}

#endif //TMP102
