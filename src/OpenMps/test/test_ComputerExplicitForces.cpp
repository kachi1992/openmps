#include <gtest/gtest.h>

#define TEST_EXPLICITFORCES
#include "../Computer.hpp"
#include "../Particle.hpp"
#include "../Vector.hpp"

#include <cmath>
#include <cstdio> // ���Ƃŏ���

namespace {
#ifndef PRESSURE_EXPLICIT
	static constexpr double eps = 1e-7;
#endif

	static constexpr double dt_step = 1.0 / 100;
	static constexpr double courant = 0.1;

	static constexpr double l0 = 0.01;
	static constexpr double g = 9.8;

	static constexpr double rho = 998.2;
	static constexpr double nu = 1.004e-06;
	static constexpr double r_eByl_0 = 2.1;

#ifndef MPS_SPP
	static constexpr double surfaceRatio = 0.95;
#endif
	// �i�q��ɔz�u����ۂ�1�ӂ�����̗��q��
	static constexpr int num_x = 7;
	static constexpr int num_z = 7;

	// �T�C�Y����� �� 2*l0*num_x, �c 2*l0*num_z �Ƃ���
	static constexpr double minX = -l0 * num_x;
	static constexpr double minZ = -l0 * num_z;
	static constexpr double maxX = l0* num_x;
	static constexpr double maxZ = l0 * num_z;

	// ���e���鑊�Ό덷
	static constexpr double testAccuracy = 1e-3;


#ifdef PRESSURE_EXPLICIT
	static constexpr double c = 1.0;
#endif

namespace OpenMps
{
	Vector positionWall(std::size_t, double, double)
	{

		return CreateVector(0.0, 0.0); // �x�N�g����Ԃ��悤�ɕϊ�����Ƃ��Ȃ�Ƃ�
	}

	double positionWallPre(double, double)
	{
		return 0.0;
	}

	class ExplicitForcesTest : public ::testing::Test
	{
	protected:
		OpenMps::Computer<decltype(positionWall)&,decltype(positionWallPre)&> *computer;

		virtual void SetUp()
		{
			auto&& environment = OpenMps::Environment(dt_step, courant,
				g, rho, nu,
#ifndef MPS_SPP
				surfaceRatio,
#endif
				r_eByl_0,
#ifdef PRESSURE_EXPLICIT
				c,
#endif
				l0,
				minX, minZ,
				maxX, maxZ
			);

			environment.Dt() = 0.01;
			environment.SetNextT();

			computer = new OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>(std::move(
				OpenMps::CreateComputer(
	#ifndef PRESSURE_EXPLICIT
					eps,
	#endif
					environment,
					positionWall, positionWallPre)));
		}

		auto& GetParticles()
		{
			return computer->particles;
		}

		double R(const Vector& x1, const Vector& x2)
		{
			return OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>::R(x1, x2);
		}

		double R(const Particle& p1, const Particle& p2)
		{
			return OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>::R(p1, p2);
		}

		void SearchNeighbor()
		{
			computer->SearchNeighbor();
		}

		void ComputeNeighborDensities()
		{
			computer->ComputeNeighborDensities();
		}

		void ComputeExplicitForces()
		{
			computer->ComputeExplicitForces();
		}

		virtual void TearDown()
		{
			delete computer;
		}
	};

