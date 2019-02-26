#ifndef MMA8451_PI_INCLUDED
#define MMA8451_PI_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

///Structure that represent a mma8451 sensor. Get it from the initialise function and give it the other ones
typedef struct mma8451_
{
    ///File descriptor that point to an i2c device in /dev
    int file;

    ///Address of the sensor on the i2c bus
    unsigned char address;

    ///Current range setting
    unsigned char range;

    ///raw data as sent by the sensor
    unsigned char raw_data[6];
} mma8451;

///Default address of the sensor if you do nothing
#define MMA8451_DEFAULT_ADDR 0x1D

///Address if you pull down the address line
#define MMA8451_PULLDOWN_ADDR 0x1C


///Initialse the sensor at the given address
mma8451 mma8451_initialise(int device, int i2c_addr);


///Vector of 3 float
typedef struct mma8451_vector3_
{
    float x, y, z;
} mma8451_vector3;


///write the content of register 0x01 to 0x06 to the output buffer.
void mma8451_get_raw_sample(mma8451* handle, char* output);

///get the current acceleration vector
mma8451_vector3 mma8451_get_acceleration_vector(mma8451* handle);
void mma8451_get_acceleration(mma8451* handle, mma8451_vector3* vector);

///set the "range" (aka:the max acceleration we register)
void mma8451_set_range(mma8451* handle, unsigned char range);

#ifdef __cplusplus
} //extern "C"
#endif


//Here we are on C++ linkage land
#ifdef __cplusplus
#include <iostream>

///operator overload ot print out the content of a mma8451_vector object if you are using C++
std::ostream& operator<< (std::ostream& out, const mma8451_vector3& vector)
{
    out << "mma8451_vector3(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
    return out;
}
#endif


#endif
