#pragma once

#include "config.h"

/*

ģ�����ϣ�
	�����ȡ�������ϰ�������ݣ�Ȼ�����



*/

#define PlainMap_Size   1200
#define PlainMap_Width  PlainMap_Size
#define PlainMap_Heigth PlainMap_Size

// �������˲�������
#define QPos_Dst	22500.0f	// ����ĵĹ��������������������ٶȵĲ����������룬��Ϊ����ĵĹ��ƺ��ٶ��й�
#define RPos_Dst	0.25f		// ����Ĳ������������������;��뱾��ľ����й�ϵ�����Ǵ���0.5mm������ֱ��ƽ����0.25

extern float PlainMap_Scale;		// 600 / 6000 / 2

class __positioning
{
public:

	__positioning();											// ����
	~__positioning();											// ɾ��

	Mat						Map;								// ��ͼͼƬ
	__pos Pos, Pos_Last, Pos_Last_2, StartPos;					// ����
	vector<__obstacle_line> Line_Group, Line_GLast;				// ���������ߵľ���
	vector<__obstacle_map>  OB_All;								// �����ϰ���ļ���
	__v						V;									// �ٶ�
	float					Freq;								// Ƶ��


	int num;													// ��Ч���ϰ�������
	

	int update_LineGroup(vector<__obstacle_line> ob);

	int calc_Grid_Velocity(float vx, float vy, float freq);		// ������ٶȼ���

	int draw_Map();												// ��ͼ

private:

	vector<__v>				rV;									// �����ϰ�����ٶ�
	vector<__pos>			rGrid, rGrid_Last;					// ÿ���ϰ����ƶ���������꣬ע����ߵ�Yaw����

	__kalman				gk_x, gk_y;							// λ�ÿ������˲�������

	int calc_GridGroup();										// ����������
	int calc_Grid(float freq);									// ��������
	int fuse_Obstacle();										// �ϰ�������

	int calc_Velocity(float freq);								// �����ٶ���

	int V_Complementary_Filter(float vx, float vy);				// �ٶȻ����˲������ͱ�Ĳ��ֵ��ٶ������������˲�

	int G_Kalman_Filter();										// λ�ÿ������˲���


};




