#include <stdio.h>
#include "qmi8658c.h"
#include "i2c_bsp.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static char *tag = "qmi8658c";

//#define QMI8658_UINT_MG_DPS
#define M_PI			(3.14159265358979323846f)
#define ONE_G			(9.807f)
#define QFABS(x)		(((x)<0.0f)?(-1.0f*(x)):(x))


static qmi8658_state g_imu;
#if defined(QMI8658_USE_CALI)
static qmi8658_cali g_cali;
#endif

unsigned char qmi8658_write_reg(unsigned char reg, unsigned char value)
{
	uint8_t buf = value;
  if(I2C_write_buff(0x6b,reg,&buf,1) != ESP_OK)
	{
		ESP_LOGW(tag,"qmi8658c_send failed\n");
	}
	return 1;
}

unsigned char qmi8658_write_regs(unsigned char reg, unsigned char *value, unsigned char len)
{
  if(I2C_write_buff(0x6b,reg,value,len) != ESP_OK)
	{
		ESP_LOGW(tag,"qmi8658c_Sends failed\n");
	}
	return 1;
}

unsigned char qmi8658_read_reg(unsigned char reg, unsigned char* buf, unsigned short len)
{
  if(I2C_read_buff(0x6b,reg,buf,len) != ESP_OK)
	{
		ESP_LOGW(tag,"qmi8658c_read failed\n");
	}
  return 1;
}
void qmi8658_delay(unsigned int ms)
{
  vTaskDelay(ms);
}
void qmi8658_delay_us(unsigned int us)
{
	vTaskDelay(us);
}
void qmi8658_axis_convert(float data_a[3], float data_g[3], int layout)
{
	float raw[3],raw_g[3];

	raw[0] = data_a[0];
	raw[1] = data_a[1];
	//raw[2] = data[2];
	raw_g[0] = data_g[0];
	raw_g[1] = data_g[1];
	//raw_g[2] = data_g[2];

	if(layout >=4 && layout <= 7)
	{
		data_a[2] = -data_a[2];
		data_g[2] = -data_g[2];
	}

	if(layout%2)
	{
		data_a[0] = raw[1];
		data_a[1] = raw[0];
		
		data_g[0] = raw_g[1];
		data_g[1] = raw_g[0];
	}
	else
	{
		data_a[0] = raw[0];
		data_a[1] = raw[1];

		data_g[0] = raw_g[0];
		data_g[1] = raw_g[1];
	}

	if((layout==1)||(layout==2)||(layout==4)||(layout==7))
	{
		data_a[0] = -data_a[0];
		data_g[0] = -data_g[0];
	}
	if((layout==2)||(layout==3)||(layout==6)||(layout==7))
	{
		data_a[1] = -data_a[1];
		data_g[1] = -data_g[1];
	}
}

