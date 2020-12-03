/***************************************************************************
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *    File  	: focaltech_self_diagnostic.c
 *    Author(s)   :
 *    Description :
 *
 ***************************************************************************/
#define LGTP_MODULE "[FT8707_SELFD]"

/****************************************************************************
* Include Files
****************************************************************************/
#include <linux/input/unified_driver_4/lgtp_common.h>
#include <linux/input/unified_driver_4/lgtp_common_driver.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_i2c.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_misc.h>
#include <linux/input/unified_driver_4/lgtp_device_ft8707.h>



#if defined(TOUCH_MODEL_K7)
/* cut 1.1 */
static int RAW_DATA_MAX = 20000;
static int RAW_DATA_MIN = 2500;
static int RAW_DATA_MARGIN = 950;
static int IB_MAX = 300;
static int IB_MIN = 0;
static int NOISE_MAX = 300;
static int NOISE_MIN = 0;
static int DELTA_MAX = 12000;
static int DELTA_MIN = -999;
static int LPWG_RAW_DATA_MAX = 20000;
static int LPWG_RAW_DATA_MIN = 2500;
static int LPWG_IB_MAX = 300;
static int LPWG_IB_MIN = 0;
static int LPWG_NOISE_MAX = 300;
static int LPWG_NOISE_MIN = 0;
static int P2P_NOISE_MAX = 9999;
static int P2P_NOISE_MIN = 0;
static int LPWG_P2P_NOISE_MAX = 9999;
static int LPWG_P2P_NOISE_MIN = 0;
static int SHORT_MAX = 9999;
static int SHORT_MIN = 0;
/* cut 1.0 */
static int RAW_DATA_MAX_CUT1 = 20000;
static int RAW_DATA_MIN_CUT1 = 2500;
static int RAW_DATA_MARGIN_CUT1 = 950;
static int IB_MAX_CUT1 = 130;
static int IB_MIN_CUT1 = 0;
static int NOISE_MAX_CUT1 = 130;
static int NOISE_MIN_CUT1 = 0;
static int DELTA_MAX_CUT1 = 12000;
static int DELTA_MIN_CUT1 = -999;
static int LPWG_RAW_DATA_MAX_CUT1 = 20000;
static int LPWG_RAW_DATA_MIN_CUT1 = 2500;
static int LPWG_IB_MAX_CUT1 = 130;
static int LPWG_IB_MIN_CUT1 = 0;
static int LPWG_NOISE_MAX_CUT1 = 130;
static int LPWG_NOISE_MIN_CUT1 = 0;
static int P2P_NOISE_MAX_CUT1 = 9999;
static int P2P_NOISE_MIN_CUT1 = 0;
static int LPWG_P2P_NOISE_MAX_CUT1 = 9999;
static int LPWG_P2P_NOISE_MIN_CUT1 = 0;
static int SHORT_MAX_CUT1 = 9999;
static int SHORT_MIN_CUT1 = 0;
#endif
static int chip_rev = FT_CHIP_CUT_1;

/****************************************************************************
* Extern Function Prototypes
****************************************************************************/
extern int lge_get_lcm_revision(void);

/****************************************************************************
* Type Definitions
****************************************************************************/

/****************************************************************************
* Variables
****************************************************************************/
static s16 fts_data[MAX_ROW][MAX_COL];
static u16 fts_data_rms[MAX_ROW][MAX_COL];
static u8 i2c_data[MAX_COL*MAX_ROW*2];
int max_data = 0;
int min_data = 0;

int FT8707_change_mode(struct i2c_client *client, const u8 mode)
{
	int i = 0;
	u8 result;
	u8 addr;
	u8 data;

	TOUCH_LOG("%s : mode=0x%x\n", __func__, mode);

	addr = 0x00;
	data = 0x00;
	fts_read_reg(client, addr, &data);
	if (data == mode) {
		return TOUCH_SUCCESS;
	}

	addr = 0x00;
	data = mode;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(10); //mdelay(100);

	for ( i = 0; i < FTS_MODE_CHANGE_LOOP; i++) {
		addr = 0x00;
		data = 0x00;
		fts_read_reg(client, addr, &data);
		if(data == mode)
			break;
		mdelay(50);
	}

	if (i >= FTS_MODE_CHANGE_LOOP) {
		TOUCH_ERR("FTS_MODE_CHANGE_LOOP ERR\n");
		return TOUCH_FAIL;
	}

	mdelay(300); //mdelay(200);
	return TOUCH_SUCCESS;
}


