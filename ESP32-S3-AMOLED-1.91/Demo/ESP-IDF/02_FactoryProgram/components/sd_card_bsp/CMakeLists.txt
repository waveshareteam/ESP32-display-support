idf_component_register(
  SRCS "sd_card_bsp.c"
  PRIV_REQUIRES fatfs sdmmc                
  INCLUDE_DIRS "./")
#REQUIRES fatfs
#PRIV_REQUIRES