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
#define RPos_Dst	0.25f		// ����Ĳ������������������;��뱾���ľ����й�ϵ�����Ǵ���0.5mm������ֱ��ƽ����0.25

extern float PlainMap_Scale;		// 600 / 6000 / 2

class __z_lidar
{
public:

	__z_lidar();											// ����
	~__z_lidar();											// ɾ��

	Mat						Map;								// ��ͼͼƬ
	__pos Pos, Pos_Last, Pos_Last_2, StartPos;						// ����
	vector<__obstacle_line> Line_Group, Line_GLast;				// ���������ߵľ���
	vector<__obstacle_map>  OB_All;								// �����ϰ���ļ���
	float					 Freq;									// Ƶ��

	vector<__point2p>			rGrid, rGrid_Last;					// ÿ���ϰ����ƶ���������꣬ע����ߵ�Yaw����
	vector<__point2p>			Landmark_Measured;

	int obstacle_num;												// ��Ч���ϰ�������


	int update_LineGroup(vector<__obstacle_line> ob);

	int calc_Lidar( float freq);		// ������ٶȼ���

	int draw_Map(bool is_show);									// ��ͼ
	int update_Position_on_Map(__point2f input, float yaw, bool is_show);

private:

	int calc_OB_Centres_All();									// �������꣬�����ϰ��ﶼ��һ��landmark��ʾ��������ƽ������
	int calc_OB_Centres_Measured();
	int fuse_Obstacle();										// �ϰ�������


};



