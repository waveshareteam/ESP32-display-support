#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sd_card_bsp.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SDMMC_U

#define PIN_NUM_MISO  8
#define PIN_NUM_MOSI  42
#define PIN_NUM_CLK   9
#define SDlist "/sd_card" // Directory, similar to a standard
#ifndef SDMMC_U
#define PIN_NUM_CS    42
#define SD_SPI SPI3_HOST
#endif

sdmmc_card_t *card = NULL; // Handle

void SD_card_Init(void)
{
#ifdef SDMMC_U
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = true,     // If mount fails, create partition table and format SD card
    .max_files = 5,                     // Max number of open files
    .allocation_unit_size = 512,        // Similar to sector size
    .disk_status_check_enable = 1,
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  //host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // High speed

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;           // 1-line
  slot_config.clk = PIN_NUM_CLK;
  slot_config.cmd = PIN_NUM_MOSI;
  slot_config.d0 = PIN_NUM_MISO;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card));

  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); // Print SD card information
    //printf("sios:%.2f\n",(float)(card->csd.capacity)/2048/1024); // in GB
  }
  //char list[30];
  //sprintf(list,"%s%s",SDlist,"Waveshare Image1.png");
  //s_example_read_file(list);
#endif

#ifndef SDMMC_U
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = true,    // If mount fails, create partition table and format SD card
    .max_files = 5,                    // Max number of open files
    .allocation_unit_size = 512        // Similar to sector size
  };
  spi_bus_config_t bus_cfg = 
  {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,   // Max transfer size   
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_initialize(SD_SPI, &bus_cfg, SDSPI_DEFAULT_DMA));
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_NUM_CS;
  slot_config.host_id = SD_SPI;
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdspi_mount(SDlist, &host, &slot_config, &mount_config, &card)); // Mount SD card
  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); // Print SD card information
    printf("sios:%.2f\n",(float)(card->csd.capacity)/2048/1024); // in GB
  }
#endif
  //char list[30];
  //sprintf(list,"%s%s",SDlist,"Waveshare Image1.png");
  //s_example_read_file(list);
}

float sd_card_get_value(void)
{
  if(card != NULL)
  {
    return (float)(card->csd.capacity)/2048/1024; // Return capacity in GB
  }
  else
  return 0;
}

/* Write data
path: file path
data: data to write
*/
esp_err_t s_example_write_file(const char *path, char *data)
{
  esp_err_t err;
  if(card == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); // Check if SD card is present
  if(err != ESP_OK)
  {
    return err;
  }
  FILE *f = fopen(path, "w"); // Open path for writing
  if(f == NULL)
  {
    printf("path: Write Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fprintf(f, data); // Write data
  fclose(f);
  return ESP_OK;
}

/* Read data
path: file path
*/
esp_err_t s_example_read_file(const char *path, uint8_t *pxbuf, uint32_t *outLen)
{
  esp_err_t err;
  if(card == NULL)
  {
    printf("path: card == NULL\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); // Check if SD card is present
  if(err != ESP_OK)
  {
    printf("path: card == NO\n");
    return err;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    printf("path: Read Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END);     // Move pointer to the end
  uint32_t unlen = ftell(f);
  //fgets(pxbuf, unlen, f); // Read text
  fseek(f, 0, SEEK_SET); // Move pointer to the start
  uint32_t poutLen = fread((void *)pxbuf, 1, unlen, f);
  printf("pxlen: %ld, outLen: %ld\n", unlen, poutLen);
  *outLen = poutLen;
  fclose(f);
  return ESP_OK;
}

/*
struct stat st;
stat(file_foo, &st); // Get file info, returns 0 on success; file_foo is a string including file extension to check for file existence
unlink(file_foo); // Delete file, returns 0 on success
rename(char*, char*); // Rename file
esp_vfs_fat_sdcard_format(); // Format SD card
esp_vfs_fat_sdcard_unmount(mount_point, card); // Unmount VFS
FILE *fopen(const char *filename, const char *mode);
"w": Open file for writing, clears content if file exists; creates new file if it doesn’t exist
"a": Open file in append mode, creates new file if it doesn’t exist
mkdir(dirname, mode); Create directory

Use "rb" mode to read non-text data (like images); it reads files in binary mode.
Using "r" opens the file in text mode, which may corrupt or give errors when reading binary files.
For image files (JPEG, PNG, etc.), use "rb" mode to ensure correct data reading.
b converts data to binary
To get file size:
  fseek(file, 0, SEEK_END); // Move pointer to file end
  ftell(file); // Return current pointer position, which is the file size in bytes
*/
