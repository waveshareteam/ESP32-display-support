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


#define VersionControl_V2

#define SDlist "/sd_card" // Directory, acts like a standard

#ifdef VersionControl_V2
  #define PIN_NUM_D0    (gpio_num_t)8
  #define PIN_NUM_CMD   (gpio_num_t)42
  #define PIN_NUM_CLK   (gpio_num_t)9
#else
  #define PIN_NUM_MISO  (gpio_num_t)8
  #define PIN_NUM_MOSI  (gpio_num_t)42
  #define PIN_NUM_CLK   (gpio_num_t)47
  #define PIN_NUM_CS    (gpio_num_t)9
  #define SD_SPI SPI3_HOST
#endif


sdmmc_card_t *card = NULL; // Handle


void SD_card_Init(void)
{
#ifdef VersionControl_V2
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
  slot_config.width = 1;           // 1-wire
  slot_config.clk = PIN_NUM_CLK;
  slot_config.cmd = PIN_NUM_CMD;
  slot_config.d0 = PIN_NUM_D0;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card));

  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); // Print card information
    printf("Size_GB: %.2f\n", (float)(card->csd.capacity) / 2048 / 1024); // in GB
  }

#else
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = true,     // If mount fails, create partition table and format SD card
    .max_files = 5,                     // Max number of open files
    .allocation_unit_size = 512         // Similar to sector size
  };
  spi_bus_config_t bus_cfg = 
  {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,           // Max transfer size   
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
    sdmmc_card_print_info(stdout, card); // Print card information
    printf("Size_GB: %.2f\n", (float)(card->csd.capacity) / 2048 / 1024); // in GB
  }
#endif
}
float sd_card_get_value(void)
{
  if(card != NULL)
  {
    return (float)(card->csd.capacity) / 2048 / 1024; // GB
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
  err = sdmmc_get_status(card); // First check if the SD card is present
  if(err != ESP_OK)
  {
    return err;
  }
  FILE *f = fopen(path, "w"); // Open the file path
  if(f == NULL)
  {
    printf("Error: Write path incorrect\n");
    return ESP_ERR_NOT_FOUND;
  }
  fprintf(f, data); // Write data
  fclose(f);
  return ESP_OK;
}

/* Read data
path: file path
pxbuf: buffer to store read data
outLen: pointer to store the output length
*/
esp_err_t s_example_read_file(const char *path, uint8_t *pxbuf, uint32_t *outLen)
{
  esp_err_t err;
  if(card == NULL)
  {
    printf("Error: card == NULL\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); // First check if the SD card is present
  if(err != ESP_OK)
  {
    printf("Error: card not found\n");
    return err;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    printf("Error: Read path incorrect\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END);     // Move pointer to the end of the file
  uint32_t unlen = ftell(f);
  // fgets(pxbuf, unlen, f); // Read text
  fseek(f, 0, SEEK_SET); // Move pointer back to the beginning
  uint32_t poutLen = fread((void *)pxbuf, 1, unlen, f);
  printf("Read length: %ld, Output length: %ld\n", unlen, poutLen);
  *outLen = poutLen;
  fclose(f);
  return ESP_OK;
}

/*
Additional functions:
  struct stat st;
  stat(file_foo, &st); // Get file information, returns 0 on success. file_foo is a string with the filename and extension. Can be used to check if the file exists.
  unlink(file_foo); // Delete file, returns 0 on success
  rename(old_name, new_name); // Rename file
  esp_vfs_fat_sdcard_format(); // Format the SD card
  esp_vfs_fat_sdcard_unmount(mount_point, card); // Unmount the vfs
  FILE *fopen(const char *filename, const char *mode);
  "w": Open file in write mode. If the file exists, clears its content; if it doesn't exist, creates a new file.
  "a": Open file in append mode. If the file doesn't exist, creates a new file.
  mkdir(dirname, mode); // Create directory
  
  Reading binary data:
  Use "rb" mode to open files in read and binary mode, suitable for images or other binary files.
  If you use "r", it opens the file in text mode, which can lead to data corruption or errors when reading binary files.
  For image files (JPEG, PNG, etc.), use "rb" mode to ensure correct reading of file content.

  To get file size:
    fseek(file, 0, SEEK_END); // Move file pointer to end
    ftell(file); // Returns current file pointer position, representing file size in bytes
*/
