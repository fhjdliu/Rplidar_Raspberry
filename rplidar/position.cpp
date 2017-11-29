#include <position.h>

// static float
float PlainMap_Scale = (float)LidarImageSize / (Rplidar_Max_Range * 2);		// ͼ�� / ����ֱ�� = ������

__positioning::__positioning()
{
	// ��ͼ��ʼ��
	Mat zero(PlainMap_Heigth, PlainMap_Width, CV_8UC3, Scalar(0, 0, 0));		// ͼƬ��ʽ�� BGR
	Map = zero.clone();
	zero.release();

	// �趨��ʼλ��
	//Pos.X = Pos.Y = PlainMap_Size * PlainMap_Scale * 0.25f;
	Pos.X = Pos.Y = 2500.0f;
	Pos.Yaw = 0.0f;

	Pos_Last =  Pos_Last_2 = StartPos = Pos;		// �趨��ʼλ�úͳ�ʼָ��

	Line_Group.clear();
	Line_GLast.clear();

	OB_All.clear();

	rV.clear();


}// __positioning::__positioning()


__positioning::~__positioning()
{


}// __positioning::~__positioning()


int __positioning::update_LineGroup(vector<__obstacle_line> ob)
{
	int i = 0;

	// ������������һ�ε�����
	Line_GLast.clear();
	for (i = 0; i < Line_Group.size(); i++)
	{
		Line_GLast.push_back(Line_Group[i]);
	}

	// �洢��һ������
	Line_Group.clear();
	for (i = 0; i < ob.size(); i++)
	{
		Line_Group.push_back(ob[i]);
	}

	return ob.size();

}// int __positioning::update_LineGroup(vector<__obstacle_line> ob)

int __positioning::calc_Grid_Velocity(float vx, float vy, float freq)
{
	// ����ģ�����Ҫ����ʵ�ֺ���
	if (!freq) return FAILED;

	Freq = freq;

	calc_GridGroup();

	calc_Velocity(Freq);			// �����ٶ�
	V_Complementary_Filter(vx, vy);

	calc_Grid(Freq);				// ��������
	//G_Kalman_Filter();			// ��Ϊ�������ݶ��Ǽ�֡���õ��ģ����Թ�������Ŀ������ô�����

	fuse_Obstacle();				// ����ͼ����

	draw_Map();						// ����������ͼ

	return SUCCESS;


}// int __positioning::calc_Grid_Velocity(float vx, float vy)

int __positioning::calc_GridGroup()
{
	int i, j;
	int pt_num;
	float avg_x, avg_y;

	float theta, rho, x, y;

	// ���ȱ�����һ�ε���������
	rGrid_Last.clear();
	for (i = 0; i < rGrid.size(); i++)
	{
		rGrid_Last.push_back(rGrid[i]);
	}

	// Ȼ��ʼ������һ�ε�����
	rGrid.clear();
	avg_x = avg_y = 0.0f;
	for (i = 0; i < Line_Group.size(); i++)
	{
		pt_num = 0;
		for (j = 0; j < ANGLE_ALL; j++)
		{
			if (Line_Group[i].Position[j] != 0.0f)
			{
				theta = j * PI / 180;
				rho = Line_Group[i].Position[j];

				x = (int)(rho  * sin(theta) * PlainMap_Scale);
				y = (int)(-rho * cos(theta) * PlainMap_Scale);


				avg_x += x;
				avg_y += y;
				pt_num++;

			}
		}
		avg_x /= pt_num;
		avg_y /= pt_num;

		__pos temp;
		temp.X = avg_x;
		temp.Y = avg_y;
		temp.Yaw = atan2(x, y) * 180.0f / PI;

		rGrid.push_back(temp);
	}

	return SUCCESS;

}// int __positioning::calc_GridGroup()

int __positioning::calc_Grid(float freq)
{
	// ��������
	Pos.X += V.X * freq;
	Pos.Y += V.Y * freq;

	return SUCCESS;

}// int __positioning::calc_Grid(float freq)