#if defined(QMI8658_USE_CALI)
void qmi8658_data_cali(unsigned char sensor, float data[3])
{
	float data_diff[3];

	if(sensor == 1) 	// accel
	{
		data_diff[0] = QFABS((data[0]-g_cali.acc_last[0]));
		data_diff[1] = QFABS((data[1]-g_cali.acc_last[1]));
		data_diff[2] = QFABS((data[2]-g_cali.acc_last[2]));
		g_cali.acc_last[0] = data[0];
		g_cali.acc_last[1] = data[1];
		g_cali.acc_last[2] = data[2];

//		qmi8658_log("acc diff : %f	", (data_diff[0]+data_diff[1]+data_diff[2]));
		if((data_diff[0]+data_diff[1]+data_diff[2]) < 0.5f)
		{
			if(g_cali.acc_cali_num == 0)
			{
				g_cali.acc_sum[0] = 0.0f;
				g_cali.acc_sum[1] = 0.0f;
				g_cali.acc_sum[2] = 0.0f;
			}
			if(g_cali.acc_cali_num < QMI8658_CALI_DATA_NUM)
			{
				g_cali.acc_cali_num++;
				g_cali.acc_sum[0] += data[0];
				g_cali.acc_sum[1] += data[1];
				g_cali.acc_sum[2] += data[2];
				if(g_cali.acc_cali_num == QMI8658_CALI_DATA_NUM)
				{
					if((g_cali.acc_cali_flag == 0)&&(data[2]<11.8f)&&(data[2]>7.8f))
					{
						g_cali.acc_sum[0] = g_cali.acc_sum[0]/QMI8658_CALI_DATA_NUM;
						g_cali.acc_sum[1] = g_cali.acc_sum[1]/QMI8658_CALI_DATA_NUM;
						g_cali.acc_sum[2] = g_cali.acc_sum[2]/QMI8658_CALI_DATA_NUM;

						g_cali.acc_bias[0] = 0.0f - g_cali.acc_sum[0];
						g_cali.acc_bias[1] = 0.0f - g_cali.acc_sum[1];
						g_cali.acc_bias[2] = 9.807f - g_cali.acc_sum[2];
						g_cali.acc_cali_flag = 1;
					}
					g_cali.imu_static_flag = 1;
					qmi8658_log("qmi8658 acc static!!!\n");
				}
			}

			if(g_cali.imu_static_flag)
			{
				if(g_cali.acc_fix_flag == 0)
				{
					g_cali.acc_fix_flag = 1;
					g_cali.acc_fix[0] = data[0];
					g_cali.acc_fix[1] = data[1];
					g_cali.acc_fix[2] = data[2];
				}
			}
			else
			{
				g_cali.acc_fix_flag = 0;
				g_cali.gyr_fix_flag = 0;
			}
		}
		else
		{
			g_cali.acc_cali_num = 0;
			g_cali.acc_sum[0] = 0.0f;
			g_cali.acc_sum[1] = 0.0f;
			g_cali.acc_sum[2] = 0.0f;

			g_cali.imu_static_flag = 0;
			g_cali.acc_fix_flag = 0;
			g_cali.gyr_fix_flag = 0;
		}

		if(g_cali.acc_fix_flag)
		{
			if(g_cali.acc_fix_index != 0)
				g_cali.acc_fix_index = 0;
			else
				g_cali.acc_fix_index = 1;

			data[0] = g_cali.acc_fix[0] + g_cali.acc_fix_index*0.01f;
			data[1] = g_cali.acc_fix[1] + g_cali.acc_fix_index*0.01f;
			data[2] = g_cali.acc_fix[2] + g_cali.acc_fix_index*0.01f;
		}
		if(g_cali.acc_cali_flag)
		{
			g_cali.acc[0] = data[0] + g_cali.acc_bias[0];
			g_cali.acc[1] = data[1] + g_cali.acc_bias[1];
			g_cali.acc[2] = data[2] + g_cali.acc_bias[2];
			data[0] = g_cali.acc[0];
			data[1] = g_cali.acc[1];
			data[2] = g_cali.acc[2];
		}
		else
		{
			g_cali.acc[0] = data[0];
			g_cali.acc[1] = data[1];
			g_cali.acc[2] = data[2];
		}
	}
	else if(sensor == 2)			// gyroscope
	{
		data_diff[0] = QFABS((data[0]-g_cali.gyr_last[0]));
		data_diff[1] = QFABS((data[1]-g_cali.gyr_last[1]));
		data_diff[2] = QFABS((data[2]-g_cali.gyr_last[2]));
		g_cali.gyr_last[0] = data[0];
		g_cali.gyr_last[1] = data[1];
		g_cali.gyr_last[2] = data[2];
		
//		qmi8658_log("gyr diff : %f	\n", (data_diff[0]+data_diff[1]+data_diff[2]));
		if(((data_diff[0]+data_diff[1]+data_diff[2]) < 0.03f)
			&& ((data[0]>-1.0f)&&(data[0]<1.0f))
			&& ((data[1]>-1.0f)&&(data[1]<1.0f))
			&& ((data[2]>-1.0f)&&(data[2]<1.0f))
			)
		{
			if(g_cali.gyr_cali_num == 0)
			{
				g_cali.gyr_sum[0] = 0.0f;
				g_cali.gyr_sum[1] = 0.0f;
				g_cali.gyr_sum[2] = 0.0f;
			}
			if(g_cali.gyr_cali_num < QMI8658_CALI_DATA_NUM)
			{
				g_cali.gyr_cali_num++;
				g_cali.gyr_sum[0] += data[0];
				g_cali.gyr_sum[1] += data[1];
				g_cali.gyr_sum[2] += data[2];
				if(g_cali.gyr_cali_num == QMI8658_CALI_DATA_NUM)
				{
					if(g_cali.gyr_cali_flag == 0)
					{
						g_cali.gyr_sum[0] = g_cali.gyr_sum[0]/QMI8658_CALI_DATA_NUM;
						g_cali.gyr_sum[1] = g_cali.gyr_sum[1]/QMI8658_CALI_DATA_NUM;
						g_cali.gyr_sum[2] = g_cali.gyr_sum[2]/QMI8658_CALI_DATA_NUM;
			
						g_cali.gyr_bias[0] = 0.0f - g_cali.gyr_sum[0];
						g_cali.gyr_bias[1] = 0.0f - g_cali.gyr_sum[1];
						g_cali.gyr_bias[2] = 0.0f - g_cali.gyr_sum[2];
						g_cali.gyr_cali_flag = 1;
					}
					g_cali.imu_static_flag = 1;
					qmi8658_log("qmi8658 gyro static!!!\n");
				}
			}
			
			if(g_cali.imu_static_flag)
			{
				if(g_cali.gyr_fix_flag == 0)
				{
					g_cali.gyr_fix_flag = 1;
					g_cali.gyr_fix[0] = data[0];
					g_cali.gyr_fix[1] = data[1];
					g_cali.gyr_fix[2] = data[2];
				}
			}
			else
			{
				g_cali.gyr_fix_flag = 0;
				g_cali.acc_fix_flag = 0;
			}
		}
		else
		{
			g_cali.gyr_cali_num = 0;
			g_cali.gyr_sum[0] = 0.0f;
			g_cali.gyr_sum[1] = 0.0f;
			g_cali.gyr_sum[2] = 0.0f;
			
			g_cali.imu_static_flag = 0;
			g_cali.gyr_fix_flag = 0;
			g_cali.acc_fix_flag = 0;
		}

		if(g_cali.gyr_fix_flag)
		{		
			if(g_cali.gyr_fix_index != 0)
				g_cali.gyr_fix_index = 0;
			else
				g_cali.gyr_fix_index = 1;

			data[0] = g_cali.gyr_fix[0] + g_cali.gyr_fix_index*0.00005f;
			data[1] = g_cali.gyr_fix[1] + g_cali.gyr_fix_index*0.00005f;
			data[2] = g_cali.gyr_fix[2] + g_cali.gyr_fix_index*0.00005f;
		}

		if(g_cali.gyr_cali_flag)
		{
			g_cali.gyr[0] = data[0] + g_cali.gyr_bias[0];
			g_cali.gyr[1] = data[1] + g_cali.gyr_bias[1];
			g_cali.gyr[2] = data[2] + g_cali.gyr_bias[2];
			data[0] = g_cali.gyr[0];
			data[1] = g_cali.gyr[1];
			data[2] = g_cali.gyr[2];
		}
		else
		{		
			g_cali.gyr[0] = data[0];
			g_cali.gyr[1] = data[1];
			g_cali.gyr[2] = data[2];
		}
	}
}

