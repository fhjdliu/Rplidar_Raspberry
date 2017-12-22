#include "main.h"
#include "datalink.h"




__pix_link::__pix_link()
{
	fd = 0;
	baudrate = 9600;

	memset(cache, 0, sizeof(char) * PIX_RX_MAX);
	cache_ptr = 0;

}// __pix_link::__pix_link()


int __pix_link::init()
{
	baudrate = 9600;
	fd = serialOpen(PIX_DATALINK_DEVICE, baudrate);

	//cout << fd << endl;

	return fd;

}// int __pix_link::init()

int __pix_link::get_Data()
{
	receive();
	refine(cache);

/*
	cout << "Pitch: " << Eular_Angle.Pitch << endl;
	cout << "Roll:  " << Eular_Angle.Roll  << endl;
	cout << "Yaw:   " << Eular_Angle.Yaw   << endl;
	cout << "Ax:    " << Acc.X << endl;
	cout << "Ay:    " << Acc.Y << endl;
	cout << "Az:    " << Acc.Z << endl;

	cout << "Wx:    " << W.X << endl;
	cout << "Wy:    " << W.Y << endl;
	cout << "Wz:    " << W.Z << endl;

	cout << "Vx:    " << V.X << endl;
	cout << "Vy:    " << V.Y << endl;
	cout << "Vz:    " << V.Z << endl;
*/

	return SUCCESS;

}// int __pix_link::get_Data()

int __pix_link::receive()
{
	int i;
	bool is_get_start = false;
	char data_last = 0;

	memset(cache, 0, sizeof(char) * PIX_RX_MAX);

	serialFlush(fd);				// 清空缓存重新收
    for (i = 0; i < PIX_RX_MAX; i++)
    {
		// 开始，这边故意卡一下
		if (cache[0] != 'P' && is_get_start == false)
		{
			cache[0] = serialGetchar(fd);
			i = 0;
			continue;
		}
		else if (cache[0] == 'P' && is_get_start == false)
			is_get_start = true;

		if (i > 0)
			data_last = cache[i - 1];

		cache[i] = serialGetchar(fd);
		cache_ptr++;

		// 结尾是俩空格
		if (data_last == cache[i] == ' ')
			break;

    }

	//cout << cache << endl;

}// int __pix_link::receive()

int __pix_link::refine(char str[])
{
	char *offset;					// 每段开始的指针位置
	int  end;						// 每段空格的位置
	char num_temp[15] = { 0 };		// 用来存放数字的地方

	int  i;							// 计数变量
	int  id;						// 本帧的ID缓存位置
	const int max_num_len = 15;		// 最大的数字字符串长度，这个是个人工估算的参考值，用在找空格的时候


	Eular_Angle_Last = Eular_Angle;

Read_Pitch:
	// Pitch
	if (strstr(str, "Pitch:") == NULL)
		goto Read_Roll;
	offset = strstr(str, "Pitch:") + strlen("Pitch:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	Eular_Angle.Pitch = atoi(num_temp);
	Eular_Angle.Pitch /= 1000.0f;

Read_Roll:
	// Roll
	if (strstr(str, "Roll:") == NULL)
		goto Read_Yaw;
	offset = strstr(str, "Roll:") + strlen("Roll:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	Eular_Angle.Roll = atoi(num_temp);
	Eular_Angle.Roll /= 1000.0f;

Read_Yaw:
	// Yaw
	if (strstr(str, "Yaw:") == NULL)
		goto Read_Wx;
	offset = strstr(str, "Yaw:") + strlen("Yaw:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	Eular_Angle.Yaw = atoi(num_temp);
	Eular_Angle.Yaw /= 1000.0f;

Read_Wx:
	// Wx
	if (strstr(str, "Wx:") == NULL)
		goto Read_Wy;
	offset = strstr(str, "Wx:") + strlen("Wx:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	W.X = atoi(num_temp);
	W.X /= 1000.0f;

Read_Wy:
	// Wy
	if (strstr(str, "Wy:") == NULL)
		goto Read_Wz;
	offset = strstr(str, "Wy:") + strlen("Wy:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	W.Y = atoi(num_temp);
	W.Y /= 1000.0f;

Read_Wz:
	// Wz
	if (strstr(str, "Wz:") == NULL)
		goto Read_Ax;
	offset = strstr(str, "Wz:") + strlen("Wz:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	W.Z = atoi(num_temp);
	W.Z /= 1000.0f;

Read_Ax:
	// Ax
	if (strstr(str, "Ax:") == NULL)
		goto Read_Ay;
	offset = strstr(str, "Ax:") + strlen("Ax:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	Acc.X = atoi(num_temp);
	Acc.X /= 1000.0f;

Read_Ay:
	// Ay
	if (strstr(str, "Ay:") == NULL)
		goto Read_Az;
	offset = strstr(str, "Ay:") + strlen("Ay:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	Acc.Y = atoi(num_temp);
	Acc.Y /= 1000.0f;

Read_Az:
	// Az
	if (strstr(str, "Az:") == NULL)
		goto Read_Vx;
	offset = strstr(str, "Az:") + strlen("Az:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	Acc.Z = atoi(num_temp);
	Acc.Z /= 1000.0f;

Read_Vx:
	// Vx
	if (strstr(str, "Vx:") == NULL)
		goto Read_Vy;
	offset = strstr(str, "Vx:") + strlen("Vx:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	V.X = atoi(num_temp);
	V.X /= 1000.0f;

Read_Vy:
	// Vy
	if (strstr(str, "Vy:") == NULL)
		goto Read_Vz;
	offset = strstr(str, "Vy:") + strlen("Vy:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	V.Y = atoi(num_temp);
	V.Y /= 1000.0f;

Read_Vz:
	// Vz
	if (strstr(str, "Vz:") == NULL)
		return -1;
	offset = strstr(str, "Vz:") + strlen("Vz:");
	end = find_Break(offset, max_num_len);

	memset(num_temp, 0, sizeof(num_temp));
	for (i = 0; i < end; i++)
	{
		num_temp[i] = offset[i];
	}
	V.Z = atoi(num_temp);
	V.Z /= 1000.0f;

	return 0;

}// int __pix_link::refine()

int find_Break(char str[], int len)
{

	// 专门用来找空格的函数，如果找不到则返回0，返回距离开始处最近的空格
	// 请确保第一位不是空格，不然这个函数不起作用
	int i = 0;

	for (i = 0; i < len; i++)
	{
		if (str[i] == 32) return i;
	}

	return 0;

}// find_Break
