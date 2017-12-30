#include "__header/tinyslam.h"

// ��TinySlam��ֲ��OpenCV��
/*

	update_Trajectory�Ǳ��������ֵ̫����

	�����vx��vy�ü�֡�����ݴ��棬ע��x��y������û

*/


static float ts_map_scale = (float)LidarImageSize / (Rplidar_Max_Range * 2);		// ͼ�� / ����ֱ�� = ������

int __tinyslam::Run(float data[], vector<__scandot> raw, float vx, float vy)
{

	static int timestamp, t_old;		// ��ʱ����
#ifndef WIN32
    static timeval t_now, t_last;
#endif
	static int nq1, nq2, q1, q2;		// ·�̱���

	static double psi_dot, psi_dotodo = 0, psi_dotodo_old;
	static double theta_rad, thetarad_dodo;

	static double m;

	static double v, vodo = 0, vodo_old;

	static int x, y;

	static int cnt_scans = 0;			// �����ܵ��������
	int stat = FAILED;

	// ���ɨ������
	stat = get_ScanData(data, raw);

	// ����ʱ��
#ifdef WIN32
	t_last = t_now;						// ms
	t_now = clock();
	timestamp = t_now;					// �õ�ʱ��
#else
	t_last = t_now;
	gettimeofday(&t_now, NULL);
	timestamp = t_now.tv_usec / 1000;	// ms
#endif


	// �õ�·��
	/*

		·�̴����±�״̬�������ȷ���

	*/
	// ���·���ü�֡����ֵ����������ܴ���һ�����
	double dst_x, dst_y, dst;
	dst_x = vx * (timestamp - t_old);
	dst_y = vy * (timestamp - t_old);
	dst = sqrt(dst_x * dst_x + dst_y * dst_y);


//	nq1 = sensor_data[cnt_scans].q1;
//	nq2 = sensor_data[cnt_scans].q2;
	nq1 = dst;
	nq2 = 0;

	// Manage robot position
	if (cnt_scans != 0) {

		// ����
		psi_dotodo_old = psi_dotodo;
		vodo_old = vodo;

		m = params.r * PI / params.inc;
		vodo = m * (nq1 - q1 + (nq2 - q2) * params.ratio);		// �������ٶ�
		theta_rad = Pos.Yaw * PI / 180;

		Pos_2 = Pos;											// ����
		Pos_2.X += vodo * 1000 * cos(theta_rad);				// ������λ��
		Pos_2.Y += vodo * 1000 * sin(theta_rad);

		psi_dotodo = (m * ((nq2 - q2) * params.ratio - nq1 + q1) / params.R) * 180 / PI;
		Pos_2.Yaw += psi_dotodo;
		vodo *= 1000.0 / (timestamp - t_old);					// 1000000.0
		psi_dotodo *= 1000.0 / (timestamp - t_old);				// 1000000.0
	}
	else {
		psi_dotodo_old = psi_dotodo = 0;
		vodo_old = vodo = 0;
	}

	Pos_2 = monte_carlo_move(Scan, Map, Pos, 0);
	v = sqrt((Pos_2.X - Pos.X) * (Pos_2.X - Pos.X) +
			 (Pos_2.Y - Pos.Y) * (Pos_2.Y - Pos.Y));
	psi_dot = Pos_2.Yaw - Pos.Yaw;

	if (cnt_scans != 0) {
		v *= 1000.0 / (timestamp - t_old);
		psi_dot *= 1000.0 / (timestamp - t_old);				// 1000000.0
	}
	else {
		v = 0;
		psi_dot = 0;
	}


	Pos = Pos_2;
	//printf("#%d : %lg %lg %lg\n", cnt_scans, Pos.X, Pos.Y, Pos.Yaw);
	update_Map(Scan, Map, Pos, 50, TEST_HOLE_WIDTH);


	// ������ǰ�켣
	x = (int)floor(Pos.X * ts_map_scale + 0.5);
	y = (int)floor(Pos.Y * ts_map_scale + 0.5);
	if (x >= 0 && y >= 0 && x < LidarImageWidth && y < LidarImageHeight)
		Trajectory.at<uchar>(y, x) = 0;							//trajectory.map[y * LidarImageSize + x] = 0;

	imshow("TS_Map", Map);
	stat = draw_Trajectory();


	//stat = Draw();
	//if (nbscans > 310 && nbscans < 350)
	//{
		//ts_map_init(&trajectory);
		//draw_scan(&sensor_data[cnt_scans].scan, &trajectory, &position);
		//sprintf(filename, "test_lab%04d.pgm", cnt_scans);
		//record_map(state.map, &trajectory, filename, 1600, 1200);
	//}

	q1 = nq1;
	q2 = nq2;
	t_old = timestamp;




	cnt_scans++;
	return stat;

}// int __tinyslam::Run(float data[], vector<__scandot> raw)