#endif

void qmi8658_config_acc(enum qmi8658_AccRange range, enum qmi8658_AccOdr odr, enum qmi8658_LpfConfig lpfEnable, enum qmi8658_StConfig stEnable)
{
	unsigned char ctl_dada;

	switch(range)
	{
		case Qmi8658AccRange_2g:
			g_imu.ssvt_a = (1<<14);
			break;
		case Qmi8658AccRange_4g:
			g_imu.ssvt_a = (1<<13);
			break;
		case Qmi8658AccRange_8g:
			g_imu.ssvt_a = (1<<12);
			break;
		case Qmi8658AccRange_16g:
			g_imu.ssvt_a = (1<<11);
			break;
		default: 
			range = Qmi8658AccRange_8g;
			g_imu.ssvt_a = (1<<12);
	}
	if(stEnable == Qmi8658St_Enable)
		ctl_dada = (unsigned char)range|(unsigned char)odr|0x80;
	else
		ctl_dada = (unsigned char)range|(unsigned char)odr;
		
	qmi8658_write_reg(Qmi8658Register_Ctrl2, ctl_dada);
// set LPF & HPF
	qmi8658_read_reg(Qmi8658Register_Ctrl5, &ctl_dada, 1);
	ctl_dada &= 0xf0;
	if(lpfEnable == Qmi8658Lpf_Enable)
	{
		ctl_dada |= A_LSP_MODE_3;
		ctl_dada |= 0x01;
	}
	else
	{
		ctl_dada &= ~0x01;
	}
	//ctl_dada = 0x00;
	qmi8658_write_reg(Qmi8658Register_Ctrl5,ctl_dada);
// set LPF & HPF
}

void qmi8658_config_gyro(enum qmi8658_GyrRange range, enum qmi8658_GyrOdr odr, enum qmi8658_LpfConfig lpfEnable, enum qmi8658_StConfig stEnable)
{
	// Set the CTRL3 register to configure dynamic range and ODR
	unsigned char ctl_dada; 

	// Store the scale factor for use when processing raw data
	switch (range)
	{
		case Qmi8658GyrRange_16dps:
			g_imu.ssvt_g = 2048;
			break;			
		case Qmi8658GyrRange_32dps:
			g_imu.ssvt_g = 1024;
			break;
		case Qmi8658GyrRange_64dps:
			g_imu.ssvt_g = 512;
			break;
		case Qmi8658GyrRange_128dps:
			g_imu.ssvt_g = 256;
			break;
		case Qmi8658GyrRange_256dps:
			g_imu.ssvt_g = 128;
			break;
		case Qmi8658GyrRange_512dps:
			g_imu.ssvt_g = 64;
			break;
		case Qmi8658GyrRange_1024dps:
			g_imu.ssvt_g = 32;
			break;
		case Qmi8658GyrRange_2048dps:
			g_imu.ssvt_g = 16;
			break;
//		case Qmi8658GyrRange_4096dps:
//			g_imu.ssvt_g = 8;
//			break;
		default: 
			range = Qmi8658GyrRange_512dps;
			g_imu.ssvt_g = 64;
			break;
	}

	if(stEnable == Qmi8658St_Enable)
		ctl_dada = (unsigned char)range|(unsigned char)odr|0x80;
	else
		ctl_dada = (unsigned char)range | (unsigned char)odr;
	qmi8658_write_reg(Qmi8658Register_Ctrl3, ctl_dada);

// Conversion from degrees/s to rad/s if necessary
// set LPF & HPF
	qmi8658_read_reg(Qmi8658Register_Ctrl5, &ctl_dada,1);
	ctl_dada &= 0x0f;
	if(lpfEnable == Qmi8658Lpf_Enable)
	{
		ctl_dada |= G_LSP_MODE_3;
		ctl_dada |= 0x10;
	}
	else
	{
		ctl_dada &= ~0x10;
	}
	//ctl_dada = 0x00;
	qmi8658_write_reg(Qmi8658Register_Ctrl5,ctl_dada);
// set LPF & HPF
}