static int FT8707_Rawdata_Test(struct i2c_client *client)
{
	int i = 0;
	int j = 0;
	int k = 0;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result= 0x00;
	int ret = 0;
	u8 scan_mode = FALSE;

	TOUCH_LOG("[Test Item]  Raw data Test \n");
	memset(i2c_data, 0, sizeof(i2c_data));

	addr = 0x06;
	data = 0x00;
	result = fts_write_reg(client, addr, data);  // To Select data Mode to Raw data
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Write ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Channel number check */
	addr = 0x02;
	fts_read_reg(client, addr, &data);
	mdelay(3);
	if (data != MAX_COL) {
		TOUCH_ERR("X Channel : %d \n", data);
		return TOUCH_FAIL;
	}

	addr = 0x03;
	fts_read_reg(client, addr, &data);
	mdelay(3);
	if (data != MAX_ROW) {
		TOUCH_ERR("Y Channel : %d \n", data);
		return TOUCH_FAIL;
	}

	/* Start SCAN */
	addr = 0x00;
	fts_read_reg(client, addr, &data);

	data |= 0x80; // 0x40|0x80
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(10);
	TOUCH_DBG("SCAN command write success\n");

	for (i = 0; i < 100; i++) {
		addr = 0x00;
		fts_read_reg(client, addr, &data);
		if (data == 0x40) {
			TOUCH_DBG("SCAN Success : 0x%X, waiting %d times \n", data, i);
			scan_mode = TRUE;
			break;
		} else {
			scan_mode = FALSE;
			mdelay(20);
		}
	}

	if(scan_mode == FALSE) {
		TOUCH_ERR("SCAN FAIL :NG :0x%X, retry %d times \n", data, i);
		return TOUCH_FAIL;
	}

	/* clear Raw data buffer*/
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	// read full data at once
	addr = 0x6A;
	result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	TOUCH_DBG("Raw data read success \n");

	/* Combine */
	for (i = 0; i < MAX_ROW; i++) {
		for(j = 0; j < MAX_COL; j++) {
			k = ((j * MAX_ROW)+i)<<1;
			fts_data[i][j] = (i2c_data[k] << 8) + i2c_data[k+1];
		}
	}

	return TOUCH_SUCCESS;
}


static int FT8707_IB_Test(struct i2c_client *client)
{
	int i, j;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result = 0x00;
	int read_len;

	TOUCH_LOG("[Test Item]  IB data Test \n");
	memset( i2c_data,0, sizeof(i2c_data));

	/* read IB data */
	for (i = 0; (MAX_COL*MAX_ROW - TEST_PACKET_LENGTH*i) > 0; i++)
	{
		addr = 0x18;
		data = ((TEST_PACKET_LENGTH*i)&0xFF00)>>8;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) return TOUCH_FAIL;
		mdelay(10);

		addr = 0x19;
		data = (TEST_PACKET_LENGTH*i)&0x00FF;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) return TOUCH_FAIL;
		mdelay(10);

		addr = 0x6E;
		read_len = ((MAX_COL*MAX_ROW - TEST_PACKET_LENGTH*i) >= TEST_PACKET_LENGTH) ? TEST_PACKET_LENGTH : (MAX_COL*MAX_ROW - TEST_PACKET_LENGTH*i);
		result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[TEST_PACKET_LENGTH*i], read_len);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("Read ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(10);

		TOUCH_DBG("%d: IbAddrH=0x%02X  IbAddrL=0x%02X : %d bytes read success... remain:%d \n",
			i, ((TEST_PACKET_LENGTH*i)&0xFF00)>>8, (TEST_PACKET_LENGTH*i)&0x00FF,
			read_len, (MAX_COL*MAX_ROW - TEST_PACKET_LENGTH*i - read_len));
	}

	for (i = 0; i < MAX_ROW; i++) {
		for (j = 0; j < MAX_COL; j++) {
			fts_data[i][j] = i2c_data[j * MAX_COL + i];
		}
	}

	return TOUCH_SUCCESS;
}

