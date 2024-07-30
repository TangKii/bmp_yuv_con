#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)  // Pack structures without padding

typedef struct
{
    uint16_t signature;  // "BM" for BMP files
    uint32_t fileSize;   // Size of the BMP file
    uint32_t reserved;   // Reserved, set to 0
    uint32_t dataOffset; // Offset of image data in the file
} BMPHeader;

typedef struct
{
    uint32_t headerSize;       // Size of this header (40 bytes)
    int32_t  width;            // Width of the image in pixels
    int32_t  height;           // Height of the image in pixels
    uint16_t planes;           // Number of color planes (must be 1)
    uint16_t bitsPerPixel;     // Number of bits per pixel (must be 24 for BMP)
    uint32_t compression;      // Compression method (0 for no compression)
    uint32_t imageSize;        // Size of the raw bitmap data
    int32_t  xResolution;      // Horizontal resolution of the image
    int32_t  yResolution;      // Vertical resolution of the image
    uint32_t colorsUsed;       // Number of colors in the color palette (0 for full color)
    uint32_t importantColors;  // Number of important colors (0 for all)
} BMPInfoHeader;

#pragma pack(pop)  // Restore default packing

#define MIN_VALUE 0
#define MAX_VALUE 255

// 最大值限制函数
uint8_t limitToRange(uint8_t value) {
    if (value < MIN_VALUE) {
        return MIN_VALUE;
    } else if (value > MAX_VALUE) {
        return MAX_VALUE;
    } else {
        return value;
    }
}

void convertBMPtoYV16(FILE *bmpFile, FILE *yv16File) {
    BMPHeader bmpHeader;
    fread(&bmpHeader, sizeof(BMPHeader), 1, bmpFile);

    BMPInfoHeader bmpInfoHeader;
    fread(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, bmpFile);

    // Check if the BMP file is 24 bits per pixel
    if (bmpInfoHeader.bitsPerPixel != 24) {
        fprintf(stderr, "Only 24-bit BMP files are supported.\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for image data
    uint8_t *imageData = (uint8_t *)malloc(bmpInfoHeader.imageSize);
    fread(imageData, 1, bmpInfoHeader.imageSize, bmpFile);

    // Calculate size of U and V planes
    int uvSize = bmpInfoHeader.width * bmpInfoHeader.height / 2;

    // Allocate memory for U and V planes
    uint8_t *uPlane = (uint8_t *)malloc(uvSize);
    uint8_t *vPlane = (uint8_t *)malloc(uvSize);

    // Calculate row padding
    int rowPadding = (4 - (bmpInfoHeader.width * 3) % 4) % 4;

    fprintf(stderr, "bmpInfoHeader.width %d, bmpInfoHeader.height %d, rowPadding %d\n", bmpInfoHeader.width, bmpInfoHeader.height, rowPadding);
    // Perform YUV422 Planar (YV16) conversion
    for (int y = bmpInfoHeader.height - 1; y >= 0; y--) {
        for (int x = 1; x <= bmpInfoHeader.width; x++) {
            int index = y * (bmpInfoHeader.width * 3 + rowPadding) + (x-1) * 3;

            uint8_t B = imageData[index];
            uint8_t G = imageData[index + 1];
            uint8_t R = imageData[index + 2];
            uint8_t Y, U, V, U_last, V_last;

            // Convert RGB to YUV
             Y = limitToRange(0.299 * R + 0.587 * G + 0.114 * B);
             U = limitToRange(-0.14713 * R - 0.288862 * G + 0.436 * B + 128);
             V = limitToRange(0.615 * R - 0.51498 * G - 0.10001 * B + 128);
            // Populate Y plane
            fwrite(&Y, 1, 1, yv16File);

            // Populate U and V planes (every second pixel)
            if (x % 2 == 0) {
                int uvIndex = (bmpInfoHeader.height - 1 - y) * bmpInfoHeader.width / 2 + (x-1) / 2;
                uPlane[uvIndex] = (U + U_last) >> 1;
                vPlane[uvIndex] = (V + V_last) >> 1;
            }

            V_last = V;
            U_last = U;
        }
        // Skip row padding
        fseek(bmpFile, rowPadding, SEEK_CUR);
    }

    // Write U and V planes to the output file
    fwrite(uPlane, 1, uvSize, yv16File);
    fwrite(vPlane, 1, uvSize, yv16File);

    // Cleanup
    free(imageData);
    free(uPlane);
    free(vPlane);
}

int main(int argc, char *argv[]) {

    // 打开输入文件以读取内容
    FILE *bmpFile = fopen(argv[1], "rb");
    if (bmpFile == NULL) {
        printf("open input file %s\n", argv[1]);
        return 1;
    }

    // 创建输出文件以写入结果
    FILE * yv16File = fopen(argv[2], "wb");
    if (yv16File == NULL) {
        printf("open output file %s\n", argv[2]);
        fclose(bmpFile);
        return 1;
    }
    

    if (bmpFile == NULL || yv16File == NULL) {
        fprintf(stderr, "Error opening files.\n");
        exit(EXIT_FAILURE);
    }

    convertBMPtoYV16(bmpFile, yv16File);

    // Cleanup
    fclose(bmpFile);
    fclose(yv16File);

    return 0;
}
