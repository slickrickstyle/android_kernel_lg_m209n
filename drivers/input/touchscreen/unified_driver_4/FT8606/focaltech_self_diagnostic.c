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
#define LGTP_MODULE "[FT8606_SELFD]"

/****************************************************************************
* Include Files
****************************************************************************/
#include <linux/input/unified_driver_4/lgtp_common.h>
#include <linux/input/unified_driver_4/lgtp_common_driver.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_i2c.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_misc.h>
#include <linux/input/unified_driver_4/lgtp_device_ft8606.h>

#define RAW_DATA_MAX 12000
#define RAW_DATA_MIN 2000
#define RAW_DATA_MARGIN 0
#define IB_MAX 48
#define IB_MIN 6
#define NOISE_MAX 100
#define NOISE_MIN 0
#define LPWG_RAW_DATA_MAX 12000
#define LPWG_RAW_DATA_MIN 2000
#define LPWG_IB_MAX 48
#define LPWG_IB_MIN 6
#define LPWG_NOISE_MAX 100
#define LPWG_NOISE_MIN 0
/****************************************************************************
* Type Definitions
****************************************************************************/

/****************************************************************************
* Variables
****************************************************************************/
static s16 fts_data[MAX_ROW][MAX_COL];
static u8 i2c_data[MAX_COL*MAX_ROW*2];
int max_data = 0;
int min_data = 0;

int FT8606_change_mode(struct i2c_client *client, const u8 mode)
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

static int FT8606_Rawdata_Test(struct i2c_client *client)
{
	int i, j, k;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result= 0x00;
	u8 scan_mode = false;
	int is_rawdata_ok_test;

	TOUCH_LOG("[Test Item]  Raw data Test \n");
	memset(i2c_data, 0, sizeof(i2c_data));

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

		for (i = 0; i < 200; i++) {
			addr = 0x00;
			fts_read_reg(client, addr, &data);
			if (data == 0x40) {
				TOUCH_DBG("SCAN Success : 0x%X, waiting %d times \n", data, i);
				scan_mode = true;
				break;
			} else {
				scan_mode = false;
				mdelay(20);
			}
		}

		if(scan_mode == true)
			break;
		else
			TOUCH_ERR("SCAN fail : 0x%X, retry %d times \n", data, k);
	}

	if(scan_mode == false) {
		TOUCH_ERR("SCAN FAIL :NG :0x%X, retry %d times \n", data, k);
		return TOUCH_FAIL;
	}

	/* Read Raw data */
	addr = 0x01;
	data = 0xAD;
	result = fts_write_reg(client, addr, data);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("write ERR \n");
		return TOUCH_FAIL;
	}
	mdelay(10);

#if 1 // read full data at once
	addr = 0x6A;
	result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
		return TOUCH_FAIL;
	}
	mdelay(10);

	TOUCH_DBG("Raw data read success \n");

#else // read packet_length data
	for (i = 0; (MAX_COL*MAX_ROW*2 - TEST_PACKET_LENGTH*i) > 0; i++)
	{
		addr = 0x1C;
		data = ((TEST_PACKET_LENGTH*i)&0xFF00)>>8;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("write ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(10);

		addr = 0x1D;
		data = (TEST_PACKET_LENGTH*i)&0x00FF;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("write ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(10);

		addr = 0x6A;
		read_len = ((MAX_COL*MAX_ROW*2 - TEST_PACKET_LENGTH*i) >= TEST_PACKET_LENGTH) ? TEST_PACKET_LENGTH : (MAX_COL*MAX_ROW*2 - TEST_PACKET_LENGTH*i);
		result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[TEST_PACKET_LENGTH*i], read_len);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("Read ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(10);

		TOUCH_DBG("%d : RawAddrH=0x%02X  RawAddrL=0x%02X : %d bytes read success... remain:%d \n",
			i, ((TEST_PACKET_LENGTH*i)&0xFF00)>>8, (TEST_PACKET_LENGTH*i)&0x00FF,
			read_len, (MAX_COL*MAX_ROW*2 - TEST_PACKET_LENGTH*i - read_len));
	}
#endif

	/* Combine */
	for (i = 0; i < MAX_ROW; i++) {
		for(j = 0; j < MAX_COL; j++) {
			fts_data[i][j] = (i2c_data[((i * MAX_COL)+j)<<1] << 8) + i2c_data[(((i * MAX_COL)+j)<<1)+1];
			/* for debug
			if (fts_data[i][j] < 0) {
				TOUCH_DBG("data[%d][%d] : %d : 0x%02X(0x%02X<<8) + 0x%02X \n", i, j, fts_data[i][j],
					(i2c_data[((i * MAX_COL)+j)<<1] << 8), i2c_data[((i * MAX_COL)+j)<<1], i2c_data[(((i * MAX_COL)+j)<<1)+1]);
			}
			*/
		}
	}

	is_rawdata_ok_test = true;

	if (is_rawdata_ok_test == false) {
		return TOUCH_FAIL ;
	}
	return TOUCH_SUCCESS;
}

static int FT8606_IB_Test(struct i2c_client *client)
{
	int i, j;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result = 0x00;
	int is_ib_ok_test = false;
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
			fts_data[i][j] = i2c_data[i * MAX_COL + j];
		}
	}

	is_ib_ok_test = true;

	if ((is_ib_ok_test == false)) {
		return TOUCH_FAIL ;
	}
	return TOUCH_SUCCESS;
}