void qmi8658_send_ctl9cmd(enum qmi8658_Ctrl9Command cmd)
{
	unsigned char	status1 = 0x00;
	unsigned short count=0;

	qmi8658_write_reg(Qmi8658Register_Ctrl9, (unsigned char)cmd);	// write commond to ctrl9
#if 1 //defined(QMI8658_NEW_FIRMWARE)
	unsigned char status_reg = Qmi8658Register_StatusInt;	
	unsigned char cmd_done = 0x80;
	//unsigned char status_reg = Qmi8658Register_Status1;
	//unsigned char cmd_done = 0x01;

	qmi8658_read_reg(status_reg, &status1, 1);
	while(((status1&cmd_done)!=cmd_done)&&(count++<100))		// read statusINT until bit7 is 1
	{
		qmi8658_delay(1);
		qmi8658_read_reg(status_reg, &status1, 1);
	}
	//qmi8658_log("ctrl9 cmd done1 count=%d\n",count);

	qmi8658_write_reg(Qmi8658Register_Ctrl9, qmi8658_Ctrl9_Cmd_NOP);	// write commond  0x00 to ctrl9
	count = 0;
	qmi8658_read_reg(status_reg, &status1, 1);
	while(((status1&cmd_done)==cmd_done)&&(count++<100))		// read statusINT until bit7 is 0
	{
		qmi8658_delay(1);	// 1 ms
		qmi8658_read_reg(status_reg, &status1, 1);
	}
	//qmi8658_log("ctrl9 cmd done2 count=%d\n",count);
#else
	while(((status1&QMI8658_STATUS1_CMD_DONE)==0)&&(count++<100))
	{
		qmi8658_delay(1);
		qmi8658_read_reg(Qmi8658Register_Status1, &status1, sizeof(status1));
	}
#endif

}

unsigned char qmi8658_readStatusInt(void)
{
	unsigned char status_int;

	qmi8658_read_reg(Qmi8658Register_StatusInt, &status_int, 1);

	return status_int;
}

unsigned char qmi8658_readStatus0(void)
{
	unsigned char status0;

	qmi8658_read_reg(Qmi8658Register_Status0, &status0, 1);

	return status0;
}

unsigned char qmi8658_readStatus1(void)
{
	unsigned char status1;
	
	qmi8658_read_reg(Qmi8658Register_Status1, &status1, 1);

	return status1;
}

float qmi8658_readTemp(void)
{
	unsigned char buf[2];
	short temp = 0;
	float temp_f = 0;

	qmi8658_read_reg(Qmi8658Register_Tempearture_L, buf, 2);
	temp = ((short)buf[1]<<8)|buf[0];
	temp_f = (float)temp/256.0f;

	return temp_f;
}

void qmi8658_read_timestamp(unsigned int *tim_count)
{
	unsigned char	buf[3];
	unsigned int timestamp;

	if(tim_count)
	{
		qmi8658_read_reg(Qmi8658Register_Timestamp_L, buf, 3);
		timestamp = (unsigned int)(((unsigned int)buf[2]<<16)|((unsigned int)buf[1]<<8)|buf[0]);
		if(timestamp > g_imu.timestamp)
			g_imu.timestamp = timestamp;
		else
			g_imu.timestamp = (timestamp+0x1000000-g_imu.timestamp);

		*tim_count = g_imu.timestamp;		
	}
}

void qmi8658_read_sensor_data(float acc[3], float gyro[3])
{
	unsigned char	buf_reg[12];
	short 			raw_acc_xyz[3];
	short 			raw_gyro_xyz[3];

	qmi8658_read_reg(Qmi8658Register_Ax_L, buf_reg, 12);
	raw_acc_xyz[0] = (short)((unsigned short)(buf_reg[1]<<8) |( buf_reg[0]));
	raw_acc_xyz[1] = (short)((unsigned short)(buf_reg[3]<<8) |( buf_reg[2]));
	raw_acc_xyz[2] = (short)((unsigned short)(buf_reg[5]<<8) |( buf_reg[4]));

	raw_gyro_xyz[0] = (short)((unsigned short)(buf_reg[7]<<8) |( buf_reg[6]));
	raw_gyro_xyz[1] = (short)((unsigned short)(buf_reg[9]<<8) |( buf_reg[8]));
	raw_gyro_xyz[2] = (short)((unsigned short)(buf_reg[11]<<8) |( buf_reg[10]));

#if defined(QMI8658_UINT_MG_DPS)
	// mg
	acc[0] = (float)(raw_acc_xyz[0]*1000.0f)/g_imu.ssvt_a;
	acc[1] = (float)(raw_acc_xyz[1]*1000.0f)/g_imu.ssvt_a;
	acc[2] = (float)(raw_acc_xyz[2]*1000.0f)/g_imu.ssvt_a;
#else
	// m/s2
	acc[0] = (float)(raw_acc_xyz[0]*ONE_G)/g_imu.ssvt_a;
	acc[1] = (float)(raw_acc_xyz[1]*ONE_G)/g_imu.ssvt_a;
	acc[2] = (float)(raw_acc_xyz[2]*ONE_G)/g_imu.ssvt_a;
#endif

#if defined(QMI8658_UINT_MG_DPS)
	// dps
	gyro[0] = (float)(raw_gyro_xyz[0]*1.0f)/g_imu.ssvt_g;
	gyro[1] = (float)(raw_gyro_xyz[1]*1.0f)/g_imu.ssvt_g;
	gyro[2] = (float)(raw_gyro_xyz[2]*1.0f)/g_imu.ssvt_g;
#else
	// rad/s
	gyro[0] = (float)(raw_gyro_xyz[0]*M_PI)/(g_imu.ssvt_g*180);		// *pi/180
	gyro[1] = (float)(raw_gyro_xyz[1]*M_PI)/(g_imu.ssvt_g*180);
	gyro[2] = (float)(raw_gyro_xyz[2]*M_PI)/(g_imu.ssvt_g*180);
#endif
}