static int FT8707_Noise_Test(struct i2c_client *client)
{

	int i, j, k;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result = 0x00;
	int frame_count = 0;

	TOUCH_LOG("[Test Item]  Noise data Test \n");
	memset( i2c_data,0, sizeof(i2c_data));

	/* Select is differ */
	addr = 0x06;
	data = 0x01;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("fts_write_reg ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Clear Data buffer for RMS*/
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Set test frame count(50 frames) */
	addr = 0x12;
	data = 0x32;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("fts_write_reg ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* to start RMS noise test */
	addr = 0x11;
	data = 0x01;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("fts_write_reg ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Polling 0x11 to make sure it is 0, 0 means RMS and P2P both are finished */

	for(i=0;i< 100; i++){

		fts_read_reg(client, addr, &data);
		if (data == 0x00){
			TOUCH_DBG("check Success : 0x%X, waiting %d times \n", data, i);
			break;  // noist test end
		}
		mdelay(50);	//mdelay(20);
	}

	if (i >= 100){ // NOISE TEST SCAN COMMAND FAIL
		TOUCH_ERR("NOISE TEST SCAN COMMAND FAIL\n");
		return TOUCH_FAIL;
	}

	/* Read out real test frame count for RMS noise */
	addr = 0x13;
	result = fts_read_reg(client, addr, &data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	frame_count = data;

	mdelay(100);
	TOUCH_LOG(" Frame count : %d\n",frame_count);

	addr = 0x6A;
	result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("FT8707_I2C_Read ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Clear Data buffer for RMS*/
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	for (i = 0; i < MAX_ROW; i++) {
		for (j = 0; j < MAX_COL; j++) {
		//STDEV
			k = ((j * MAX_COL)+i)<<1;
			fts_data_rms[i][j] = (((i2c_data[k]<<8) + i2c_data[k+1]))/frame_count;
			fts_data_rms[i][j] = (u16)int_sqrt((unsigned long)fts_data_rms[i][j]);
		}
	}

	return TOUCH_SUCCESS;
}

static int FT8707_P2P_Noise_Test(struct i2c_client *client)
{
	int i, j, k;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result= 0x00;
	int frame_count = 0;

	TOUCH_LOG("[Test Item]  Peak to Peak Noise Test \n");
	memset(i2c_data, 0, sizeof(i2c_data));

	/* Select is differ */
	addr = 0x06;
	data = 0x01;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("fts_write_reg ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Clear Data buffer for P2P*/
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Set test frame count(50 frames) */
	addr = 0x12;
	data = 0x32;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("fts_write_reg ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* to start P2P noise test */
	addr = 0x11;
	data = 0x01;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("fts_write_reg ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Polling 0x11 to make sure it is 0, 0 means RMS and P2P both are finished */
	for(i=0;i< 100; i++){
		fts_read_reg(client, addr, &data);
		if (data == 0x00){
			TOUCH_DBG("check Success : 0x%X, waiting %d times \n", data, i);
			break;  // noist test end
		}
		mdelay(50);	//mdelay(20);
	}

	if (i >= 100){ // NOISE TEST SCAN COMMAND FAIL
		TOUCH_ERR("NOISE TEST SCAN COMMAND FAIL\n");
		return TOUCH_FAIL;
	}

	/* Read out real test frame count for P2P noise */
	addr = 0x13;
	result = fts_read_reg(client, addr, &data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	frame_count = data;

	mdelay(10);
	TOUCH_LOG(" Frame count : %d\n",frame_count);

	// read full data at once
	addr = 0x8B;
	result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	TOUCH_DBG("P2P noise data read success \n");

	/* Clear Data buffer */
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* Combine */
	for (i = 0; i < MAX_ROW; i++) {
		for(j = 0; j < MAX_COL; j++) {
			k = ((j * MAX_ROW)+i)<<1;
			fts_data[i][j] = (i2c_data[k] << 8) + i2c_data[k+1];
		}
	}

	return TOUCH_SUCCESS;
}
static int FT8707_Short_Test(struct i2c_client *client)
{
	int i, j, k;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result= 0x00;

	TOUCH_LOG("[Test Item]  Short Test \n");
	memset(i2c_data, 0, sizeof(i2c_data));

	/* enable short test function */
	addr = 0x0F;
	data = 0x01;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(100);


	/* Check Test Completion */
	addr = 0x10;
	for (i = 0; i < 100; i++) {
		result = fts_read_reg(client, addr, &data);
		if (result < 0) {
			TOUCH_ERR("i2c error\n");
			return result;
		}
		if(data == 0x00) {
			TOUCH_DBG("check Success : 0x%X, waiting %d times \n", data, i);
			break;
		}
		mdelay(20);
	}

	if (i >= 100) {
		TOUCH_ERR("Short Test Fail : 0x%X, %d ms \n", data, i*20);
		return TOUCH_FAIL;
	}

	/* get full data */
	addr = 0x89;
	result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	/* Combine */
	for (i = 0; i < MAX_ROW; i++) {
		for(j = 0; j < MAX_COL; j++) {
			k = ((j * MAX_ROW)+i)<<1;
			fts_data[i][j] = (i2c_data[k] << 8) + i2c_data[k+1];
		}
	}

	return TOUCH_SUCCESS;

}

static int FT8707_Delta_Test(struct i2c_client *client)
{
	int i, j, k;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result= 0x00;
	int ret = 0;
	u8 scan_mode = FALSE;
	int is_delta_ok_test;
	int read_len;

	TOUCH_LOG("[Test Item]  delta Test \n");
	memset(i2c_data, 0, sizeof(i2c_data));

	//result = FT8707_change_mode(client, FTS_FACTORY_MODE);

	// To Select data Mode to delta
	addr = 0x06;
	data = 0x01;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(100);

	/* clear delta buffer */
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	/* Channel number check */
	addr = 0x02;
	fts_read_reg(client, addr, &data);
	mdelay(3);
	if (data != MAX_COL) {
		TOUCH_ERR("X Channel : %d \n", data);
		return TOUCH_FAIL;
	}

	addr = 0x03;
	fts_read_reg(client, addr, &data);
	mdelay(3);
	if (data != MAX_ROW) {
		TOUCH_ERR("Y Channel : %d \n", data);
		return TOUCH_FAIL;
	}


	/* Start SCAN */
	for (k = 0; k < 3; k++)
	{
		addr = 0x00;
		fts_read_reg(client, addr, &data);

		data |= 0x80; // 0x40|0x80
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("write ERR \n");
			return TOUCH_FAIL;
		}
		mdelay(10);
		TOUCH_DBG("SCAN command write success\n");

		for (i = 0; i < 100; i++) {
			addr = 0x00;
			fts_read_reg(client, addr, &data);
			if (data == 0x40) {
				TOUCH_DBG("SCAN Success : 0x%X, waiting %d times \n", data, i);
				scan_mode = TRUE;
				break;
			} else {
				scan_mode = FALSE;
				mdelay(20);
			}
		}

		if(scan_mode == TRUE)
			break;
		else
			TOUCH_ERR("SCAN fail : 0x%X, retry %d times \n", data, k);
	}

	if(scan_mode == FALSE) {
		TOUCH_ERR("SCAN FAIL :NG :0x%X, retry %d times \n", data, k);
		return TOUCH_FAIL;
	}

	// read full data at once
	addr = 0x6A;
	result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	TOUCH_DBG("Raw data read success \n");

		/* clear delta buffer */
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	/* Combine */
	for (i = 0; i < MAX_ROW; i++) {
		for(j = 0; j < MAX_COL; j++) {
			k = ((j * MAX_ROW)+i)<<1;
			fts_data[i][j] = (i2c_data[k] << 8) + i2c_data[k+1];
		}
	}

	return TOUCH_SUCCESS;

}
static int FT8707_DoTest(struct i2c_client *client, int type)
{
	int result = 0;

	TOUCH_FUNC();
	TOUCH_LOG("Do Test type = %d \n", type);

	if(result != TOUCH_SUCCESS)
		return TOUCH_FAIL;

	switch(type){
		case RAW_DATA_SHOW:
			TOUCH_LOG("=== Rawdata image ===\n");
			result = FT8707_Rawdata_Test(client);
			break;
		case IB_SHOW:
			TOUCH_LOG("=== IB image ===\n");
			result = FT8707_IB_Test(client);
			break;
		case NOISE_SHOW:
			TOUCH_LOG("=== Noise image ===\n");
			result = FT8707_Noise_Test(client);
			break;

		case P2P_NOISE_SHOW:
			TOUCH_LOG("=== Peak to Peak Noise image ===\n");
			result = FT8707_P2P_Noise_Test(client);
			break;
		case SHORT_SHOW:
			TOUCH_LOG("=== Short image ===\n");
			result = FT8707_Short_Test(client);
			break;
		case DELTA_SHOW:

			TOUCH_LOG("=== Delta image ===\n");
			result = FT8707_Delta_Test(client);
			break;
		case LPWG_RAW_DATA_SHOW:
			TOUCH_LOG("=== LPWG Rawdata image ===\n");
			result = FT8707_Rawdata_Test(client);
			break;
		case LPWG_IB_SHOW:
			TOUCH_LOG("=== LPWG IB image ===\n");
			result = FT8707_IB_Test(client);
			break;
		case LPWG_NOISE_SHOW:
			TOUCH_LOG("=== LPWG Noise image ===\n");
			result = FT8707_Noise_Test(client);
			break;
		case LPWG_P2P_NOISE_SHOW:
			TOUCH_LOG("=== LPWG Peak to Peak Noise image ===\n");
			result = FT8707_P2P_Noise_Test(client);
			break;
		default:
			TOUCH_ERR("type select error, type : %d", type);
			break;
	}

	TOUCH_LOG("%s [DONE]\n", __func__);
	return result;
}

static int FT8707_PrintData(struct i2c_client *client, char *buf, int* result, int type)
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int limit_upper = 0;
	int limit_lower = 0;
	int error_count = 0;
	int eachResult = TOUCH_SUCCESS;

	TOUCH_FUNC();

	min_data = fts_data[0][0];
	max_data = fts_data[0][0];

	chip_rev = lge_get_lcm_revision();
	TOUCH_LOG("Chip rev: %d\n",chip_rev);

	switch (type) {
		case RAW_DATA_SHOW:
		TOUCH_LOG("Rawdata Result\n");
		ret += sprintf(buf + ret,"Rawdata Result\n");
		TOUCH_LOG("Raw Data Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "Raw Data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = RAW_DATA_MAX_CUT1 + RAW_DATA_MARGIN_CUT1;
			limit_lower = RAW_DATA_MIN_CUT1 - RAW_DATA_MARGIN_CUT1;
		}
		else{
			limit_upper = RAW_DATA_MAX + RAW_DATA_MARGIN;
			limit_lower = RAW_DATA_MIN - RAW_DATA_MARGIN;
		}

		break;
		case IB_SHOW:
		TOUCH_LOG("IB Result\n");
		ret += sprintf(buf + ret,"IB Result\n");
		TOUCH_LOG("IB data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "IB data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = IB_MAX_CUT1;
			limit_lower = IB_MIN_CUT1;
		}
		else{
			limit_upper = IB_MAX;
			limit_lower = IB_MIN;
		}
		break;
		case NOISE_SHOW:
		TOUCH_LOG("Noise Result\n");
		ret += sprintf(buf + ret,"Noise Result\n");
		TOUCH_LOG("Noise data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "Noise data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = NOISE_MAX_CUT1;
			limit_lower = NOISE_MIN_CUT1;
		}
		else{
			limit_upper = NOISE_MAX;
			limit_lower = NOISE_MIN;
		}
		break;
		case P2P_NOISE_SHOW:
		TOUCH_LOG("Peak to Peak Noise Result\n");
		ret += sprintf(buf + ret,"Peak to Peak Noise Result\n");
		TOUCH_LOG("Peak to Peak Noise data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "Peak to Peak Noise data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = P2P_NOISE_MAX_CUT1;
			limit_lower = P2P_NOISE_MIN_CUT1;
		}
		else{
			limit_upper = P2P_NOISE_MAX;
			limit_lower = P2P_NOISE_MIN;
		}
		break;
		case SHORT_SHOW:
		TOUCH_LOG("Short Result\n");
		ret += sprintf(buf + ret,"Short Result\n");
		TOUCH_LOG("Short data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "Short data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = SHORT_MAX_CUT1;
			limit_lower = SHORT_MIN_CUT1;
		}
		else{
			limit_upper = SHORT_MAX;
			limit_lower = SHORT_MIN;
		}
		break;
		case DELTA_SHOW:
		TOUCH_LOG("Delta Result\n");
		ret += sprintf(buf + ret,"Delta Result\n");
		TOUCH_LOG("Delta data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "Delta data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = DELTA_MAX_CUT1;
			limit_lower = DELTA_MIN_CUT1;
		}
		else{
			limit_upper = DELTA_MAX;
			limit_lower = DELTA_MIN;
		}
		break;
		case LPWG_RAW_DATA_SHOW:
		TOUCH_LOG("LPWG Rawdata Result\n");
		ret += sprintf(buf + ret,"LPWG Rawdata Result\n");
		TOUCH_LOG("LPWG Raw Data Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG Raw Data Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = LPWG_RAW_DATA_MAX_CUT1 + RAW_DATA_MARGIN_CUT1;
			limit_lower = LPWG_RAW_DATA_MIN_CUT1 - RAW_DATA_MARGIN_CUT1;
		}
		else{
			limit_upper = LPWG_RAW_DATA_MAX + RAW_DATA_MARGIN;
			limit_lower = LPWG_RAW_DATA_MIN - RAW_DATA_MARGIN;
		}
		break;
		case LPWG_IB_SHOW:
		TOUCH_LOG("LPWG IB Result\n");
		ret += sprintf(buf + ret,"LPWG IB Result\n");
		TOUCH_LOG("LPWG IB Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG IB Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = LPWG_IB_MAX_CUT1;
			limit_lower = LPWG_IB_MIN_CUT1;
		}
		else{
			limit_upper = LPWG_IB_MAX;
			limit_lower = LPWG_IB_MIN;
		}
		break;
		case LPWG_NOISE_SHOW:
		TOUCH_LOG("LPWG Noise Result\n");
		ret += sprintf(buf + ret,"LPWG Noise Result\n");
		TOUCH_LOG("LPWG Noise Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG Noise Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = LPWG_NOISE_MAX_CUT1;
			limit_lower = LPWG_NOISE_MIN_CUT1;
		}
		else{
			limit_upper = LPWG_NOISE_MAX;
			limit_lower = LPWG_NOISE_MIN;
		}
		break;
		case LPWG_P2P_NOISE_SHOW:
		TOUCH_LOG("LPWG Peak to Peak Noise Result\n");
		ret += sprintf(buf + ret,"LPWG Peak to Peak Noise Result\n");
		TOUCH_LOG("LPWG Peak to Peak Noise Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG Peak to Peak Noise Read --------------------------------------- \n");
		if(chip_rev == FT_CHIP_CUT_1){
			limit_upper = LPWG_P2P_NOISE_MAX_CUT1;
			limit_lower = LPWG_P2P_NOISE_MIN_CUT1;
		}
		else{
			limit_upper = LPWG_P2P_NOISE_MAX;
			limit_lower = LPWG_P2P_NOISE_MIN;
		}
		break;
		default:
		return TOUCH_FAIL;
	}

	if(type == NOISE_SHOW || type == LPWG_NOISE_SHOW) {
		for (i = 0; i < MAX_ROW; i++) {
			ret += sprintf(buf + ret, "[%2d] ", i+1);
			printk("[%2d] ", i+1);
			for (j = 0; j < MAX_COL; j++) {
				if (fts_data_rms[i][j] >= limit_lower && fts_data_rms[i][j] <= limit_upper) {
					ret += sprintf(buf + ret, "%5d ", fts_data_rms[i][j]);
					printk("%5d", fts_data_rms[i][j]);
				} else {
					ret += sprintf(buf + ret,"%5d ", fts_data_rms[i][j]);
					printk("%5d", fts_data_rms[i][j]);
					error_count++;
					eachResult |= TOUCH_FAIL;
				}

				min_data = (min_data > fts_data_rms[i][j]) ? fts_data_rms[i][j] : min_data;
				max_data = (max_data < fts_data_rms[i][j]) ? fts_data_rms[i][j] : max_data;
			}

			ret += sprintf(buf + ret, "\n");
			printk("\n");
		}
	} else {
		for (i = 0; i < MAX_ROW; i++) {
			ret += sprintf(buf + ret, "[%2d] ", i+1);
			printk("[%2d] ", i+1);
			for (j = 0; j < MAX_COL; j++) {
				if (fts_data[i][j] >= limit_lower && fts_data[i][j] <= limit_upper) {
					ret += sprintf(buf + ret, "%5d ", fts_data[i][j]);
					printk("%5d", fts_data[i][j]);
				} else {
					ret += sprintf(buf + ret,"%5d ", fts_data[i][j]);
					printk("%5d", fts_data[i][j]);
					error_count++;
					eachResult |= TOUCH_FAIL;
				}

				min_data = (min_data > fts_data[i][j]) ? fts_data[i][j] : min_data;
				max_data = (max_data < fts_data[i][j]) ? fts_data[i][j] : max_data;
			}

			ret += sprintf(buf + ret, "\n");
			printk("\n");
		}
	}



	printk("\n");
	printk("------------------------------------------------------- \n");
	ret += sprintf(buf + ret, "------------------------------------------------------- \n");

	if (eachResult == TOUCH_FAIL){
		ret += sprintf(buf + ret, "TEST FAIL!! This is %d error in test with '!' character\n", error_count);
        TOUCH_LOG("TEST FAIL!! min_data:%d, max_data:%d\n",min_data,max_data);
	}else{
		ret += sprintf(buf + ret, "TEST PASS!!");
        TOUCH_LOG("TEST PASS!! min_data:%d, max_data:%d\n",min_data,max_data);
	}
	*result |= eachResult;
	//*result = TOUCH_SUCCESS;
	//TODO: Compare to limitation spec & determine result test
	ret += sprintf(buf + ret,"MAX = %d,  MIN = %d  (MAX - MIN = %d), Spec rower : %d, upper : %d\n\n",max_data, min_data, max_data - min_data, limit_lower, limit_upper);

	return ret;
}

ssize_t FT8707_GetTestResult(struct i2c_client *client, char *pBuf, int *result, int type)
{
	int i;
	int ret = 0;
	int retry_count = 0;
	int retry_max = 1;

	TOUCH_FUNC();
	for ( i = 0 ; i < MAX_ROW ; i++) {
		memset(fts_data[i], 0, sizeof(s16) * MAX_COL);
	}

	retry_count = 0;
	while (retry_count++ < retry_max) {
		if (FT8707_DoTest(client, type) == TOUCH_FAIL) {
			TOUCH_ERR("Getting data failed, retry (%d/%d)\n", retry_count, retry_max);
			FT8707_Reset(0,10);
			FT8707_Reset(1,100);
			mdelay(150);
			FT8707_change_mode(client, FTS_FACTORY_MODE);
		} else { break; }

			if (retry_count >= retry_max) {
				TOUCH_ERR("%s all retry failed \n", __func__);
				goto ERROR;
			}
		}

		ret += FT8707_PrintData(client, pBuf, result, type);
		if (ret < 0) {
			TOUCH_ERR("fail to print type data\n");
			goto ERROR;
		}

		return ret;

		ERROR:
		*result = TOUCH_FAIL;
		return TOUCH_FAIL;
	}