__tinyslam::__tinyslam()
{
	init_Map();

	Pos.X = 0.3 * LidarImageSize / ts_map_scale;
	Pos.Y = 0.3 * LidarImageSize / ts_map_scale;
	Pos.Yaw = 0;
	startPos = Pos_2 = Pos;

	// ʱ���ʼ��
#ifdef WIN32
	t_now = t_start = t_last = clock();
#else

#endif

	// ��������������øĵ�
	params.r = 0.077;
	params.R = 0.165;
	params.inc = 2000;
	params.ratio = 1.0;

}// __tinyslam::__tinyslam()



__tinyslam::~__tinyslam()
{
	Map.release();
	Trajectory.release();

}// __tinyslam::~__tinyslam()


int __tinyslam::init_Map()
{
	int val = (TS_OBSTACLE + TS_NO_OBSTACLE) / 2;
	Mat zero(LidarImageHeight, LidarImageWidth, CV_8UC3, Scalar(val, val, val));		// ͼƬ��ʽ�� BGR
	Map = zero.clone();
	zero.release();

	cvtColor(Map, Map, CV_BGR2GRAY);

	// ���ӹ켣
	Trajectory = Map.clone();

	return SUCCESS;

}// int __tinyslam::init_Map()



int __tinyslam::update_Map(__ts_scan_t &scan, Mat &map, __ts_pos_t &pos, int quality, int hole_width)
{
	double c, s;		// cos, sin
	int x1, y1;			// ��ǰλ��
	int xp, yp;
	int x2, y2;
	int x2p, y2p;

	int q, value;

	double dist;
	double add;

	int i;

	c = cos(Pos.Yaw * PI / 180);
	s = sin(Pos.Yaw * PI / 180);

	x1 = (int)floor(Pos.X * ts_map_scale + 0.5);
	y1 = (int)floor(Pos.Y * ts_map_scale + 0.5);

	for (i = 0; i != scan.nb_points; i++) {
		x2p = c * scan.X[i] - s * scan.Y[i];
		y2p = s * scan.X[i] + c * scan.Y[i];
		xp = (int)floor((pos.X + x2p) * ts_map_scale + 0.5);
		yp = (int)floor((pos.Y + y2p) * ts_map_scale + 0.5);
		dist = sqrt(x2p * x2p + y2p * y2p);
		add = hole_width / 2 / dist;
		x2p *= ts_map_scale * (1 + add);
		y2p *= ts_map_scale * (1 + add);
		x2 = (int)floor(pos.X * ts_map_scale + x2p + 0.5);
		y2 = (int)floor(pos.Y * ts_map_scale + y2p + 0.5);
		if (scan.val[i] == TS_NO_OBSTACLE) {
			q = quality / 4;
			value = TS_NO_OBSTACLE;
		}
		else {
			q = quality;
			value = TS_OBSTACLE;
		}
		//printf("%d %d %d %d %d %d %d\n", i, x1, y1, x2, y2, xp, yp);
		ts_map_laser_ray(map, x1, y1, x2, y2, xp, yp, value, q);
	}


	return SUCCESS;
}// int __tinyslam::init_Map()