	// �S������+�d�͂̌v�Z�l�͉�͉��ƈ�v���邩�H
	// �񎟑����� ax^2 + bxy + cy^2 + d,
	// ���v���V�A�� 2(a+c)
	TEST_F(ExplicitForcesTest, ForceValuePolynomial)
	{
		// �����������x��̌W��, O(1)
		static constexpr double cx1 = -3.0;
		static constexpr double cx2 = 4.0;
		static constexpr double cx3 = 5.0;
		static constexpr double cx4 = -6.0;

		static constexpr double cy1 = 5.0;
		static constexpr double cy2 = -6.0;
		static constexpr double cy3 = -7.0;
		static constexpr double cy4 = 8.0;

		// 1��l0, num_x*num_z�̊i�q��ɗ��q��z�u
		std::vector<OpenMps::Particle> particles0;
		for (int j = 0; j < num_z; ++j)
		{
			for (int i = 0; i < num_x; ++i)
			{

				auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
				const double x = i * l0;
				const double z = j * l0;
				particle.X()[OpenMps::AXIS_X] = x;
				particle.X()[OpenMps::AXIS_Z] = z;

				particle.U()[OpenMps::AXIS_X] = cx1 * x * x + cx2 * x * z + cx3 * z * z + cx4;
				particle.U()[OpenMps::AXIS_Z] = cy1 * x * x + cy2 * x * z + cy3 * z * z + cy4;
				particle.P() = 0.0;
				particle.N() = 0.0;

				particles0.push_back(std::move(particle));
			}
		}
		computer->AddParticles(std::move(particles0));

		// �ߖT���q�T��
		const auto &env = computer->GetEnvironment();
		const double dt = dt_step;
		SearchNeighbor();
		ComputeNeighborDensities();

		const auto& particles = GetParticles();
		const auto id = 24; // �����̗��q���w��ID

		// �͂�������O��ł̑��x���擾���ĉ����x�v�Z
		const double ux0 = particles[id].U()[OpenMps::AXIS_X];
		const double uy0 = particles[id].U()[OpenMps::AXIS_Z];
		ComputeExplicitForces();
		const double ux1 = particles[id].U()[OpenMps::AXIS_X];
		const double uy1 = particles[id].U()[OpenMps::AXIS_Z];

		const double x = particles[id].X()[OpenMps::AXIS_X];
		const double z = particles[id].X()[OpenMps::AXIS_Z];
		const double ax = (ux1 - ux0) / dt_step;
		const double ay = (uy1 - uy0) / dt_step;
		const double ax_analy = 2*(cx1+cx3) * nu;
		const double ay_analy = 2*(cy1+cy3) * nu - g;

		// ���Ό덷�ŕ]��
		ASSERT_NEAR( abs( (ax-ax_analy)/ax_analy ), 0.0, testAccuracy);
		ASSERT_NEAR( abs( (ay-ay_analy)/ay_analy ), 0.0, testAccuracy);
	}

	// �O�p�֐� a sin(om * x) + b cos(om' * z) 
	TEST_F(ExplicitForcesTest, ForceValueSinCos)
	{
		// �����������x��̌W��
		static constexpr double cx1 = -3.0;
		static constexpr double cx2 = 4.0;
		static constexpr double ox1 = 0.1;
		static constexpr double ox2 = -0.5;

		static constexpr double cy1 = 3.3;
		static constexpr double cy2 = -4.0;
		static constexpr double oy1 = -0.3;
		static constexpr double oy2 = 0.2;

		// 1��l0, num_x*num_z�̊i�q��ɗ��q��z�u
		std::vector<OpenMps::Particle> particles0;
		for (int j = 0; j < num_z; ++j)
		{
			for (int i = 0; i < num_x; ++i)
			{

				auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
				const double x = i * l0;
				const double z = j * l0;
				particle.X()[OpenMps::AXIS_X] = x;
				particle.X()[OpenMps::AXIS_Z] = z;

				particle.U()[OpenMps::AXIS_X] = cx1 * sin(ox1*x) + cx2 * cos(ox2*z);
				particle.U()[OpenMps::AXIS_Z] = cy1 * sin(oy1 * x) + cy2 * cos(oy2 * z);
				particle.P() = 0.0;
				particle.N() = 0.0;

				particles0.push_back(std::move(particle));
			}
		}
		computer->AddParticles(std::move(particles0));

		// �ߖT���q�T��
		const auto &env = computer->GetEnvironment();
		const double dt = dt_step;
		SearchNeighbor();
		ComputeNeighborDensities();

		const auto& particles = GetParticles();
		const auto id = 24; // �����̗��q���w��ID

		// �͂�������O��ł̑��x���擾���ĉ����x�v�Z
		const double ux0 = particles[id].U()[OpenMps::AXIS_X];
		const double uy0 = particles[id].U()[OpenMps::AXIS_Z];
		ComputeExplicitForces();
		const double ux1 = particles[id].U()[OpenMps::AXIS_X];
		const double uy1 = particles[id].U()[OpenMps::AXIS_Z];

		const double x = particles[id].X()[OpenMps::AXIS_X];
		const double z = particles[id].X()[OpenMps::AXIS_Z];

		const double ax = (ux1 - ux0) / dt_step;
		const double ay = (uy1 - uy0) / dt_step;
		const double ax_analy = (-ox1*ox1*cx1*sin(ox1*x) - ox2*ox2*cx2*cos(ox2*z)) * nu;
		const double ay_analy = (-oy1*oy1*cy1*sin(oy1*x) - oy2*oy2*cy2*cos(oy2*z)) * nu - g;

		ASSERT_NEAR( abs( (ax-ax_analy)/ax_analy ), 0.0, testAccuracy);
		ASSERT_NEAR( abs( (ay-ay_analy)/ay_analy ), 0.0, testAccuracy);
	}
}
}