void qmi8658_read_xyz(float acc[3], float gyro[3])
{
	unsigned char	status;
	unsigned char data_ready = 0;

#if defined(QMI8658_SYNC_SAMPLE_MODE)
	qmi8658_read_reg(Qmi8658Register_StatusInt, &status, 1);
	if(status&0x01)
	{
		data_ready = 1;
		qmi8658_delay_us(6);	// delay 6us
	}
#else
	qmi8658_read_reg(Qmi8658Register_Status0, &status, 1);
	if(status&0x03)
	{
		data_ready = 1;
	}
#endif
	if(data_ready)
	{
		qmi8658_read_sensor_data(acc, gyro);
		qmi8658_axis_convert(acc, gyro, 0);
#if defined(QMI8658_USE_CALI)
		qmi8658_data_cali(1, acc);
		qmi8658_data_cali(2, gyro);
#endif
		g_imu.imu[0] = acc[0];
		g_imu.imu[1] = acc[1];
		g_imu.imu[2] = acc[2];
		g_imu.imu[3] = gyro[0];
		g_imu.imu[4] = gyro[1];
		g_imu.imu[5] = gyro[2];
	}
	else
	{
		acc[0] = g_imu.imu[0];
		acc[1] = g_imu.imu[1];
		acc[2] = g_imu.imu[2];
		gyro[0] = g_imu.imu[3];
		gyro[1] = g_imu.imu[4];
		gyro[2] = g_imu.imu[5];
		qmi8658_log("data ready fail!\n");
	}
}


void qmi8658_enableSensors(unsigned char enableFlags)
{
#if defined(QMI8658_SYNC_SAMPLE_MODE)
	qmi8658_write_reg(Qmi8658Register_Ctrl7, enableFlags | 0x80);
#elif defined(QMI8658_USE_FIFO)
	//qmi8658_write_reg(Qmi8658Register_Ctrl7, enableFlags|QMI8658_DRDY_DISABLE);
	qmi8658_write_reg(Qmi8658Register_Ctrl7, enableFlags);
#else
	qmi8658_write_reg(Qmi8658Register_Ctrl7, enableFlags);
#endif
	g_imu.cfg.enSensors = enableFlags&0x03;

	qmi8658_delay(1);
}

void qmi8658_dump_reg(void)
{
	unsigned char read_data[8];

	qmi8658_read_reg(Qmi8658Register_Ctrl1, read_data, 8);
	qmi8658_log("Ctrl1[0x%x]\nCtrl2[0x%x]\nCtrl3[0x%x]\nCtrl4[0x%x]\nCtrl5[0x%x]\nCtrl6[0x%x]\nCtrl7[0x%x]\nCtrl8[0x%x]\n",
					read_data[0],read_data[1],read_data[2],read_data[3],read_data[4],read_data[5],read_data[6],read_data[7]);	
}

//void qmi8658_soft_reset(void)
//{
//	qmi8658_log("qmi8658_soft_reset \n");
//	qmi8658_write_reg(Qmi8658Register_Reset, 0xb0);
//	qmi8658_delay(2000);
//	qmi8658_write_reg(Qmi8658Register_Reset, 0x00);
//	qmi8658_delay(5);
//}

void qmi8658_on_demand_cali(void)
{
	qmi8658_log("qmi8658_on_demand_cali start\n");
	qmi8658_write_reg(Qmi8658Register_Reset, 0xb0);
	qmi8658_delay(10);	// delay
	qmi8658_write_reg(Qmi8658Register_Ctrl9, (unsigned char)qmi8658_Ctrl9_Cmd_On_Demand_Cali);
	qmi8658_delay(2200);	// delay 2000ms above
	qmi8658_write_reg(Qmi8658Register_Ctrl9, (unsigned char)qmi8658_Ctrl9_Cmd_NOP);
	qmi8658_delay(100);	// delay
	qmi8658_log("qmi8658_on_demand_cali done\n");
}

void qmi8658_config_reg(unsigned char low_power)
{
	qmi8658_enableSensors(QMI8658_DISABLE_ALL);
	if(low_power)
	{
		g_imu.cfg.enSensors = QMI8658_ACC_ENABLE;
		g_imu.cfg.accRange = Qmi8658AccRange_8g;
		g_imu.cfg.accOdr = Qmi8658AccOdr_LowPower_21Hz;
		g_imu.cfg.gyrRange = Qmi8658GyrRange_1024dps;
		g_imu.cfg.gyrOdr = Qmi8658GyrOdr_250Hz;
	}
	else
	{		
		g_imu.cfg.enSensors = QMI8658_ACCGYR_ENABLE;
		g_imu.cfg.accRange = Qmi8658AccRange_8g;
		g_imu.cfg.accOdr = Qmi8658AccOdr_250Hz;
		g_imu.cfg.gyrRange = Qmi8658GyrRange_1024dps;
		g_imu.cfg.gyrOdr = Qmi8658GyrOdr_250Hz;
	}
	
	if(g_imu.cfg.enSensors & QMI8658_ACC_ENABLE)
	{
		qmi8658_config_acc(g_imu.cfg.accRange, g_imu.cfg.accOdr, Qmi8658Lpf_Disable, Qmi8658St_Disable);
	}
	if(g_imu.cfg.enSensors & QMI8658_GYR_ENABLE)
	{
		qmi8658_config_gyro(g_imu.cfg.gyrRange, g_imu.cfg.gyrOdr, Qmi8658Lpf_Disable, Qmi8658St_Disable);
	}
}