static int FT8606_Noise_Test(struct i2c_client *client)
{

	int i, j;
	u8 addr = 0x00;
	u8 data = 0x00;
	u8 result = 0x00;
	int frame_count = 0;
	int is_noise_ok_test = false;

	TOUCH_LOG("[Test Item]  Noise data Test \n");
	memset( i2c_data,0, sizeof(i2c_data));

		FT8606_Reset(0, 5);
		FT8606_Reset(1, 300);
		mdelay(300);

		result = FT8606_change_mode(client, FTS_FACTORY_MODE);
		if(result != TOUCH_SUCCESS)
			return TOUCH_FAIL;
		
		addr = 0x06;
		data = 0x01;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("fts_write_reg ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(100);

		addr = 0x12;
		data = 0x20;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("fts_write_reg ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(10);

		// to start noise test
		addr = 0x11;
		data = 0x01;
		result = fts_write_reg(client, addr, data);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("fts_write_reg ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(100);
		/*
		// for check value
		for(i=0;i< 100; i++)
		{
			addr = 0x00;
			fts_read_reg(client, addr, &data);
			TOUCH_LOG("data: %x\n",data);
			mdelay(1);
		}
		*/
		//mdelay(100);

		addr = 0x01;
		data = 0xad;
		result = fts_write_reg(client, addr, data); // Raw data type swithing
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("fts_write_reg ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(100);

		for(i=0;i< 100; i++)
		{
			addr = 0x00;
			fts_read_reg(client, addr, &data);
			if ((data & 0x80) == 0x00){
				TOUCH_LOG("(data & 0x80) == 0x00: i = %d, data: %x\n",i,data);
				break;  // noist test end
			}
			mdelay(50);	//mdelay(20);
		}

		if ((data & 0x80) != 0x00){ // NOISE TEST SCAN COMMAND FAIL
			TOUCH_ERR("NOISE TEST SCAN COMMAND FAIL\n");
			return TOUCH_FAIL;
		}

		addr = 0x6a;
		result = FT8707_I2C_Read(client, &addr, 1, &i2c_data[0], MAX_COL*MAX_ROW*2);
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("FT8707_I2C_Read ERR\n");
			return TOUCH_FAIL;
		}
		mdelay(100);

		addr = 0x13;
		result = fts_read_reg(client, addr, &data);  // Get the noise test frame. (FRAME_CNT)
		if(result != TOUCH_SUCCESS) {
			TOUCH_ERR("Read ERR\n");
			return TOUCH_FAIL;
		}
		frame_count = data;

		mdelay(10);
		TOUCH_LOG(" Frame count : %d\n",frame_count);

	for (i = 0; i < MAX_ROW; i++) {
		for (j = 0; j < MAX_COL; j++) {
#if 1		//STDEV
			fts_data[i][j] = (((i2c_data[((i * MAX_COL)+j)<<1]<<8) + i2c_data[(((i * MAX_COL)+j)<<1)+1]))/frame_count;
			fts_data[i][j] = (unsigned long)int_sqrt((unsigned long)fts_data[i][j]);
#else
			// MAX
			fts_data[i][j] = (((i2c_data[((i * MAX_COL)+j)<<1]<<8) + i2c_data[(((i * MAX_COL)+j)<<1)+1]))/frame_count;
			//fts_data[i][j] = (unsigned long)int_sqrt((unsigned long)fts_data[i][j]);;
#endif
		}
	}

	addr = 0x06;
    data = 0x00;
    result = fts_write_reg(client, addr,data);  // To Select data Mode to Raw data
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Read ERR\n");
			return TOUCH_FAIL;
	}

	is_noise_ok_test = true;

	if ((is_noise_ok_test == false)) {
		return TOUCH_FAIL ;
	}

	return TOUCH_SUCCESS;

}

static int FT8606_DoTest(struct i2c_client *client, int type)
{
	int result = 0;

	TOUCH_FUNC();

	/* Enter Factory mdoe */
	result = FT8606_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS)
		return TOUCH_FAIL;

	switch(type){
		case RAW_DATA_SHOW:
			TOUCH_LOG("=== Rawdata image ===\n");
			result = FT8606_Rawdata_Test(client);
			break;
		case IB_SHOW:
			TOUCH_LOG("=== IB image ===\n");
			result = FT8606_IB_Test(client);
			break;
		case NOISE_SHOW:
			TOUCH_LOG("=== Noise image ===\n");
			result = FT8606_Noise_Test(client);
			break;
		case LPWG_RAW_DATA_SHOW:
			TOUCH_LOG("=== LPWG Rawdata image ===\n");
			result = FT8606_Rawdata_Test(client);
			break;
		case LPWG_IB_SHOW:
			TOUCH_LOG("=== LPWG IB image ===\n");
			result = FT8606_IB_Test(client);
			break;
		case LPWG_NOISE_SHOW:
			TOUCH_LOG("=== LPWG Noise image ===\n");
			result = FT8606_Noise_Test(client);
			break;
		default:
			TOUCH_ERR("type select error, type : %d", type);
			break;
	}

	/* block : do not change to work-mode at each test.
	after all test is done, TP Reset in sysfs function .
	FT8606_change_mode(client, FTS_WORK_MODE);
	*/
	FT8606_change_mode(client, FTS_WORK_MODE);

	TOUCH_LOG("%s [DONE]\n", __func__);
	return result;
}

static int FT8606_PrintData(struct i2c_client *client, char *buf, int* result, int type)
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int limit_upper = 0;
	int limit_lower = 0;
	int error_count = 0;
	int eachResult = TOUCH_SUCCESS;
	u8 version[2] = {0, };
	u8 version_buf = 0;
	u8 version_temp[2] = {0,}; /* [0] = 10pos, [1] = 1 pos*/

	//*result = TOUCH_SUCCESS;
	TOUCH_FUNC();

	min_data = fts_data[0][0];
	max_data = fts_data[0][0];


	ret = fts_read_reg(client, FTS_REG_FW_VER, &version_buf);
	version[0] = (version_buf & 0x80) >> 7;
	version[1] = version_buf & 0x7F;

	if( ret == TOUCH_FAIL )
		return TOUCH_FAIL;

	/* Re Parse version for Unified 4 */
	version_temp[0] = (version[1] / 10) << 4;
	version_temp[1] = version[1] % 10;
	version[1] = version_temp[0] | version_temp[1];
	TOUCH_LOG("IC F/W Version = v%X.%02X\n", version[0], version[1]);
	ret += sprintf(buf + ret,"IC F/W Version = v%X.%02X\n", version[0], version[1]);

	switch (type) {
	case RAW_DATA_SHOW:
		TOUCH_LOG("Rawdata Result\n");
		ret += sprintf(buf + ret,"Rawdata Result\n");
		TOUCH_LOG("Raw Data Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "Raw Data Read --------------------------------------- \n");

		limit_upper = RAW_DATA_MAX + RAW_DATA_MARGIN;
		limit_lower = RAW_DATA_MIN - RAW_DATA_MARGIN;
		break;
	case IB_SHOW:
		TOUCH_LOG("IB Result\n");
		ret += sprintf(buf + ret,"IB Result\n");
		TOUCH_LOG("IB data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "IB data Read --------------------------------------- \n");
		limit_upper = IB_MAX;
		limit_lower = IB_MIN;
		break;
	case NOISE_SHOW:
		TOUCH_LOG("Noise Result\n");
		ret += sprintf(buf + ret,"Noise Result\n");
		TOUCH_LOG("Noise data Read -----------------------------------------\n");
		ret += sprintf(buf + ret, "Noise data Read --------------------------------------- \n");
		limit_upper = NOISE_MAX;
		limit_lower = NOISE_MIN;
		break;
    case LPWG_RAW_DATA_SHOW:
		TOUCH_LOG("LPWG Rawdata Result\n");
		ret += sprintf(buf + ret,"LPWG Rawdata Result\n");
		TOUCH_LOG("LPWG Raw Data Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG Raw Data Read --------------------------------------- \n");
		limit_upper = LPWG_RAW_DATA_MAX;
		limit_lower = LPWG_RAW_DATA_MIN;
		break;
    case LPWG_IB_SHOW:
		TOUCH_LOG("LPWG IB Result\n");
		ret += sprintf(buf + ret,"LPWG IB Result\n");
		TOUCH_LOG("LPWG IB Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG IB Read --------------------------------------- \n");
		limit_upper = LPWG_IB_MAX;
		limit_lower = LPWG_IB_MIN;
		break;
	case LPWG_NOISE_SHOW:
		TOUCH_LOG("LPWG Noise Result\n");
		ret += sprintf(buf + ret,"LPWG Noise Result\n");
		TOUCH_LOG("LPWG Noise Read --------------------------------------- \n");
		ret += sprintf(buf + ret, "LPWG Noise Read --------------------------------------- \n");
		limit_upper = LPWG_NOISE_MAX;
		limit_lower = LPWG_NOISE_MIN;
		break;
	default:
		return TOUCH_FAIL;
	}
	/*
	for (j = 0; j < MAX_COL; j++) {
		if(type == RAW_DATA_SHOW || type == LPWG_RAW_DATA_SHOW){
			ret += sprintf(buf + ret, "[%3d]", j+1);
			printk("[%3d]", j+1);
		}
		else{
			ret += sprintf(buf + ret, "[%2d]", j+1);
			printk("[%2d]", j+1);
		}
	}
	*/
	for (i = 0; i < MAX_ROW; i++) {
		ret += sprintf(buf + ret, "[%2d] ", i+1);
		printk("[%2d] ", i+1);
		for (j = 0; j < MAX_COL; j++) {
			if(type == RAW_DATA_SHOW || type == LPWG_RAW_DATA_SHOW){
				if (fts_data[i][j] >= limit_lower && fts_data[i][j] <= limit_upper) {
					ret += sprintf(buf + ret, "%4d ", fts_data[i][j]);
					printk("%4d", fts_data[i][j]);
				} else {
					ret += sprintf(buf + ret,"%4d ", fts_data[i][j]);
					printk("%4d", fts_data[i][j]);
					error_count++;
					eachResult |= TOUCH_FAIL;
				}
			}
			else{
				if (fts_data[i][j] >= limit_lower && fts_data[i][j] <= limit_upper) {
					ret += sprintf(buf + ret, "%3d ", fts_data[i][j]);
					printk("%3d ", fts_data[i][j]);
				} else {
					ret += sprintf(buf + ret,"%3d ", fts_data[i][j]);
					printk("%3d ", fts_data[i][j]);
					error_count++;
					eachResult |= TOUCH_FAIL;
				}
			}
				min_data = (min_data > fts_data[i][j]) ? fts_data[i][j] : min_data;
				max_data = (max_data < fts_data[i][j]) ? fts_data[i][j] : max_data;
		}

		ret += sprintf(buf + ret, "\n");
		printk("\n");
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
	//*result |= eachResult;
	*result = TOUCH_SUCCESS;
	//TODO: Compare to limitation spec & determine result test
	ret += sprintf(buf + ret,"MAX = %d,  MIN = %d  (MAX - MIN = %d), Spec rower : %d, upper : %d\n\n",max_data, min_data, max_data - min_data, limit_lower, limit_upper);

	return ret;
}

ssize_t FT8606_GetTestResult(struct i2c_client *client, char *pBuf, int *result, int type)
{
	int i;
	int ret = 0;
	int retry_count = 0;
	int retry_max = 3;

	TOUCH_FUNC();

	for ( i = 0 ; i < MAX_ROW ; i++) {
		memset(fts_data[i], 0, sizeof(s16) * MAX_COL);
	}

	retry_count = 0;
	while (retry_count++ < retry_max) {
		if (FT8606_DoTest(client, type) == TOUCH_FAIL) {
			TOUCH_ERR("Getting data failed, retry (%d/%d)\n", retry_count, retry_max);
			FT8606_Reset(0,5);
			FT8606_Reset(1,300);
			//mdelay(100);
		} else { break; }

		if (retry_count >= retry_max)
		{
			TOUCH_ERR("%s all retry failed \n", __func__);
			goto ERROR;
		}
	}

	ret += FT8606_PrintData(client, pBuf, result, type);
	if (ret < 0) {
		TOUCH_ERR("fail to print type data\n");
		goto ERROR;
	}

	return ret;

ERROR:
	*result = TOUCH_FAIL;
	return TOUCH_FAIL;
}
