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


//#define VersionControl_V2

#define SDlist "/sd_card" // Directory, acting as a standard path

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
    .format_if_mount_failed = true,     // Format SD card if mounting fails
    .max_files = 5,                     // Maximum number of open files
    .allocation_unit_size = 512,        // Allocation unit size (similar to sector size)
    .disk_status_check_enable = 1,
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  //host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // High-speed mode

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;           // 1-wire mode
  slot_config.clk = PIN_NUM_CLK;
  slot_config.cmd = PIN_NUM_CMD;
  slot_config.d0 = PIN_NUM_D0;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card));

  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); // Print card information
    printf("practical_size:%.2f\n",(float)(card->csd.capacity) / 2048 / 1024); // Size in GB
  }
#else
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = true,    // Format SD card if mounting fails
    .max_files = 5,                    // Maximum number of open files
    .allocation_unit_size = 512        // Allocation unit size
  };
  spi_bus_config_t bus_cfg = 
  {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,   // Maximum transfer size   
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_initialize(SD_SPI, &bus_cfg, SDSPI_DEFAULT_DMA));
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = (gpio_num_t)PIN_NUM_CS;
  slot_config.host_id = SD_SPI;
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdspi_mount(SDlist, &host, &slot_config, &mount_config, &card)); // Mount SD card
  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); // Print card information
    printf("practical_size:%.2f\n",(float)(card->csd.capacity) / 2048 /1024); // Size in GB
  }
#endif
}
float sd_cadr_get_value(void)
{
  if(card != NULL)
  {
    return (float)(card->csd.capacity) / 2048 / 1024; // Size in GB
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
  FILE *f = fopen(path, "w"); // Open file at path
  if(f == NULL)
  {
    printf("path:Write Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fprintf(f, data); // Write data
  fclose(f);
  return ESP_OK;
}

/* Read data
path: file path
*/
esp_err_t s_example_read_file(const char *path,uint8_t *pxbuf,uint32_t *outLen)
{
  esp_err_t err;
  if(card == NULL)
  {
    printf("path:card == NULL\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); // Check if SD card is present
  if(err != ESP_OK)
  {
    printf("path:card == NO\n");
    return err;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    printf("path:Read Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END);     // Move pointer to end of file
  uint32_t unlen = ftell(f);
  fseek(f, 0, SEEK_SET);      // Move pointer back to start
  uint32_t poutLen = fread((void *)pxbuf,1,unlen,f);
  printf("pxlen: %ld,outLen: %ld\n",unlen,poutLen);
  *outLen = poutLen;
  fclose(f);
  return ESP_OK;
}

/*
Additional file operations:
  struct stat st;
  stat(file_foo, &st); // Get file info, returns 0 on success. file_foo is a string, with file extension included.
  unlink(file_foo); // Delete file, returns 0 on success
  rename(char*,char*); // Rename file
  esp_vfs_fat_sdcard_format(); // Format SD card
  esp_vfs_fat_sdcard_unmount(mount_point, card); // Unmount VFS
  FILE *fopen(const char *filename, const char *mode);
  "w": Open file for writing, clears contents if file exists, or creates file if it doesn't exist
  "a": Open file in append mode, creates file if it doesn't exist
  mkdir(dirname, mode); // Create a directory
  
For non-text files, open in "rb" mode to avoid data corruption. Binary files like images require "rb" to be read correctly.
To get the file size:
  fseek(file, 0, SEEK_END); // Move pointer to end of file
  ftell(file); // Returns current position, which is the file size in bytes.*/