int __positioning::fuse_Obstacle()
{
	// �ϰ����ں�
	// ����ƥ���ϰ���������ϰ�����бȶԣ��������������ӽ����ݿ�
	int i, j, k;
	bool is_matched[GROUP_MAX] = { false };

	int d_pt_eff_threshold = ANGLE_ALL / 20;		// ��Ч��������ֵ
	float d_theta_threshold = 0.05f;				// �ǶȲ�ֵ��ֵ
	float d_rho_threshold = 15.0f;				// �����ֵ��ֵ

	__obstacle_map      temp;
	//__obstacle_line     line_t;
	//__obstacle_circle circle_t;

	// ���������ϰ���
	temp.is_Line = true;
	for (i = 0; i < Line_Group.size(); i++)
	{
		for (j = 0; j < OB_All.size(); j++)
		{
			// ���ƥ��
			int pt_eff, pt_eff_last;
			int d_pt_eff;		// ��Ч������
			float d_theta;		// �ǶȲ�ֵ
			float d_rho;		// �����ֵ

								// ������Ч�����
			pt_eff = 0;
			for (k = 0; k < ANGLE_ALL; k++)
				if (Line_Group[i].Position[k] != 0.0f)
					pt_eff++;

			pt_eff_last = 0;
			for (k = 0; k < ANGLE_ALL; k++)
				if (OB_All[j].l.Position[k] != 0.0f)
					pt_eff_last++;

			d_pt_eff = abs(pt_eff - pt_eff_last);
			d_theta = fabs(Line_Group[i].Theta - OB_All[j].l.Theta);
			d_rho = fabs(Line_Group[i].Rho - OB_All[j].l.Rho);

			// ��ֵС����ֵ����Ϊ��Ч
			if (d_pt_eff <= d_pt_eff_threshold &&
				d_theta <= d_theta_threshold  &&
				d_rho <= d_rho_threshold)
			{
				// �����Ϊ��ͬһ����ʼ����

				is_matched[i] = true;
				break;
			}

		}

		// ���û��ƥ��������Ϊ����ϰ���������һ���·��ֵ�
		if (is_matched[i] == false)
		{
			temp.l = Line_Group[i];
			temp.Center = Pos;

			OB_All.push_back(temp);
		}
	}

	return SUCCESS;
}// int __positioning::fuse_Obstacle()

int __positioning::calc_Velocity(float freq)
{
	// ͨ��ƥ���ϰ���������
	// ֱ�Ӷ����������ж�,����ֵ�������Ŀ��ֱ�ߵ�theta��rho

	// ��ֵ��
	int d_pt_eff_threshold = ANGLE_ALL / 20;		// ��Ч��������ֵ
	float d_theta_threshold = 0.05f;				// �ǶȲ�ֵ��ֵ
	float d_rho_threshold   = 15.0f;				// �����ֵ��ֵ

	int i, j, k;
	bool is_matched[GROUP_MAX] = { false };


	//if (Line_Group.size() != rGrid.size() ||
	//	Line_GLast.size() != rGrid_Last.size())
	//	return FAILED;

	rV.clear();
	for (i = 0; i < Line_Group.size(); i++)
	{
		for (j = 0; j < Line_GLast.size(); j++)		// Line_Group�Ĵ�С������Ĵ�С��һһ��Ӧ��
		{
			if (is_matched[j] == false)
			{
				// ���������ƥ��
				// ��Ϊ���ٲ����������£�ÿ���ؾ�ǰ����ת��ĽǶȶ������޵ģ���������������ǽ���ƥ��
				int pt_eff, pt_eff_last;
				int d_pt_eff;		// ��Ч������
				float d_theta;		// �ǶȲ�ֵ
				float d_rho;		// �����ֵ

				// ������Ч�����
				pt_eff = 0;
				for (k = 0; k < ANGLE_ALL; k++)
				{
					if (Line_Group[i].Position[k] != 0.0f)
						pt_eff++;
				}

				pt_eff_last = 0;
				for (k = 0; k < ANGLE_ALL; k++)
				{
					if (Line_GLast[j].Position[k] != 0.0f)
						pt_eff_last++;
				}

				d_pt_eff = abs(pt_eff - pt_eff_last);
				d_theta  = fabs(Line_Group[i].Theta - Line_GLast[j].Theta);
				d_rho    = fabs(Line_Group[i].Rho   - Line_GLast[j].Rho);


				// ��ֵС����ֵ����Ϊ��Ч
				if (d_pt_eff <= d_pt_eff_threshold &&
					d_theta  <= d_theta_threshold  &&
					d_rho    <= d_rho_threshold)
				{
					// �����Ϊ��ͬһ����ʼ����

					__v temp;

					temp.X = rGrid[i].X - rGrid_Last[j].X;
					temp.Y = rGrid[i].Y - rGrid_Last[j].Y;
					temp.X *= freq;
					temp.Y *= freq;

					temp.Val = sqrt(temp.X * temp.X + temp.Y * temp.Y);	// ����ģֵ

					temp.Yaw = rGrid[i].Yaw - rGrid_Last[j].Yaw;		// ���Լ�����ٶ�
					temp.Yaw *= freq;

					rV.push_back(temp);

					is_matched[j] = true;
					break;
				}
			}
		}

	}

	// ��ƽ��
	if (rV.size() == 0)
	{
		V.X = V.Y = 0.0f;
		return FAILED;
	}

	for (i = 0; i < rV.size(); i++)
	{
		V.X += rV[i].X;
		V.Y += rV[i].Y;
		V.Val += rV[i].Val;

		V.Yaw += rV[i].Yaw;
	}
	V.X /= rV.size();
	V.Y /= rV.size();
	V.Val /= rV.size();
	V.Yaw /= rV.size();

	// �ٶ�������������ؾߵģ�����Ҫȡ��ֵ��ģֵû�и��ģ�ƫ��������ȡ��ֵ������Ļ���һ��
	V.X = -V.X;
	V.Y = -V.Y;


	return SUCCESS;

}// int __positioning::calc_Velocity()

