#include <gtest/gtest.h>

#define TEST_IMPLICITFORCES
#include "../Computer.hpp"
#include "../Particle.hpp"
#include "../Vector.hpp"

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
	static constexpr double r_eByl_0 = 1.5; // �e�X�g�p�̊ȗ����Ƃ���, �����߂ɐݒ�.

#ifndef MPS_SPP
	static constexpr double surfaceRatio = 0.95;
#endif
	// �i�q��ɔz�u����ۂ�1�ӂ�����̗��q��
	static constexpr std::size_t num_x = 7;
	static constexpr std::size_t num_z = 7;

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

		class ImplicitForcesTest : public ::testing::Test
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

				std::vector<OpenMps::Particle> particles0;

				for (auto j = decltype(num_z){0}; j < num_z; j++)
				{
					for (auto i = decltype(num_x){0}; i < num_x; i++)
					{

						auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
						const double x = i * l0;
						const double z = j * l0;
						particle.X()[OpenMps::AXIS_X] = x;
						particle.X()[OpenMps::AXIS_Z] = z;

						particle.U()[OpenMps::AXIS_X] = 0.0;
						particle.U()[OpenMps::AXIS_Z] = 0.0;
						particle.P() = 0.0;
						particle.N() = 0.0;

						particles0.push_back(std::move(particle));
					}
				}
				computer->AddParticles(std::move(particles0));
			}

			auto& GetParticles()
			{
				return computer->particles;
			}

			void SearchNeighbor()
			{
				computer->SearchNeighbor();
			}

			auto& Neighbor(const std::size_t i, const std::size_t idx)
			{
				return computer->Neighbor(i, idx);
			}

			auto& NeighborCount(const std::size_t i)
			{
				return computer->NeighborCount(i);
			}

			void ComputeNeighborDensities()
			{
				computer->ComputeNeighborDensities();
			}

			void ComputeImplicitForces()
			{
				computer->ComputeImplicitForces();
			}

			void SetPressurePoissonEquation()
			{
				computer->SetPressurePoissonEquation();
			}

			void SolvePressurePoissonEquation()
			{
				computer->SolvePressurePoissonEquation();
			}

			bool IsAlive(const Particle& p)
			{
				return p.TYPE() != Particle::Type::Dummy && p.TYPE() != Particle::Type::Disabled;
			}

			auto& getPpe()
			{
				return computer->ppe;
			}

			virtual void TearDown()
			{
				delete computer;
			}
		};

		// �W���s��͑Ώ̍s��ł��邩�H
		TEST_F(ImplicitForcesTest, MatrixSymmetry)
		{
			SearchNeighbor();
			ComputeNeighborDensities();
			SetPressurePoissonEquation();

			auto& ppe = getPpe();
			const auto Ndim = num_x * num_z;

			ASSERT_EQ(ppe.b.size(), Ndim);
			for (auto i = decltype(Ndim){0}; i < Ndim; i++)
			{
				for (auto j = decltype(Ndim){0}; j < Ndim; j++)
				{
					if (j != i)
					{
						ASSERT_NEAR(ppe.A(i, j) - ppe.A(j, i), 0.0, testAccuracy);
					}
				}
			}
		}

		// �W���s�� a_ii = -��a_ij (i!=j) �Ƃ����P�����͐������邩�H
		// ���E���痣�ꂽ�������q�ɂ����ăe�X�g
		TEST_F(ImplicitForcesTest, MatrixDiagIdentity)
		{
			SearchNeighbor();
			ComputeNeighborDensities();
			SetPressurePoissonEquation();

			auto& ppe = getPpe();
			const auto Ndim = num_x * num_z;

			const auto ic = (num_x - 1) / 2 * (num_z + 1); // �������q��index

			double sum_nondiag = 0.0;
			for (auto j = decltype(Ndim){0}; j < Ndim; j++)
			{
				if (j != ic)
				{
					sum_nondiag += ppe.A(ic, j); // disable,dummy,free surface particle�͊�^���Ȃ�
				}
			}
			ASSERT_NEAR(ppe.A(ic, ic), -sum_nondiag, testAccuracy);
		}

		// ���qi �� �ߖT���qj �ɑΉ����鐬���� a_ij != 0 �ł��邱��
		// (dummy, disable, free surface���q�͏��O)
		TEST_F(ImplicitForcesTest, MatrixNeighborNonzero)
		{
			SearchNeighbor();
			ComputeNeighborDensities();
			SetPressurePoissonEquation();

			auto& ppe = getPpe();
			const auto Ndim = num_x * num_z;
			const auto& particles = GetParticles();

			for (auto i = decltype(Ndim){0}; i < Ndim; i++)
			{
				if (!IsAlive(particles[i]))
				{
					continue;
				}

				std::vector<decltype(i)> neighList; // i�̋ߖT���q���X�g
				for (auto idx = decltype(i){0}; idx < NeighborCount(i); idx++)
				{
					const auto j = Neighbor(i, idx);
					if (IsAlive(particles[j]))
					{
						neighList.push_back(j);
					}
				}

				for (auto j = decltype(Ndim){0}; j < Ndim; j++)
				{
					if (j != i && std::find(neighList.begin(), neighList.end(), j) == neighList.end()) // j��i�ߖT���X�g�ɑ����Ȃ�
					{
						ASSERT_NEAR(ppe.A(i, j), 0.0, testAccuracy);
					}
				}
			}
		}

	}

	TEST_F(ImplicitForcesTest, SolveIdentityMatrix)
	{
		std::vector<OpenMps::Particle> particles0;

		for (auto j = decltype(num_z){0}; j < num_z; j++)
		{
			for (auto i = decltype(num_x){0}; i < num_x; i++)
			{

				auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
				const double x = i * l0;
				const double z = j * l0;
				particle.X()[OpenMps::AXIS_X] = x;
				particle.X()[OpenMps::AXIS_Z] = z;

				particle.U()[OpenMps::AXIS_X] = 0.0;
				particle.U()[OpenMps::AXIS_Z] = 0.0;
				particle.P() = 0.0;
				particle.N() = 0.0;

				particles0.push_back(std::move(particle));
			}
		}
		computer->AddParticles(std::move(particles0));

		const auto& env = computer->GetEnvironment();
		SearchNeighbor();
		ComputeNeighborDensities();

		SetPressurePoissonEquation();
		auto& ppe = getPpe();
		const auto Ndim = num_x * num_z;

		for (auto j = decltype(Ndim){0}; j < Ndim; j++)
		{
			ppe.b(j) = static_cast<double>(j);
			for (auto i = decltype(Ndim){0}; i < Ndim; i++)
			{
				if (i == j)
				{
					ppe.A(i, j) = 1.0;
				} else {
					ppe.A(i, j) = 0.0;
				}
			}
		}

		SolvePressurePoissonEquation();
		double diff = 0.0;
		for (auto i = decltype(Ndim){0}; i < Ndim; i++)
		{
			diff += abs(static_cast<double>(i) - ppe.x(i));
		}

		ASSERT_NEAR(diff / num_x, 0.0, 1e-5);
	}

		// ���T�C�Y��4x4�Ƃ���, �t�s��matrix������
		TEST_F(ImplicitForcesTest, Solve4x4Matrix)
		{
			std::vector<OpenMps::Particle> particles0;

			for (auto j = decltype(num_z){0}; j < 2; j++)
			{
				for (auto i = decltype(num_x){0}; i < 2; i++)
				{

					auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
					const double x = i * l0;
					const double z = j * l0;
					particle.X()[OpenMps::AXIS_X] = x;
					particle.X()[OpenMps::AXIS_Z] = z;

					particle.U()[OpenMps::AXIS_X] = 0.0;
					particle.U()[OpenMps::AXIS_Z] = 0.0;
					particle.P() = 0.0;
					particle.N() = 0.0;

					particles0.push_back(std::move(particle));
				}
			}
			computer->AddParticles(std::move(particles0));

			const auto& env = computer->GetEnvironment();
			SearchNeighbor();
			ComputeNeighborDensities();

			SetPressurePoissonEquation();
			auto& ppe = getPpe();
			const auto Ndim = 2 * 2;

			ppe.A(0, 0) = 1.0;
			ppe.A(0, 1) = 1.0;
			ppe.A(0, 2) = 1.0;
			ppe.A(0, 3) = -1.0;

			ppe.A(1, 0) = 1.0;
			ppe.A(1, 1) = 1.0;
			ppe.A(1, 2) = -1.0;
			ppe.A(1, 3) = 1.0;

			ppe.A(2, 0) = 1.0;
			ppe.A(2, 1) = -1.0;
			ppe.A(2, 2) = 1.0;
			ppe.A(2, 3) = 1.0;

			ppe.A(3, 0) = -1.0;
			ppe.A(3, 1) = 1.0;
			ppe.A(3, 2) = 1.0;
			ppe.A(3, 3) = 1.0;

			ppe.b(0) = 4*1.0;
			ppe.b(1) = 4*2.0;
			ppe.b(2) = 4*3.0;
			ppe.b(3) = 4*4.0;

			SolvePressurePoissonEquation();

			ASSERT_NEAR(ppe.x(0), 2.0, 1e-5);
			ASSERT_NEAR(ppe.x(1), 4.0, 1e-5);
			ASSERT_NEAR(ppe.x(2), 6.0, 1e-5);
			ASSERT_NEAR(ppe.x(3), 8.0, 1e-5);
		}
	}

}
