#ifndef __TS_LIDAR_H

#define __TS_LIDAR_H

#include "config.h"
#ifdef WIN32
	#include <time.h>
#else
	#include <sys/time.h>
#endif

#define TS_OBSTACLE 0
#define TS_NO_OBSTACLE 255

#define TS_DISTANCE_NO_DETECTION 0		// ע�⣬�����tinyslam���Ҳ���


#define TEST_OFFSET_LASER 145
#define TEST_HOLE_WIDTH 600

typedef struct {
	double r;	    // length wheels' radius
	double R;	    // half the wheels' axis length
	int inc;	    // wheels' counters increments per turn
	double ratio;   // ratio between left and right wheel
} cart_parameters_t;


class __tinyslam
{
public:
	__tinyslam();
	~__tinyslam();


	Mat Map, Trajectory;		// �ֱ��ʾ��ͼ���켣
	__ts_pos_t Pos, startPos, Pos_2;

	vector<__scandot> Data;		// �����״ﴫ������ԭʼ����
	__ts_scan_t Scan;			// TinySlam������ʾ���ݵĽṹ��


	// �������������д���
	int Run(float data[], vector<__scandot> raw, float vx, float vy);

	int init_Map();
	int update_Map(__ts_scan_t &scan, Mat &map, __ts_pos_t &pos, int quality, int hole_width);

	int get_ScanData(vector<__scandot> raw);
	int get_ScanData(float data[], vector<__scandot> raw);		// �ڶ���rawֻ�Ǵ�����Ϊ�˼���
	int Draw();
	int draw_Trajectory();

private:

	// ʱ��
#ifdef WIN32
	clock_t t_now, t_last, t_start;
#else

#endif

	cart_parameters_t params;


	__ts_pos_t monte_carlo_move(__ts_scan_t &scan, Mat &map, __ts_pos_t &start_pos, int debug);
	int ts_distance_scan_to_map(__ts_scan_t &scan, Mat &map, __ts_pos_t &start_pos);
	void ts_map_laser_ray(Mat &map, int x1, int y1, int x2, int y2, int xp, int yp, int value, int alpha);
};



#endif	/* __TS_LIDAR_H */

