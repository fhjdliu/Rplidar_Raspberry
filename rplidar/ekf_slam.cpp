#include "ekf_slam.h"



__ekf_slam::__ekf_slam()
{
	// 开始的时候只有三个变量，没有障碍物，则X只有三个变量x, y和yaw
	P.resize(3, 3);
	P.setOnes();		// 设置为全1矩阵

	Pe.resize(3, 3);
	Pe.setOnes();


	X.resize(3, 1);
	X.setZero();		// 设置为全0矩阵
						// 以初始位置为0

	Xe.resize(3, 1);
	Xe.setZero();

	Q.resize(3, 3);
	Q << 0.49,  0,     0,
		 0,     0.49,  0,
		 0,     0,    (7 * PI / 180) * (7 * PI / 180);		// Q先带个值进去，然后整定

	Z.resize(0, 1);
	Z.setZero();

	landmark_num = landmark_num_last = 0;

	//cout << Q << endl;

}// __ekf_slam::__ekf_slam()

int __ekf_slam::run()
{
	// 单次运行，这个东西要扔到while (1)里
	MatrixXd temp;

	if (landmark_num > landmark_num_last)		// 如果有新增加输入的数目，这里我们需要更新估计矩阵
	{
		// 这么多操作其实只是为了扩展X
		// 为什么更新的是X(k)而不是Xe(k + 1)，因为下面就Xe(k + 1)可以根据X(k)算出来

		temp.resize(X.rows(), X.cols());
		temp = X;
		X.resize(3 + 2 * landmark_num, 1);		// X是个2n + 3行1列的矩阵
		X.setZero();

		for (int i = 0; i < temp.rows(); i++)	// 除了旧的是有值的，新的是没有值的，所以上面先置0再赋值
			X(i, 0) = temp(i, 0);

		Xe.resize(X.rows(), X.cols());
		Xe.setZero();

		// 同理，更新P
		// 为什么更新P(k)而不是P_e(k + 1)理由同上
		temp.resize(P.rows(), P.cols());
		temp = P;
		P.resize(3 + 2 * landmark_num, 3 + 2 * landmark_num);		// P是个2n + 3行，2n + 3列的矩阵
		P.setOnes();												// 注意协方差默认开始是个全1矩阵，所以这边不能设全0

		for (int i = 0; i < temp.rows(); i++)						// 这边跟上面一样，也是先设好其他的，再覆盖赋值
			for (int j = 0; j < temp.cols(); j++)
				P(i, j) = temp(i, j);

		Pe.resize(P.rows(), P.cols());

		// 同理，更新Q
		temp.resize(Q.rows(), Q.cols());
		temp = Q;
		Q.resize(3 + 2 * landmark_num, 3 + 2 * landmark_num);
		Q.setZero();
		for (int i = 0; i < temp.rows(); i++)						// 这边跟上面一样，也是先设好其他的，再覆盖赋值
			for (int j = 0; j < temp.cols(); j++)
				Q(i, j) = temp(i, j);

	}
	else
	{

	}

	//cout << "X:" << endl << X << endl;
	//cout << "P:" << endl << P << endl;
	//cout << "Q:" << endl << Q << endl;

	//
	// 开始迭代


	// F
	F.resize(X.rows(), X.rows());			// 你没看错，就是这么写的
	F.setZero();
	for (int i = 0; i < X.rows(); i++)
		F(i, i) = 1;
	F(0, 2) = (Acc.X * dt * dt + 0.5 * Acc.X * dt * dt) *  cos(phi);
	F(1, 2) = (Acc.Y * dt * dt + 0.5 * Acc.Y * dt * dt) * -sin(phi);

	//cout << "F: " << endl << F << endl;

	// W
	W.resize(X.rows(), X.rows());			// 你又没看错，就是这么写的
	W.setZero();
	W(0, 0) = W(1, 1) = W(2, 2) = 1;

	//cout << "W: " << endl << W << endl;

	// 公式1
	// x
	Xe(0, 0) = X(0, 0) + (Acc.X * dt * dt + 0.5 * Acc.X * dt * dt) * sin(phi);
	// y
	Xe(1, 0) = X(1, 0) + (Acc.Y * dt * dt + 0.5 * Acc.Y * dt * dt) * cos(phi);
	// z
	Xe(2, 0) = phi;																	// X(2, 0) + Gyro.Z * dt
	// 障碍物的数据
	// 这些照搬就好了
	for (int i = 3; i < X.rows(); i++)
		Xe(i, 0) = X(i, 0);

	cout << "Xe: " << endl << Xe << endl;

	// 公式2
	MatrixXd F_T = F.transpose();
	MatrixXd W_T = W.transpose();
	Pe = F * P * F_T + W * Q * W_T;

	cout << "Pe: " << endl << Pe << endl;


	// R
	R.resize(2 * landmark_num, 2 * landmark_num);
	R.setZero();
	for (int i = 0; i < R.rows(); i += 2)						// 这边跟上面一样，也是先设好其他的，再覆盖赋值
	{
		R(i, i) = 2;							// mm
		R(i + 1, i + 1) = PI * 1 / 180;			// rad
	}

	//cout << "R: " << endl << R << endl;



	// V
	V.resize(Z.rows(), Z.rows());			// 你还是没看错，就是这么写的
	V.setZero();
	for (int i = 0; i < V.rows(); i++)
		V(i, i) = 1;

	//cout << "V: " << endl << V << endl;



	// Jh
	// 这货最难算
	Jh.resize(Z.rows(), X.rows());
	Jh.setZero();
	for (int i = 0; i < Z.rows() - 1; i += 2)
	{
		double x_k  = Xe(0, 0);
		double y_k  = Xe(1, 0);
		double m_ik = Xe(3 + i, 0);			// 障碍物的x坐标
		double n_ik = Xe(3 + i + 1, 0);		// 障碍物的y坐标
		double r_ik = Z(i, 0);

		Jh(i, 0)     = (x_k - m_ik) / r_ik;							// 注意这里的几个变量，如果变量爆炸了这里就全错了
		Jh(i + 1, 0) = (y_k - n_ik) / (r_ik * r_ik);
		Jh(i, 1)     = (y_k - n_ik) / r_ik;
		Jh(i + 1, 1) = (x_k - m_ik) / (r_ik * r_ik);

		Jh(i, 3 + i)         = -(x_k - m_ik) / r_ik;
		Jh(i, 3 + i + 1)     = -(y_k - n_ik) / r_ik;
		Jh(i + 1, 3 + i)     = -(y_k - n_ik) / (r_ik * r_ik);
		Jh(i + 1, 3 + i + 1) = -(x_k - m_ik) / (r_ik * r_ik);

		if (r_ik == 0)
		{
			Jh(i, 0)     = 0;
			Jh(i + 1, 0) = 0;
			Jh(i, 1)     = 0;
			Jh(i + 1, 1) = 0;

			Jh(i, 3 + i)         = 0;
			Jh(i, 3 + i + 1)     = 0;
			Jh(i + 1, 3 + i)     = 0;
			Jh(i + 1, 3 + i + 1) = 0;
		}

	}

	cout << "Jh: " << endl << Jh << endl;


	// 公式3
	MatrixXd Jh_T = Jh.transpose();
	MatrixXd V_T  = V.transpose();
	MatrixXd inv = Jh * Pe *Jh_T + V * R * V_T;
	inv = inv.inverse();
	K = Pe * Jh_T * inv;

	cout << "K: " << endl << K << endl;



	H.resize(2 * landmark_num, 1);
	for (int i = 0; i < Z.rows() - 1; i += 2)
	{
		double x_k  = Xe(0, 0);
		double y_k  = Xe(1, 0);
		double m_ik = Xe(3 + i, 0);			// 障碍物的x坐标
		double n_ik = Xe(3 + i + 1, 0);		// 障碍物的y坐标

		// r
		H(i, 0) = sqrt((x_k - m_ik) * (x_k - m_ik) + (y_k - n_ik) * (y_k - n_ik));
		// theta
		H(i + 1, 0) = atan2((x_k - m_ik), (y_k - n_ik)); 						// 注意这里会爆炸

		cout << "Hi: " << H(i + 1, 0) << endl;
		//
		//
		//	角度要做个补偿！！！！！！！！
		//
		//
	}
	// 公式4
	X = Xe + K * (Z - H);

	//cout << "X: " << endl << X << endl;


	// 公式5
	MatrixXd I;
	I.resize(Pe.cols(), Pe.rows());
	for (int i = 0; i < I.rows(); i++)
				I(i, i) = 1;

	P = (I - K * Jh) * Pe;

	cout << "P: " << endl << P << endl;


	return 0;

}// int __ekf_slam::run()