unsigned char qmi8658_get_id(void)
{
	unsigned char qmi8658_chip_id = 0x00;
	unsigned char qmi8658_revision_id = 0x00;
	unsigned char qmi8658_slave[2] = {QMI8658_SLAVE_ADDR_L, QMI8658_SLAVE_ADDR_H};
	int retry = 0;
	unsigned char iCount = 0;
	unsigned char firmware_id[3];
	unsigned char uuid[6];
	unsigned int uuid_low, uuid_high;

	while(iCount<2)
	{
		g_imu.slave = qmi8658_slave[iCount];
		retry = 0;
		while((qmi8658_chip_id != 0x05)&&(retry++ < 5))
		{
			qmi8658_read_reg(Qmi8658Register_WhoAmI, &qmi8658_chip_id, 1);
			qmi8658_log("Qmi8658Register_WhoAmI = 0x%x\n", qmi8658_chip_id);
		}
		if(qmi8658_chip_id == 0x05)
		{
			qmi8658_on_demand_cali();

			g_imu.cfg.ctrl8_value = 0xc0;
			//QMI8658_INT1_ENABLE, QMI8658_INT2_ENABLE
			qmi8658_write_reg(Qmi8658Register_Ctrl1, 0x60|QMI8658_INT2_ENABLE|QMI8658_INT1_ENABLE);
			qmi8658_read_reg(Qmi8658Register_Revision, &qmi8658_revision_id, 1);			
			qmi8658_read_reg(Qmi8658Register_firmware_id, firmware_id, 3);
			qmi8658_read_reg(Qmi8658Register_uuid, uuid, 6);
			qmi8658_write_reg(Qmi8658Register_Ctrl7, 0x00);
			qmi8658_write_reg(Qmi8658Register_Ctrl8, g_imu.cfg.ctrl8_value);
			uuid_low = (unsigned int)((unsigned int)(uuid[2]<<16)|(unsigned int)(uuid[1]<<8)|(uuid[0]));
			uuid_high = (unsigned int)((unsigned int)(uuid[5]<<16)|(unsigned int)(uuid[4]<<8)|(uuid[3]));
			qmi8658_log("qmi8658_init slave=0x%x Revision=0x%x\n", g_imu.slave, qmi8658_revision_id);
			qmi8658_log("Firmware ID[0x%x 0x%x 0x%x]\n", firmware_id[2], firmware_id[1],firmware_id[0]);
			qmi8658_log("UUID[0x%x %x]\n", uuid_high ,uuid_low);

			break;
		}
		iCount++;
	}

	return qmi8658_chip_id;
}

#if defined(QMI8658_USE_AMD)
void qmi8658_config_amd(void)
{
	g_imu.cfg.ctrl8_value &= (~QMI8658_CTRL8_ANYMOTION_EN);
	qmi8658_write_reg(Qmi8658Register_Ctrl8, g_imu.cfg.ctrl8_value);

	qmi8658_write_reg(Qmi8658Register_Cal1_L, 0x03);		// any motion X threshold U 3.5 first three bit(uint 1g)  last five bit (uint 1/32 g)
	qmi8658_write_reg(Qmi8658Register_Cal1_H, 0x03);		// any motion Y threshold U 3.5 first three bit(uint 1g)  last five bit (uint 1/32 g)
	qmi8658_write_reg(Qmi8658Register_Cal2_L, 0x03);		// any motion Z threshold U 3.5 first three bit(uint 1g)  last five bit (uint 1/32 g)
	qmi8658_write_reg(Qmi8658Register_Cal2_H, 0x02);		// no motion X threshold U 3.5 first three bit(uint 1g)  last five bit (uint 1/32 g)
	qmi8658_write_reg(Qmi8658Register_Cal3_L, 0x02);
	qmi8658_write_reg(Qmi8658Register_Cal3_H, 0x02);

	qmi8658_write_reg(Qmi8658Register_Cal4_L, 0xf7);		// MOTION_MODE_CTRL
	qmi8658_write_reg(Qmi8658Register_Cal4_H, 0x01);		// value 0x01

	qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_Motion);

	qmi8658_write_reg(Qmi8658Register_Cal1_L, 0x03);		// AnyMotionWindow. 
	qmi8658_write_reg(Qmi8658Register_Cal1_H, 0x01);		// NoMotionWindow 
	qmi8658_write_reg(Qmi8658Register_Cal2_L, 0x2c);		// SigMotionWaitWindow[7:0]
	qmi8658_write_reg(Qmi8658Register_Cal2_H, 0x01);		// SigMotionWaitWindow [15:8]
	qmi8658_write_reg(Qmi8658Register_Cal3_L, 0x64);		// SigMotionConfirmWindow[7:0]
	qmi8658_write_reg(Qmi8658Register_Cal3_H, 0x00);		// SigMotionConfirmWindow[15:8]
	//qmi8658_write_reg(Qmi8658Register_Cal4_L, 0xf7);
	qmi8658_write_reg(Qmi8658Register_Cal4_H, 0x02);		// value 0x02

	qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_Motion);
}

