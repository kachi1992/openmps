#include <gtest/gtest.h>

#define TEST_EXPLICITFORCES
#include "../Computer.hpp"
#include "../Particle.hpp"
#include "../Vector.hpp"

#include <cmath>

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
	static constexpr size_t num_x = 13;
	static constexpr size_t num_z = 13;

	// �T�C�Y����� �� 2*l0*num_x, �c 2*l0*num_z �Ƃ���
	static constexpr double minX = -l0 * num_x;
	static constexpr double minZ = -l0 * num_z;
	static constexpr double maxX = l0 * num_x;
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

			return CreateVector(0.0, 0.0);
		}

		Vector positionWallPre(double, double)
		{
			return CreateVector(0.0, 0.0);
		}

		class ExplicitForcesTest : public ::testing::Test
		{
		protected:
			OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>* computer;

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

				environment.Dt() = dt_step;
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
			static constexpr double cx1 = -3.0;
			static constexpr double cx2 = 4.0;
			static constexpr double cx3 = 5.0;
			static constexpr double cx4 = -6.0;

			static constexpr double cz1 = 5.0;
			static constexpr double cz2 = -6.0;
			static constexpr double cz3 = -7.0;
			static constexpr double cz4 = 8.0;

			// 1��l0, num_x*num_z�̊i�q��ɗ��q��z�u
			std::vector<OpenMps::Particle> particles0;

#ifdef SIGNED_LOOP_COUNTER
			for (auto jj = std::make_signed_t<decltype(num_z)>{0}; jj < static_cast<std::make_signed_t<decltype(num_z)>>(num_z); jj++)
			{
				const auto j = static_cast<decltype(num_z)>(jj);

				for (auto ii = std::make_signed_t<decltype(num_x)>{0}; ii < static_cast<std::make_signed_t<decltype(num_x)>>(num_x); ii++)
				{
					const auto i = static_cast<decltype(num_x)>(ii);
#else
			for (auto j = decltype(num_z){0}; j < num_z; j++)
			{
				for (auto i = decltype(num_x){0}; i < num_x; i++)
				{
#endif

					auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
					const double x = i * l0;
					const double z = j * l0;
					particle.X()[OpenMps::AXIS_X] = x;
					particle.X()[OpenMps::AXIS_Z] = z;

					particle.U()[OpenMps::AXIS_X] = cx1 * x * x + cx2 * x * z + cx3 * z * z + cx4;
					particle.U()[OpenMps::AXIS_Z] = cz1 * x * x + cz2 * x * z + cz3 * z * z + cz4;
					particle.P() = 0.0;
					particle.N() = 0.0;

					particles0.push_back(std::move(particle));
				}
			}
			computer->AddParticles(std::move(particles0));

			// �ߖT���q�T���E���q�����x�v�Z
			const auto& env = computer->GetEnvironment();
			SearchNeighbor();
			ComputeNeighborDensities();

			// �������q�̉����x���擾
			const auto& particles = GetParticles();
			const auto id = (num_x - 1) / 2 * (num_z + 1);

			const auto v0 = particles[id].U();
			ComputeExplicitForces();
			const auto v1 = particles[id].U();

			auto accNum = (v1 - v0) / dt_step;

			// ���Ό덷��]��
			const auto gvec = CreateVector(0.0, -g);
			const auto visc = CreateVector(2 * (cx1 + cx3) * nu, 2 * (cz1 + cz3) * nu);
			const auto accAnaly = gvec + visc;

			const auto accDiff = accNum - accAnaly;
			const auto errx = abs(accDiff[OpenMps::AXIS_X] / accAnaly[OpenMps::AXIS_X]);
			const auto errz = abs(accDiff[OpenMps::AXIS_Z] / accAnaly[OpenMps::AXIS_Z]);

			ASSERT_NEAR(errx, 0.0, testAccuracy);
			ASSERT_NEAR(errz, 0.0, testAccuracy);
		}

		// �O�p�֐� a sin(om * x) + b cos(om' * z) 
		TEST_F(ExplicitForcesTest, ForceValueSinCos)
		{
			// �����������x��̌W��
			static constexpr double cx1 = -3.0;
			static constexpr double cx2 = 4.0;
			static constexpr double ox1 = 0.1;
			static constexpr double ox2 = -0.5;

			static constexpr double cz1 = 3.3;
			static constexpr double cz2 = -4.0;
			static constexpr double oz1 = -0.3;
			static constexpr double oz2 = 0.2;

			// 1��l0, num_x*num_z�̊i�q��ɗ��q��z�u
			std::vector<OpenMps::Particle> particles0;
#ifdef SIGNED_LOOP_COUNTER
			for (auto jj = std::make_signed_t<decltype(num_z)>{0}; jj < static_cast<std::make_signed_t<decltype(num_z)>>(num_z); jj++)
			{
				const auto j = static_cast<decltype(num_z)>(jj);

				for (auto ii = std::make_signed_t<decltype(num_x)>{0}; ii < static_cast<std::make_signed_t<decltype(num_x)>>(num_x); ii++)
				{
					const auto i = static_cast<decltype(num_x)>(ii);
#else
			for (auto j = decltype(num_z){0}; j < num_z; j++)
			{
				for (auto i = decltype(num_x){0}; i < num_x; i++)
				{
#endif
					auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
					const double x = i * l0;
					const double z = j * l0;
					particle.X()[OpenMps::AXIS_X] = x;
					particle.X()[OpenMps::AXIS_Z] = z;

					particle.U()[OpenMps::AXIS_X] = cx1 * sin(ox1 * x) + cx2 * cos(ox2 * z);
					particle.U()[OpenMps::AXIS_Z] = cz1 * sin(oz1 * x) + cz2 * cos(oz2 * z);
					particle.P() = 0.0;
					particle.N() = 0.0;

					particles0.push_back(std::move(particle));
				}
			}
			computer->AddParticles(std::move(particles0));

			const auto& env = computer->GetEnvironment();
			SearchNeighbor();
			ComputeNeighborDensities();

			// �������q�̉����x���擾
			const auto& particles = GetParticles();
			const auto id = (num_x - 1) / 2 * (num_z + 1);

			const auto xvec = particles[id].X();
			const double x = xvec[OpenMps::AXIS_X];
			const double z = xvec[OpenMps::AXIS_Z];

			const auto v0 = particles[id].U();
			ComputeExplicitForces();
			const auto v1 = particles[id].U();
			auto accNum = (v1 - v0) / dt_step;

			// ���Ό덷��]��
			const auto gvec = CreateVector(0.0, -g);
			const auto visc = CreateVector((-ox1 * ox1 * cx1 * sin(ox1 * x) - ox2 * ox2 * cx2 * cos(ox2 * z)) * nu,
				(-oz1 * oz1 * cz1 * sin(oz1 * x) - oz2 * oz2 * cz2 * cos(oz2 * z)) * nu);
			const auto accAnaly = gvec + visc;

			const auto accDiff = accNum - accAnaly;
			const auto errx = abs(accDiff[OpenMps::AXIS_X] / accAnaly[OpenMps::AXIS_X]);
			const auto errz = abs(accDiff[OpenMps::AXIS_Z] / accAnaly[OpenMps::AXIS_Z]);

			ASSERT_NEAR(errx, 0.0, testAccuracy);
			ASSERT_NEAR(errz, 0.0, testAccuracy);
		}
	}
}