int __ekf_slam::get_Sensor(vector<__point2p> lidar, __Vec3f a, __Vec3f w, float yaw, float t)
{
	// 输入传感器参数
	//MatrixXd temp;

	landmark_num_last = landmark_num;
	landmark_num = lidar.size();

	//temp.resize(Z.rows(), Z.cols());
	//temp = Z;
	Z.resize(landmark_num * 2, 1);
	Z.setZero();

	// 激光雷达参数
	// 前次状态的保存的任务交给旧的建图和匹配模块，故这里无需关心，因为前一级已经处理好了
	for (int i = 0, j = 0; i < Z.rows() - 1; i += 2, j++)
	{
		Z(i, 0)     = lidar[j].r / 1000.0f;				// mm/s^2 -> m/s^2
		Z(i + 1, 0) = lidar[j].deg * PI / 180.0f;
	}

	//for (int i = 0; i < lidar.size(); i++)
	//	cout << "lidar.r: " << lidar[i].r << "lidar.deg: " << lidar[i].deg << endl;
	//cout << Z << endl << " " << endl;

	// 加速度
	Acc.X = a.X;
	Acc.Y = a.Y;
	Acc.Z = a.Z;

	// 角速度
	Gyro.X = w.X;
	Gyro.Y = w.Y;
	Gyro.Z = w.Z;

	// 偏航角
	phi = yaw * PI / 180.0f;

	dt = t;

	return 0;

}// int __ekf_slam::get_Sensor(vector<__point2f> lidar, __Vec3f a, __Vec3f w, float yaw)

