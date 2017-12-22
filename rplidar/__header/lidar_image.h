#pragma once


#include "config.h"
#include <cmath>

#define Q_Spd	9000.0f		// �ٶȵĹ����������ٶȵĹ��ƺͼ��ٶ��й�ϵ�����������ü��ٶȵ�ֵ�Ĳ�����ƽ�������㣬���ٶȵĲ��������0.18f~-0.12f��������λӦ����m/s^2
#define R_Spd	22500.0f	// �ٶȵĲ���������������������ͨ���۲����ֵ�ã���������ﵽ150mm����

#define Q_Dst	R_Spd		// ����ĵĹ��������������������ٶȵĲ����������룬��Ϊ����ĵĹ��ƺ��ٶ��й�
#define R_Dst	0.25f		// ����Ĳ������������������;��뱾��ľ����й�ϵ�����Ǵ���0.5mm������ֱ��ƽ����0.25

class __lidar_img
{
public:
	__lidar_img();		// ����
	~__lidar_img();		// ����

	vector<__scandot> Data;
	vector<__scandot> Data_Last;
	vector<__scandot> Vlct;

	float Vx;							// x����: ƽ���ڳ���ǰ������ķ���
	float Vy;							// y����: ��ֱ�ڳ���ǰ������ķ���

	float Scan_Speed;

	Mat Img_Dst_Raw;					// ����ͼ��ԭʼ����

	// ��һ������
	float Data_NArray[ANGLE_ALL];		// ��һ���������
	float Data_NLast[ANGLE_ALL];		// ��һ�������ݱ���

	float Vlct_NArray[ANGLE_ALL];		// �ٶ�

	// �������˲���
	__kalman KF_Speed[ANGLE_ALL];		// �ٶȿ������˲���
	__kalman KF_Dst[ANGLE_ALL];			// ���뿨�����˲���

	float Data_KArray[ANGLE_ALL];		// �������˲������
	float Vlct_KArray[ANGLE_ALL];


	int scanData(rplidar_response_measurement_node_t *buffer, size_t count, float frequency);
	int Draw(Mat &dst, vector<__scandot> data, char window_name[]);
	int Draw(Mat &dst, float data[], char window_name[]);

	int Normalize_Data(vector<__scandot> data);

	int calc_Velocity(void);

	int Kalman_Filter(float acc_x, float acc_y);

	int Vlct_Orthogonal_Decomposition(float vlct[]);

	int normalize_Orentation(float data[], int yaw);

private:

};