void qmi8658_enable_amd(unsigned char enable, enum qmi8658_Interrupt int_map, unsigned char low_power)
{
	if(int_map == qmi8658_Int1)
	{
		g_imu.cfg.ctrl8_value &= (~QMI8658_CTRL8_ANYMOTION_EN);
		g_imu.cfg.ctrl8_value |= QMI8658_CTRL8_DATAVALID_EN;
	}
	else if(int_map == qmi8658_Int2)
	{	
		g_imu.cfg.ctrl8_value &= (~QMI8658_CTRL8_ANYMOTION_EN);
		g_imu.cfg.ctrl8_value &= (~QMI8658_CTRL8_DATAVALID_EN);
	}	
	qmi8658_write_reg(Qmi8658Register_Ctrl8, g_imu.cfg.ctrl8_value);
	qmi8658_delay(2);

	if(enable)
	{
		unsigned char ctrl1;

		qmi8658_enableSensors(QMI8658_DISABLE_ALL);
		qmi8658_config_reg(low_power);

		qmi8658_read_reg(Qmi8658Register_Ctrl1, &ctrl1, 1);
		if(int_map == qmi8658_Int1)
		{
			ctrl1 |= QMI8658_INT1_ENABLE;
			qmi8658_write_reg(Qmi8658Register_Ctrl1, ctrl1);// enable int for dev-E
		}
		else if(int_map == qmi8658_Int2)
		{
			ctrl1 |= QMI8658_INT2_ENABLE;
			qmi8658_write_reg(Qmi8658Register_Ctrl1, ctrl1);// enable int for dev-E
		}
		g_imu.cfg.ctrl8_value |= QMI8658_CTRL8_ANYMOTION_EN;
		qmi8658_write_reg(Qmi8658Register_Ctrl8, g_imu.cfg.ctrl8_value);

		qmi8658_delay(1);
		qmi8658_enableSensors(g_imu.cfg.enSensors);
	}
	else
	{

	}
}
#endif

#if defined(QMI8658_USE_PEDOMETER)
void qmi8658_config_pedometer(unsigned short odr)
{
	float finalRate = (float)(200.0f/odr);  //14.285
	unsigned short ped_sample_cnt = (unsigned short)(0x0032 / finalRate);//6;//(unsigned short)(0x0032 / finalRate) ;
	unsigned short ped_fix_peak2peak = 0x00AC;//0x0006;//0x00CC;
	unsigned short ped_fix_peak = 0x00AC;//0x0006;//0x00CC;
	unsigned short ped_time_up = (unsigned short)(200 / finalRate);
	unsigned char ped_time_low = (unsigned char) (20 / finalRate);
	unsigned char ped_time_cnt_entry = 8;
	unsigned char ped_fix_precision = 0;
	unsigned char ped_sig_count = 1;//�Ʋ�����1

	qmi8658_write_reg(Qmi8658Register_Cal1_L, ped_sample_cnt & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal1_H, (ped_sample_cnt >> 8) & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal2_L, ped_fix_peak2peak & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal2_H, (ped_fix_peak2peak >> 8) & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal3_L, ped_fix_peak & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal3_H, (ped_fix_peak >> 8) & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal4_H, 0x01);
	qmi8658_write_reg(Qmi8658Register_Cal4_L, 0x02);
	qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_EnablePedometer);

	qmi8658_write_reg(Qmi8658Register_Cal1_L, ped_time_up & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal1_H, (ped_time_up >> 8) & 0xFF);
	qmi8658_write_reg(Qmi8658Register_Cal2_L, ped_time_low);
	qmi8658_write_reg(Qmi8658Register_Cal2_H, ped_time_cnt_entry);
	qmi8658_write_reg(Qmi8658Register_Cal3_L, ped_fix_precision);
	qmi8658_write_reg(Qmi8658Register_Cal3_H, ped_sig_count);
	qmi8658_write_reg(Qmi8658Register_Cal4_H, 0x02);
	qmi8658_write_reg(Qmi8658Register_Cal4_L, 0x02);
	qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_EnablePedometer);
}

void qmi8658_enable_pedometer(unsigned char enable)
{
	if(enable)
	{
		g_imu.cfg.ctrl8_value |= QMI8658_CTRL8_PEDOMETER_EN;
	}
	else
	{
		g_imu.cfg.ctrl8_value &= (~QMI8658_CTRL8_PEDOMETER_EN);
	}
	qmi8658_write_reg(Qmi8658Register_Ctrl8, g_imu.cfg.ctrl8_value);
}

unsigned int qmi8658_read_pedometer(void)
{
	unsigned char buf[3];

    qmi8658_read_reg(Qmi8658Register_Pedo_L, buf, 3);	// 0x5a
	g_imu.step = (unsigned int)((buf[2]<<16)|(buf[1]<<8)|(buf[0]));

	return g_imu.step;
}
#endif