int __tinyslam::get_ScanData(vector<__scandot> raw)
{
	// ��ӦTinySLAM�У�int read_sensor_data(ts_sensor_data_t *data)
	// ����Ĵ������£������ǵĴ�����ʵ���޶��£����ǽ����������ݱ��x��y�ĵ�
	/*
		angle_deg = TEST_ANGLE_MIN + ((double)(i * SPAN + j)) * (TEST_ANGLE_MAX - TEST_ANGLE_MIN) / (TEST_SCAN_SIZE * SPAN - 1);

		// Correction of angle according to odometry
		//angle_deg += psidotodo_old / 3600.0 * ((double)(i * SPAN + j)) * (TEST_ANGLE_MAX - TEST_ANGLE_MIN) / (TEST_SCAN_SIZE * SPAN - 1);

		angle_rad = angle_deg * M_PI / 180;
		if (i > 45 && i < TEST_SCAN_SIZE - 45) {
		if (d[i] == 0) {
		scan->x[scan->nb_points] = TS_DISTANCE_NO_DETECTION * cos(angle_rad);
		scan->y[scan->nb_points] = TS_DISTANCE_NO_DETECTION * sin(angle_rad);
		scan->value[scan->nb_points] = TS_NO_OBSTACLE;
		scan->x[scan->nb_points] += TEST_OFFSET_LASER;
		scan->nb_points++;
		}
		if (d[i] > TEST_HOLE_WIDTH / 2) {
		scan->x[scan->nb_points] = d[i] * cos(angle_rad);
		scan->y[scan->nb_points] = d[i] * sin(angle_rad);
		scan->value[scan->nb_points] = TS_OBSTACLE;
		scan->x[scan->nb_points] += TEST_OFFSET_LASER;
		scan->nb_points++;

	*/

	int i;
	double theta, rho;
	int halfWidth  = LidarImageWidth / 2;
	int halfHeight = LidarImageHeight / 2;

	// �����ݿ�������
	/*
	// ���������ڴ棬������
	memset(Scan.X, 0, sizeof(double) * ANGLE_ALL);
	memset(Scan.Y, 0, sizeof(double) * ANGLE_ALL);
	memset(Scan.val, TS_NO_OBSTACLE, sizeof(double) * ANGLE_ALL);
	Scan.nb_points = 0;
	*/
	for (i = 0; i < raw.size(); i++)
	{
		// raw -> Data
		Data.push_back(raw[i]);

		__scandot dot;
		dot = raw[i];		// δ�˲�:Data[i] �˲���:data_dst[i]

		theta = dot.Angle * PI / 180;
		rho = dot.Dst;

		// �����tinyslam��˼·�����������ݱ��ͼ���X��Y����
		//Scan.X[i] = x;
		//Scan.Y[i] = y;
		if (dot.Dst == 0) {
			Scan.X[i] = TS_DISTANCE_NO_DETECTION * cos(theta);
			Scan.Y[i] = TS_DISTANCE_NO_DETECTION * sin(theta);
			Scan.val[i] = TS_NO_OBSTACLE;
			//Scan.X[i] += TEST_OFFSET_LASER;
			Scan.nb_points++;
		}
		if (dot.Dst > TEST_HOLE_WIDTH / 2) {
			Scan.X[i] = rho * cos(theta);
			Scan.Y[i] = rho * sin(theta);
			Scan.val[i] = TS_OBSTACLE;
			//Scan.X[i] += TEST_OFFSET_LASER;		// ����x������˸�offset�������Ǵ�������������⣬�����Ȳ�����
			Scan.nb_points++;
		}
	}

	Scan.nb_points = raw.size();
	return SUCCESS;

}// int __tinyslam::get_ScanData(vector<__scandot> raw)

int __tinyslam::get_ScanData(float data[], vector<__scandot> raw)
{
	// ��ӦTinySLAM�У�int read_sensor_data(ts_sensor_data_t *data)
	// ����Ĵ������£������ǵĴ�����ʵ���޶��£����ǽ����������ݱ��x��y�ĵ�
	int i;
	double theta, rho;
	int halfWidth = LidarImageWidth / 2;
	int halfHeight = LidarImageHeight / 2;

	// �����ݿ�������
	Scan.nb_points = 0;
	for (i = 0; i < ANGLE_ALL; i++)
	{
		theta = i * PI / 180;
		rho = data[i];

		// �����һ֡������
		Scan.X[i] = Scan.Y[i] = 0;
		Scan.val[i] = 0;

		// �����tinyslam��˼·�����������ݱ��ͼ���X��Y����
		// Scan.X[i] = x;
		// Scan.Y[i] = y;
		// ��Ҫע�����x,y���ˣ������Rplidar���������
		/*
			x = (int)(rho  * sin(theta) / 20) + halfWidth;
			y = (int)(-rho * cos(theta) / 20) + halfHeight;
		*/
		if (rho == 0) {
			Scan.X[i] = TS_DISTANCE_NO_DETECTION * sin(theta);
			Scan.Y[i] = TS_DISTANCE_NO_DETECTION * -cos(theta);
			Scan.val[i] = TS_NO_OBSTACLE;
			//Scan.X[i] += TEST_OFFSET_LASER;
			Scan.nb_points++;
		}
		if (rho > TEST_HOLE_WIDTH / 2) {
			Scan.X[i] = rho * sin(theta);
			Scan.Y[i] = rho * -cos(theta);
			Scan.val[i] = TS_OBSTACLE;
			//Scan.X[i] += TEST_OFFSET_LASER;		// ����x������˸�offset�������Ǵ�������������⣬�����Ȳ�����
			Scan.nb_points++;
		}
	}

	Scan.nb_points = raw.size();
	return SUCCESS;

}// int __tinyslam::get_ScanData(float data[], vector<__scandot> raw)