int __positioning::V_Complementary_Filter(float vx, float vy)
{
	// �����˲���
	// �����ֵҪ����
	int i;
	float threshold_x = 0.667f;
	float threshold_y = 0.333f;

	V.X = threshold_x * V.X + (1 - threshold_x) * vx;
	V.Y = threshold_y * V.Y + (1 - threshold_y) * vy;
	V.Val = sqrt(V.X * V.X + V.Y * V.Y);

	V.Yaw = atan2(V.X, V.Y) * 180.0f / PI;

	return SUCCESS;

}// int V_Complementary_Filter(float vx, float vy)

int __positioning::G_Kalman_Filter()
{
	// λ�ÿ������˲���
	// QPos_Dst��RPos_Dst��������������ֵҲ��Ҫ����

	// x����
	gk_x.X_last = gk_x.X;
	gk_x.P_last = gk_x.P;

	gk_x.X_mid = gk_x.X_last + V.X + QPos_Dst;

	gk_x.K = gk_x.P_mid / (gk_x.P_mid + RPos_Dst);
	gk_x.X = gk_x.X_mid + gk_x.K * (Pos.X - gk_x.X_mid);
	gk_x.P = (1 - gk_x.K) * gk_x.P_mid;

	// y����
	gk_y.X_last = gk_y.X;
	gk_y.P_last = gk_y.P;

	gk_y.X_mid = gk_y.X_last + V.Y + QPos_Dst;

	gk_y.K = gk_y.P_mid / (gk_y.P_mid + RPos_Dst);
	gk_y.X = gk_y.X_mid + gk_y.K * (Pos.Y - gk_y.X_mid);
	gk_y.P = (1 - gk_y.K) * gk_y.P_mid;

	return SUCCESS;

}// int __positioning::G_Kalman_Filter()

int __positioning::draw_Map()
{

	int i, j;
	float theta;
	float rho;
	int x, y;

	// ����
	for (i = 0; i < OB_All.size(); i++)
	{
		if (OB_All[i].is_Line == true)
		for (j = 0; j < ANGLE_ALL; j++)	// scan_data.size()
		{
			theta = j * PI / 180;
			rho = OB_All[i].l.Position[j];

			if (rho <= Rplidar_Max_Range / 200.0f)
				continue;

			x = (int)(rho  * sin(theta) / 20) + OB_All[i].Center.X * PlainMap_Scale;
			y = (int)(-rho * cos(theta) / 20) + OB_All[i].Center.Y * PlainMap_Scale;

			if ((x >= 0 && x < LidarImageWidth) &&
				(y >= 0 && y < LidarImageHeight))		// �����Щ���ڲ���ͼ��
			{
				circle(Map, Point(x, y), 1, Scalar(255, 255, 255), -1, 8, 0);
			}
		}
	}

	imshow("SLAM_Result", Map);

	return SUCCESS;

}// int __positioning::draw_Map()
