idf_component_register(
  SRCS "user_app.c"
  PRIV_REQUIRES i2c_bsp qmi8658c
  INCLUDE_DIRS "./")