int __tinyslam::Draw()
{
	Mat dst(LidarImageHeight, LidarImageWidth, CV_8UC3, Scalar(255, 255, 255));		// ͼƬ��ʽ�� BGR

	int x, y;
	double theta, rho;
	int halfWidth = dst.cols / 2;
	int halfHeight = dst.rows / 2;


	for (unsigned int i = 0; i < ANGLE_ALL; i++)	// scan_data.size()
	{
		// ��ͼ��ʱ��Ҫ��һ����������
		x = (int)(Scan.X[i] * ts_map_scale) + halfWidth;
		y = (int)(Scan.Y[i] * ts_map_scale) + halfHeight;

		// ����
		circle(dst, Point(x, y), 1, Scalar(Scan.val[i], Scan.val[i], Scan.val[i]), -1, 8, 0);		// Point(x, y)

	}


	imshow("TS_Test", dst);

	return SUCCESS;

}// int __tinyslam::Draw()

int __tinyslam::draw_Trajectory()
{
	double c, s;
	double x2p, y2p;
	int i, x1, y1, x2, y2;

	c = cos(Pos.Yaw * PI / 180);
	s = sin(Pos.Yaw * PI / 180);
	x1 = (int)floor(Pos.X * ts_map_scale + 0.5);
	y1 = (int)floor(Pos.Y * ts_map_scale + 0.5);
	// Translate and rotate scan to robot position
	for (i = 0; i != Scan.nb_points; i++) {
		if (Scan.val[i] != TS_NO_OBSTACLE) {
			x2p = c * Scan.X[i] - s * Scan.Y[i];
			y2p = s * Scan.X[i] + c * Scan.Y[i];
			x2p *= ts_map_scale;
			y2p *= ts_map_scale;
			x2 = (int)floor(Pos.X * ts_map_scale + x2p + 0.5);
			y2 = (int)floor(Pos.Y * ts_map_scale + y2p + 0.5);
			if (x2 >= 0 && y2 >= 0 && x2 < LidarImageWidth && y2 < LidarImageHeight)
				Trajectory.at<uchar>(y2, x2) = 0;			//map->map[y2 * TS_MAP_SIZE + x2] = 0;
		}
	}

	imshow("Trajectory", Trajectory);

	return SUCCESS;

}// int __tinyslam::draw_Trajectory()


__ts_pos_t __tinyslam::monte_carlo_move(__ts_scan_t &scan, Mat &map, __ts_pos_t &start_pos, int debug)
{
	__ts_pos_t cpp, currentpos, bestpos, lastbestpos;
	int currentdist;
	int bestdist, lastbestdist;
	int counter = 0;

	currentpos = bestpos = lastbestpos = start_pos;
	currentdist = ts_distance_scan_to_map(scan, map, currentpos);
	bestdist = lastbestdist = currentdist;

	do {
		currentpos = lastbestpos;
		currentpos.X += 50 * (((double)rand()) / RAND_MAX - 0.5);
		currentpos.Y += 50 * (((double)rand()) / RAND_MAX - 0.5);
		currentpos.Yaw += 50 * (((double)rand()) / RAND_MAX - 0.5);

		currentdist = ts_distance_scan_to_map(scan, map, currentpos);

		if (currentdist < bestdist) {
			bestdist = currentdist;
			bestpos = currentpos;
			if (debug) printf("Monte carlo ! %lg %lg %lg %d (count = %d)\n", bestpos.X, bestpos.Y, bestpos.Yaw, bestdist, counter);
		}
		else {
			counter++;
		}
		if (counter > 100) {
			if (bestdist < lastbestdist) {
				lastbestpos = bestpos;
				lastbestdist = bestdist;
				counter = 0;
			}
		}
	} while (counter < 1000);
	return bestpos;
}// __ts_pos_t __tinyslam::monte_carlo_move(__ts_scan_t &scan, Mat &map, __ts_pos_t &start_pos, int debug)


