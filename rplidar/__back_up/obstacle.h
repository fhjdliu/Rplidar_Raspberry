#ifndef __Obstacle_Group_H

#define __Obstacle_Group_H

#include "config.h"


#define Line_Threshold 5.0f

///
/// ���ģ����Ҫ����֡��ͼ�����Լ��ϰ���ķ���
/// ��Ҫͨ������任�ͷ������ﵽĿ��
class __obstacle_group
{
public:
	__obstacle_group();			// ���췽��
	~__obstacle_group();		// KO

	float Data[ANGLE_ALL];
	float Data_Last[ANGLE_ALL];

	vector<__obstacle_line> OLines;

	Mat Img_Initial;			// ��ʼ��ͼ
	Mat Img_Lines, Img_Lines_Last;

	int get_Array(float data[]);
	int draw();
	int draw(Mat &dst, vector<__obstacle_line> ob);
	int draw_lines(Mat &dst, vector<__obstacle_line> ob);
	int calc_Lines();

	int surf();

private:


	int merge_LineGroup();
	int remove_Standlone_Pts();


};


#endif	/* __Obstacle_Group_H */
