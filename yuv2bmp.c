#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

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

void print_help(void) {
    printf("Usage: -w <width> -h <height> [-v] <input_file> <output_file>\n");
    printf("Options:\n");
    printf("  -v, -verbose     Enable verbose mode for debugging\n");
    printf("  -w <width>       Specify the width of the output image\n");
    printf("  -h <height>      Specify the height of the output image\n");
}
// 将YUV数据转换为RGB数据
void yuv422p_to_rgb(unsigned char *y_data, unsigned char *u_data, unsigned char *v_data, unsigned char *rgb_data, int width, int height) {
    int i, j;
    int y, u, v;
    int r, g, b;
    int c, d, e;
    int index;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            // 计算在一维数组中的偏移量
            index = i * width + j;
            // 获取YUV数据
            y = y_data[index];
            u = u_data[index/2];
            v = v_data[index/2];

            int coeff00 = 2048;
            int coeff01 = 0;
            int coeff02 = 2871;
            int coeff03 = -367447;

            int coeff10 = 2048;
            int coeff11 = -704;
            int coeff12 = -1463;
            int coeff13 = 276480;

            int coeff20 = 2048;
            int coeff21 = 3629;
            int coeff22 = 0;
            int coeff23 = -464896;

            // YUV转RGB
            r = (coeff00 * y + coeff01 * u + coeff02 * v + coeff03) >> 11;
            g = (coeff10 * y + coeff11 * u + coeff12 * v + coeff13) >> 11;
            b = (coeff20 * y + coeff21 * u + coeff22 * v + coeff23) >> 11;

            // 裁剪RGB值
            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);

            // 将RGB数据写入数组
            rgb_data[((height-1-i) * width + j) * 3] = r;
            rgb_data[(((height-1-i) * width + j) * 3) + 1] = g;
            rgb_data[(((height-1-i) * width + j) * 3) + 2] = b;
        }
    }
}


// 将RGB数据保存为BMP文件
void save_bmp(unsigned char *rgb_data, int width, int height, const char *filename) {

    FILE *file;
    uint32_t allagn_width =  ((width*3 + 3) / 4) * 4;
    uint8_t fill_data[256] = {0xff};

    BMPHeader bmpHeader = {0x4D42, sizeof(BMPHeader) + sizeof(BMPInfoHeader) + allagn_width * height * 3, 0, sizeof(BMPHeader) + sizeof(BMPInfoHeader)};
    BMPInfoHeader bmpInfoHeader = {40, width, height, 1, 24, 0, allagn_width * height * 3, 0, 0, 0, 0};

    file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error: Unable to open file for writing!\n");
        return;
    }

    // 写入BMP文件头
    fwrite(&bmpHeader, sizeof(BMPHeader), 1, file);
    fwrite(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, file);

    // 写入RGB数据
    //fwrite(rgb_data, sizeof(unsigned char), width * height * 3, file);
    for(uint32_t j=0;j<height;j++) {
		fwrite(rgb_data+j*width*3, width*3, 1, file);
		fwrite(fill_data, allagn_width-width * 3, 1, file);
	}

    fclose(file);
}

long get_file_size(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // 将文件指针移动到文件末尾
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Error seeking to end of file");
        fclose(file);
        return -1;
    }

    // 获取文件指针的当前位置，即文件大小
    long size = ftell(file);
    if (size == -1) {
        perror("Error getting file size");
        fclose(file);
        return -1;
    }

    fclose(file);
    return size;
}

int main(int argc, char *argv[]) {
    FILE *yuv_file;
    unsigned char *y_data;
    unsigned char *u_data;
    unsigned char *v_data;
    unsigned char *rgb_data;

  if (argc == 1) {
        print_help();
        return 1;
    }
    int opt;
    int flag_verbose = 0;
    int width = 0;
    int height = 0;
    int pixel_size = 0;
    char *input_file = NULL;
    char *output_file = NULL;

    // 遍历命令行参数并解析选项
    while ((opt = getopt(argc, argv, "vw:h:")) != -1) {
        switch (opt) {
            case 'v':
                flag_verbose = 1;
                break;
            case 'w':
                width = atoi(optarg); // 将参数值转换为整数
                break;
            case 'h':
                height = atoi(optarg); // 将参数值转换为整数
                break;
            case '?':
                if (optopt == 'w' || optopt == 'h') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
                break;
            default:
                return 1;
        }
    }

    // 检查是否提供了宽度和高度参数
    if (width == 0 || height == 0) {
        fprintf(stderr, "Error: Both width (-w) and height (-h) parameters are required.\n");
        return 1;
    }

    // 检查是否提供了输入和输出文件名参数
    if (optind + 1 >= argc) {
        fprintf(stderr, "Error: Input and output file names are required.\n");
        return 1;
    }

    // 获取输入和输出文件名参数
    input_file = argv[optind];
    output_file = argv[optind + 1];
    pixel_size = width*height;

    if(flag_verbose) {
        // 打印参数值
        printf("image width: %d\n", width);
        printf("image height: %d\n", height);
        printf("yuv inout file: %s\n", input_file);
        printf("bmp output file: %s\n", output_file);
    }
    if(get_file_size(input_file) != pixel_size*2) {
        printf("YUV input file size error, only YUV422P support now!\n", output_file);
        return 1;
    }
    // 打开YUV文件
    yuv_file = fopen(input_file, "rb");
    if (yuv_file == NULL) {
        printf("Error: Unable to open YUV file for reading!\n");
        return 1;
    }

    // 读取YUV数据
    y_data = (unsigned char *)malloc(pixel_size);
    u_data = (unsigned char *)malloc(pixel_size / 2);
    v_data = (unsigned char *)malloc(pixel_size / 2);
    rgb_data = (unsigned char *)malloc(pixel_size * 3);

    fread(y_data, sizeof(unsigned char),pixel_size, yuv_file);
    fread(u_data, sizeof(unsigned char), pixel_size / 2, yuv_file);
    fread(v_data, sizeof(unsigned char), pixel_size / 2, yuv_file);

    // 关闭YUV文件
    fclose(yuv_file);

    // 将YUV数据转换为RGB数据
    yuv422p_to_rgb(y_data, u_data, v_data, rgb_data, width, height);

    // 保存RGB数据为BMP文件
    save_bmp(rgb_data, width, height, output_file);
    // 释放内存
    free(y_data);
    free(u_data);
    free(v_data);
    free(rgb_data);
    return 0;
}