int __tinyslam::ts_distance_scan_to_map(__ts_scan_t &scan, Mat &map, __ts_pos_t &pos)
{
	double c, s;
	int i, x, y, nb_points = 0;
	int64_t sum;

	c = cos(pos.Yaw * PI / 180);
	s = sin(pos.Yaw * PI / 180);
	// Translate and rotate scan to robot position
	// and compute the distance
	for (i = 0, sum = 0; i != scan.nb_points; i++) {
		if (scan.val[i] != TS_NO_OBSTACLE) {
			x = (int)floor((pos.X + c * scan.X[i] - s * scan.Y[i]) * ts_map_scale + 0.5);
			y = (int)floor((pos.Y + s * scan.X[i] + c * scan.Y[i]) * ts_map_scale + 0.5);
			// Check boundaries
			if (x >= 0 && x < LidarImageWidth && y >= 0 && y < LidarImageHeight) {

				//sum += map->map[y * TS_MAP_SIZE + x];
				sum += map.at<uchar>(y, x);

				nb_points++;
			}
		}
	}
	if (nb_points) sum = sum * 1024 / nb_points;
	else sum = (LidarImageWidth * LidarImageHeight * TS_NO_OBSTACLE) * 1024 / ANGLE_ALL;		// sum = 2000000000;

	return (int)sum;

}// int __tinyslam::ts_distance_scan_to_map(__ts_scan_t &scan, Mat &map, __ts_pos_t &start_pos)


void __tinyslam::ts_map_laser_ray(Mat &map, int x1, int y1, int x2, int y2, int xp, int yp, int value, int alpha)
{
	int x2c, y2c, dx, dy, dxc, dyc, error, errorv, derrorv, x;
	int incv, sincv, incerrorv, incptrx, incptry, pixval, horiz, diago;
	int ptr;
	int ptr_val;

	if (x1 < 0 || x1 >= LidarImageSize || y1 < 0 || y1 >= LidarImageSize)
		return; // Robot is out of map

	x2c = x2; y2c = y2;
	// Clipping
	if (x2c < 0) {
		if (x2c == x1) return;
		y2c += (y2c - y1) * (-x2c) / (x2c - x1);
		x2c = 0;
	}
	if (x2c >= LidarImageSize) {
		if (x1 == x2c) return;
		y2c += (y2c - y1) * (LidarImageSize - 1 - x2c) / (x2c - x1);
		x2c = LidarImageSize - 1;
	}
	if (y2c < 0) {
		if (y1 == y2c) return;
		x2c += (x1 - x2c) * (-y2c) / (y1 - y2c);
		y2c = 0;
	}
	if (y2c >= LidarImageSize) {
		if (y1 == y2c) return;
		x2c += (x1 - x2c) * (LidarImageSize - 1 - y2c) / (y1 - y2c);
		y2c = LidarImageSize - 1;
	}

	dx = abs(x2 - x1); dy = abs(y2 - y1);
	dxc = abs(x2c - x1); dyc = abs(y2c - y1);
	incptrx = (x2 > x1) ? 1 : -1;
	incptry = (y2 > y1) ? LidarImageSize : -LidarImageSize;	// Ϊʲô�ܹ����ָ���
	sincv = (value > TS_NO_OBSTACLE) ? 1 : -1;
	if (dx > dy) {
		derrorv = abs(xp - x2);
	}
	else {
		SWAP(dx, dy); SWAP(dxc, dyc); SWAP(incptrx, incptry);
		derrorv = abs(yp - y2);
	}
	error = 2 * dyc - dxc;
	horiz = 2 * dyc;
	diago = 2 * (dyc - dxc);
	errorv = derrorv / 2;
	incv = (value - TS_NO_OBSTACLE) / derrorv;
	incerrorv = value - TS_NO_OBSTACLE - derrorv * incv;
	ptr = y1 * LidarImageSize + x1;
	pixval = TS_NO_OBSTACLE;
	for (x = 0; x <= dxc; x++, ptr += incptrx) {
		if (x > dx - 2 * derrorv) {
			if (x <= dx - derrorv) {
				pixval += incv;
				errorv += incerrorv;
				if (errorv > derrorv) {
					pixval += sincv;
					errorv -= derrorv;
				}
			}
			else {
				pixval -= incv;
				errorv -= incerrorv;
				if (errorv < 0) {
					pixval -= sincv;
					errorv += derrorv;
				}
			}
		}
		// Integration into the map
		//*ptr = ((256 - alpha) * (*ptr) + alpha * pixval) >> 8;		// ԭ��ptr�Ǹ�ָ�룬�����ΪMat������ô�����������취
		ptr_val = ((256 - alpha) * ptr + alpha * pixval) >> 8;

		/* �����ӣ�unsigned short: 0~65535 */
		ptr_val %= 65536;
		// Ȼ��Ϊ�˻�ͼ�������������
		ptr_val = 255 * ptr_val / 65535;

		map.at<uchar>(ptr / LidarImageSize, ptr % LidarImageSize) = ptr_val;

		if (error > 0) {
			ptr += incptry;
			error += diago;
		}
		else error += horiz;
	}
}// void __tinyslam::ts_map_laser_ray(Mat &map, int x1, int y1, int x2, int y2, int xp, int yp, int value, int alpha)