#if defined(QMI8658_USE_FIFO)
void qmi8658_config_fifo(unsigned char watermark,enum qmi8658_FifoSize size,enum qmi8658_FifoMode mode,enum qmi8658_Interrupt int_map)
{
	unsigned char ctrl1;

	qmi8658_enableSensors(QMI8658_DISABLE_ALL);
	qmi8658_read_reg(Qmi8658Register_Ctrl1, &ctrl1, 1);
	if(int_map == qmi8658_Int1)
	{
		ctrl1 |= QMI8658_FIFO_MAP_INT1;
	}
	else if(int_map == qmi8658_Int2)
	{
		ctrl1 &= QMI8658_FIFO_MAP_INT2;
	}
	qmi8658_write_reg(Qmi8658Register_Ctrl1, ctrl1);

	g_imu.cfg.fifo_ctrl = (unsigned char)(size | mode);
	qmi8658_write_reg(Qmi8658Register_FifoCtrl, g_imu.cfg.fifo_ctrl);
	qmi8658_write_reg(Qmi8658Register_FifoWmkTh, watermark);

	qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_Rst_Fifo);
	qmi8658_enableSensors(QMI8658_ACCGYR_ENABLE);
}

unsigned short qmi8658_read_fifo(unsigned char* data)
{
	unsigned char fifo_status[2] = {0,0};
	unsigned char fifo_sensors = 1;
	unsigned short fifo_bytes = 0;
	unsigned short fifo_level = 0;
	
	if((g_imu.cfg.fifo_ctrl&0x03)!=qmi8658_Fifo_Bypass)
	{
		//qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_Req_Fifo);

		qmi8658_read_reg(Qmi8658Register_FifoCount, fifo_status, 2);
		fifo_bytes = (unsigned short)(((fifo_status[1]&0x03)<<8)|fifo_status[0]);
		if((g_imu.cfg.enSensors == QMI8658_ACC_ENABLE)||(g_imu.cfg.enSensors == QMI8658_GYR_ENABLE))
		{
			fifo_sensors = 1;
		}
		else if(g_imu.cfg.enSensors == QMI8658_ACCGYR_ENABLE)
		{
			fifo_sensors = 2;
		}
		fifo_level = fifo_bytes/(3*fifo_sensors);
		fifo_bytes = fifo_level*(6*fifo_sensors);
		//qmi8658_log("fifo-level : %d\n", fifo_level);
		if(fifo_level > 0)
		{	
			qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_Req_Fifo);
#if 1
			for(int i=0; i<fifo_level; i++)
			{
				qmi8658_read_reg(Qmi8658Register_FifoData, &data[i*fifo_sensors*6], fifo_sensors*6);
			}
#else
			qmi8658_read_reg(Qmi8658Register_FifoData, data, fifo_bytes);
#endif
			qmi8658_write_reg(Qmi8658Register_FifoCtrl, g_imu.cfg.fifo_ctrl);
		}
		qmi8658_send_ctl9cmd(qmi8658_Ctrl9_Cmd_Rst_Fifo);
	}

	return fifo_level;
}
#endif

#if defined(QMI8658_SOFT_SELFTEST)
unsigned char qmi8658_do_selftest(void)
{
	float acc[3], gyr[3];
	unsigned char status;
	unsigned short st_count, retry, imu_fail_count;
	float norm_acc, norm_gyo;

	imu_fail_count = 0;
	st_count = 0;
	qmi8658_delay(50);
	while(st_count++ < 20)
	{
		qmi8658_delay(1);
		status = 0;
		retry = 0;
		while(!(status&0x03) && (retry++<50))
		{
			qmi8658_read_reg(Qmi8658Register_Status0, &status, 1);
			qmi8658_delay(1);
		}
		if((status&0x03))
		{
			qmi8658_read_sensor_data(acc, gyr);
			norm_acc = acc[0]*acc[0] + acc[1]*acc[1] + acc[2]*acc[2];
			norm_gyo = gyr[0]*gyr[0] + gyr[1]*gyr[1] + gyr[2]*gyr[2];
			qmi8658_log("qmi8658_do_selftest-%d %f	%f\n", st_count, norm_acc, norm_gyo);
			if((norm_acc < 0.01f) || (norm_gyo < 0.000001f))
			{
				imu_fail_count++;
			}
		}
		else
		{
			imu_fail_count++;
		}
	}

	if(imu_fail_count > 15)
	{
		qmi8658_log("qmi8658_do_selftest-fail\n");
		return 0;
	}
	else
	{
		qmi8658_log("qmi8658_do_selftest-ok\n");
		return 1;
	}
}
#endif

unsigned char qmi8658_init(void)
{
	if(qmi8658_get_id() == 0x05)
	{
#if defined(QMI8658_USE_AMD)
		qmi8658_config_amd();
#endif
#if defined(QMI8658_USE_PEDOMETER)
		qmi8658_config_pedometer(125);
		qmi8658_enable_pedometer(1);
#endif
		qmi8658_config_reg(0);
		qmi8658_enableSensors(g_imu.cfg.enSensors);
		qmi8658_dump_reg();
#if defined(QMI8658_USE_CALI)
		memset(&g_cali, 0, sizeof(g_cali));
#endif
		return 1;
	}
	else
	{
		qmi8658_log("qmi8658_init fail\n");
		return 0;
	}
}


/*qmi8658c-example*/
void qmi8658c_example(void* parmeter)
{
  float acc[3];
  float gyro[3];
  float temp;
  qmi8658_init();
	vTaskDelay(pdMS_TO_TICKS(1000));
  for (;;)
  {
    qmi8658_read_xyz(acc,gyro);
    temp = qmi8658_readTemp();
    printf("x:%f y:%f z:%f gyro_x:%f gyro_y:%f gyro_z:%f Temp:%f\n",acc[0],acc[1],acc[2],gyro[0],gyro[1],gyro[2],temp);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}