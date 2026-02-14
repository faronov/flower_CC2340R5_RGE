/*
  OPT3001 Ambient light sensor (ALS) driver for CC2340R5, CC2652
  Product: https://www.ti.com/product/OPT3001
  Written by Koptyakov Sergey
  From source https://github.com/karl-mohring/OPT3001_driver
*/
#ifdef OPT3001
#include "opt3001.h"

#include <opt3001.h>
#include <ti/log/Log.h>
#include "ti_drivers_config.h"
// Import I2C Driver definitions
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CLPF3.h>

#include <math.h>

extern I2C_Handle i2cHandle;
/**
 * Set the address of the sensor.
 * The address is set with hardware, depending on the configuration of the ADDR pin.
 *
 * ADDR -> GND = 0x44
 * ADDR -> VDD = 0x45
 * ADDR -> SDA = 0x46
 * ADDR -> SCL = 0x47
 *
 * @param address: Address to set the driver to.
 */
void OPT3001_set_address(uint8_t address)
{
    // Restrict the input address to the valid range
    address |= 0b1000100;
    address = address & 0b1000111;
    device_address = address;
}

/**
 * Write a value to a register using I2C
 *
 * @param input: Byte to write to the register.
 * @param address: Address of register to write to.
 * @return: Success/error result of the write.
 */
bool OPT3001_write(uint8_t *input, OPT3001_reg_t address)
{
/*
    bool result = true;
    Wire.beginTransmission(device_address);
    Wire.write(address);
    for (size_t i = 0; i < length; i++)
    {
        Wire.write(input[i]);
    }

    if (Wire.endTransmission() != 0)
    {
        result = false;
    }
    return result;
*/
    bool result = true;
    I2C_Transaction i2cTransaction = {0};
    uint8_t writeBuffer[3];
    writeBuffer[0] = address;
    writeBuffer[1] = input[1];
    writeBuffer[2] = input[0];

    i2cTransaction.targetAddress = device_address;
    i2cTransaction.writeBuf = writeBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    result = I2C_transfer(i2cHandle, &i2cTransaction);
    if (result == false)
    {
        // Unsuccessful I2C transfer
//        Log_printf(LogModule_Zigbee_App, Log_INFO, "Unsuccessful I2C transfer %d address %x", result, device_address);
        if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
        {
            // I2C target address not acknowledged
        }
    }
//    Log_printf(LogModule_Zigbee_App, Log_INFO, "Successful I2C transfer status %d address %x" , result, device_address);
    return result;
}

/**
 * Read a specified number of bytes using the I2C bus.
 * @param output: The buffer in which to store the read values.
 * @param address: Register address to read (or starting address in burst reads)
 * @param length: Number of bytes to read.
 */
bool OPT3001_read(uint8_t *output, OPT3001_reg_t address)
{
/*
    bool result = true;
    Wire.beginTransmission(address);
    Wire.write(address);
    if (Wire.endTransmission() != 0)
        result = false;

    else // OK, all worked, keep going
    {
        Wire.requestFrom(address, length);
        for (size_t i = 0; (i < length) and Wire.available(); i++)
        {
            uint8_t c = Wire.read();
            output[i] = c;
        }
    }
    return result;
*/
    bool result = true;
    I2C_Transaction i2cTransaction = {0};
    uint8_t writeBuffer[1];
    writeBuffer[0] = address;
    uint8_t readBuffer[2];

    i2cTransaction.targetAddress = device_address;
    i2cTransaction.writeBuf = writeBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = readBuffer;
    i2cTransaction.readCount = 2;

    result = I2C_transfer(i2cHandle, &i2cTransaction);
    if (result == false)
    {
        // Unsuccessful I2C transfer
//        Log_printf(LogModule_Zigbee_App, Log_INFO, "Unsuccessful I2C transfer %d address %x", result, device_address);
        if (i2cTransaction.status == I2C_STATUS_ADDR_NACK)
        {
            // I2C target address not acknowledged
        }
        return result;
    }
//    Log_printf(LogModule_Zigbee_App, Log_INFO, "Successful I2C transfer status %d address %x" , result, device_address);
    output[0] = readBuffer[1];
    output[1] = readBuffer[0];
    return result;
}

/**
 *
 */
void OPT3001_apply_config(void)
{
//    OPT3001_read_config();
//    Log_printf(LogModule_Zigbee_App, Log_INFO, "OPT3001_CONFIG write %x" , config.raw);
    OPT3001_write((uint8_t *)&config.raw, OPT3001_CONFIG);
//    OPT3001_read_config();
}

/**
 *
 */
OPT3001_config_t OPT3001_read_config(void)
{
    OPT3001_config_t current_config;
    OPT3001_read((uint8_t *)&current_config, OPT3001_CONFIG);
//    Log_printf(LogModule_Zigbee_App, Log_INFO, "OPT3001_CONFIG read %x" , current_config.raw);
    return current_config;
}

/**
 * Check that things work
 */
bool OPT3001_check_comms(void)
{
    uint16_t manufacturer_id;
    OPT3001_read((uint8_t *)&manufacturer_id, OPT3001_MANUFACTURER_ID);
    // Make sure the manufacturer's ID matches the expected value ('TI')
    bool success = false;
    if (manufacturer_id == 0x5449)
        success = true;
    Log_printf(LogModule_Zigbee_App, Log_INFO, "OPT3001_MANUFACTURER_ID %x address %x success %d" , manufacturer_id, device_address, success);
    return success;
}

/**
 * Calculate the optical power measured by the sensor.
 * @return: Optical power of incident light in nW/cm^2
 */
uint32_t OPT3001_get_illuminance(void)
{
    OPT3001_result_t result;
    OPT3001_read((uint8_t *)&result, OPT3001_RESULT);

    // Calculate optical power [ref: Equation 1, OPT3001 Datasheet]
    // Optical_Power = R[11:0] * 2^(E[3:0]) * 1.2 nW/cm^2
    uint32_t exponent = 1 << result.exponent;
    uint32_t illuminance = result.reading * exponent * 0.01;
    return illuminance;
}

bool OPT3001_begin(uint8_t address)
{
    OPT3001_set_address(address);
    bool working = OPT3001_check_comms();
    if (working)
    {
        OPT3001_apply_config();
    }

    return working;
}

void OPT3001_set_high_limit(OPT3001_result_t high_limit)
{
    OPT3001_write((uint8_t *)&high_limit, OPT3001_HIGH_LIMIT);
}

OPT3001_result_t OPT3001_get_high_limit(void)
{
    OPT3001_result_t limit;
    OPT3001_read((uint8_t *)&limit, OPT3001_HIGH_LIMIT);
    return limit;
}

void OPT3001_set_low_limit(OPT3001_result_t low_limit)
{
    OPT3001_write((uint8_t *)&low_limit, OPT3001_LOW_LIMIT);
}

/**
 * Get the low limit level from the sensor.
 * The default low-level after reset is 0.
 */
OPT3001_result_t OPT3001_get_low_limit(void)
{
    OPT3001_result_t limit;
    OPT3001_read((uint8_t *)&limit, OPT3001_LOW_LIMIT);
    return limit;
}

#endif //OPT3001
