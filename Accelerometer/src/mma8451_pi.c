#include "mma8451_pi.h"
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

void mma8451_write_byte(mma8451* handle, int reg, char data)
{
    unsigned char outbuf[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1]; //We are just writing one byte to the device

    messages[0].addr  = handle->address;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    //Register address
    outbuf[0] = reg;

    //Data
    outbuf[1] = data;

    //Configure packet list
    packets.msgs  = messages;
    packets.nmsgs = 1;

    //Send to the bus
    if(ioctl(handle->file, I2C_RDWR, &packets) < 0)
    {
        // perror("Unable to send data");
    }

}

char mma8451_read_byte(mma8451* handle, int reg)
{
    unsigned char inbuf, outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    //Send only the address in a write
    outbuf = reg;
    messages[0].addr  = handle->address;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    //Structure where the data will be written
    messages[1].addr  = handle->address;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = sizeof(inbuf);
    messages[1].buf   = &inbuf;

    //Build packet list
    packets.msgs      = messages;
    packets.nmsgs     = 2;

    //Send to the bus
    if(ioctl(handle->file, I2C_RDWR, &packets) < 0)
    {
        //perror("Unable to send data");
    }

    //return the value
    return inbuf;
}

void mma8451_read_multibyte(mma8451* handle, int reg, char* output, size_t len)
{
    unsigned char outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    //Send only the address in a write
    outbuf = reg;
    messages[0].addr  = handle->address;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    //Structure where the data will be written
    //
    //the mma8451 auto-increment address on successive reads. Reading 3 bytes from 0x01 will read 0x01, 0x02 and 0x03 
    messages[1].addr  = handle->address;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = len;    //trusting the user that the lengs is less or equals the size of whatever's output is pointing to
    messages[1].buf   = output; //pointer given by the user

    //Build packet list
    packets.msgs      = messages;
    packets.nmsgs     = 2;

    //Send to the bus
    if(ioctl(handle->file, I2C_RDWR, &packets) < 0)
    {
        // perror("Unable to send data");
    }
}

mma8451 mma8451_initialise(int device, int addr)
{
    mma8451 handle;
    handle.file = -1;
    handle.address = addr;
    char buf[15];

    //Open /dev/i2c-x file without a buffer
    sprintf(buf, "/dev/i2c-%d", device);
    if ((handle.file = open(buf, O_RDWR)) < 0)
    {
        handle.file = -2;
        return handle;
    }

    //Configure slave i2c address via ioctl
    if(ioctl(handle.file, I2C_SLAVE, addr) < 0)
    {
        handle.file = -3;
        return handle;
    }

    //Check if we read correctly from the sensor
    char whoami = mma8451_read_byte(&handle, 0x0D);

    //Undefined behavior for the rest of device operation if the device is not returning hex 1A
    if(whoami != 0x1A) perror("mma451_pi warning: Device correctly intialized but not returning 0x1A at WHO_AM_I request.\n"
            "Are you sure you are using a MMA8451 accelerometer on this address?");

    //Send reset request
    mma8451_write_byte(&handle, 0x2B, 0x40);
    while(mma8451_read_byte(&handle, 0x2B) & 0x40); //reset done

    mma8451_set_range(&handle, 2);
    mma8451_write_byte(&handle, 0x2B, 0x02); //high resolution mode
    mma8451_write_byte(&handle, 0x2A, 0x01 | 0x04); //high rate low noise

    //Deactivate fifo
    mma8451_write_byte(&handle, 0x09, 0);
    //turn on orientation configuration
    mma8451_write_byte(&handle, 0x11, 0x40);

    return handle;
}

void mma8451_get_raw_sample(mma8451* handle, char* output)
{
    mma8451_read_multibyte(handle, 0x01, output, 6);
}

uint16_t get_divider(const unsigned char range)
{
    switch (range)
    {
        default:
            perror("unknown range. use 2, 4 or 8 only!");
        case 8:
            return 1024;
        case 4:
            return 2048;
        case 2:
            return 4096;

    }
}

void mma8451_get_acceleration(mma8451* handle, mma8451_vector3* vect)
{
    int16_t x, y, z;

    mma8451_get_raw_sample(handle, handle->raw_data);

    //Convert the 14bit two's complement numbers into 16bit integers that makes some sense
    x = handle->raw_data[0];
    x <<= 8;
    x |= handle->raw_data[1];
    x >>= 2;

    y = handle->raw_data[2];
    y <<= 8;
    y |= handle->raw_data[3];
    y >>= 2;

    z = handle->raw_data[4];
    z <<= 8;
    z |= handle->raw_data[5];
    z >>= 2;

    //Result is in "count" over a number that depend on the range.
    //Computer a float that represent the number of G experienced
    //on each axies and store them in the vector
    vect->x = (float) x / get_divider(handle->range);
    vect->y = (float) y / get_divider(handle->range);
    vect->z = (float) z / get_divider(handle->range);
}

mma8451_vector3 mma8451_get_acceleration_vector(mma8451* handle)
{
    mma8451_vector3 vector;
    mma8451_get_acceleration(handle, &vector);
    return vector;
}

void mma8451_set_range(mma8451* handle, unsigned char range)
{
    handle->range = range;
    unsigned char XYZ_DATA_CFG = 0, REG1 = 0;
    switch(range)
    {
        default:
            perror("unknown range. use 2, 4 or 8 only!");
        case 2:
            XYZ_DATA_CFG = 0b00;
            break;
        case 4:
            XYZ_DATA_CFG = 0b01;
            break;
        case 8:
            XYZ_DATA_CFG = 0b10;
            break;
    }

    REG1 = mma8451_read_byte(handle, 0x2A) | 0x01;
    mma8451_write_byte(handle, 0x2A, 0x00);
    mma8451_write_byte(handle, 0x0E, XYZ_DATA_CFG);
    mma8451_write_byte(handle, 0x2A, REG1);
}

