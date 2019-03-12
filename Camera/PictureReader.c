#include <stdio.h>
#include <"ssd1306_16bit.h">
#include <unistd.h>

static const char *cameraOutput = "/tmp/hudview_camera_output";
// image size = resolution * bytes/pixel
static const int imageSize = 160 * 120 * (16 / 2);
// sleep is 1 / framewrate of camera in microseconds
static const int sleep = 1000000 / 10;

int main(int argc, char **argv) {
    FILE* imageFile;
    unsigned char imageData[imageSize];
    
    imageFile = fopen(cameraOutput, "rb");
    while(1) {
        memset(imageData, 0, imageSize);
        size_t newLen = fread(imageData, sizeof(unsigned char), imageSize, imageFile);
        ssd1306_drawBitmap16(0, 0, 160, 120, &imageData);
        usleep(sleep);
    }
}
