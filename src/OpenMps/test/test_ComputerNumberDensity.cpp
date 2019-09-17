#include <gtest/gtest.h>

#define TEST_NUMBERDENSITY
#include "../Computer.hpp"
#include "../Particle.hpp"
#include "../Vector.hpp"

#include <cmath>

namespace {
#ifndef PRESSURE_EXPLICIT
	static constexpr double eps = 1e-10;
#endif

	static constexpr double dt_step = 1.0 / 100;
	static constexpr double courant = 0.1;

	static constexpr double l0 = 0.1;
	static constexpr double g = 9.8;

	static constexpr double rho = 998.2;
	static constexpr double nu = 1.004e-06;
	static constexpr double r_eByl_0 = 2.1;
	static constexpr double surfaceRatio = 0.95;
	static constexpr double minX = -0.004;
	static constexpr double minZ = -0.004;
	static constexpr double maxX = 0.053;
	static constexpr double maxZ = 0.1;

	// �i�q��ɔz�u����ۂ�1�ӂ�����̗��q��
	static constexpr int num_x = 7;
	static constexpr int num_z = 7;

#ifdef PRESSURE_EXPLICIT
	static constexpr double c = 1.0;
#endif

namespace OpenMps
{
	double positionWall(std::size_t, double, double)
	{
		return 0.0;
	}

	double positionWallPre(double, double)
	{
		return 0.0;
	}

	class NumberDensityTest : public ::testing::Test
	{
	protected:
		OpenMps::Computer<decltype(positionWall)&,decltype(positionWallPre)&> *computer;

		virtual void SetUp()
		{
			auto&& environment = OpenMps::Environment(dt_step, courant,
				g, rho, nu, surfaceRatio, r_eByl_0,
#ifdef PRESSURE_EXPLICIT
				c,
#endif
				l0,
				minX, minZ,
				maxX, maxZ
			);


			computer = new OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>(std::move(
				OpenMps::CreateComputer(
	#ifndef PRESSURE_EXPLICIT
					eps,
	#endif
					environment,
					positionWall, positionWallPre)));

			std::vector<OpenMps::Particle> particles;

			// 1��l0, num_x*num_z�̊i�q��ɗ��q��z�u
			for (int j = 0; j < num_z; ++j)
			{
				for (int i = 0; i < num_x; ++i)
				{
					auto particle = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
					particle.X()[OpenMps::AXIS_X] = i * l0;
					particle.X()[OpenMps::AXIS_Z] = j * l0;

					particle.U()[OpenMps::AXIS_X] = 0.0;
					particle.U()[OpenMps::AXIS_Z] = 0.0;
					particle.P() = 0.0;
					particle.N() = 0.0;

					particles.push_back(std::move(particle));
				}
			}
			computer->AddParticles(std::move(particles));
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

		auto& Neighbor(const std::size_t i, const std::size_t idx)
		{
			return computer->Neighbor(i, idx);
		}

		auto& NeighborCount(const std::size_t i)
		{
			return computer->NeighborCount(i);
		}

		void SearchNeighbor()
		{
			computer->SearchNeighbor();
		}

		void ComputeNeighborDensities()
		{
			computer->ComputeNeighborDensities();
		}

		virtual void TearDown()
		{
			delete computer;
		}
	};

	TEST_F(NumberDensityTest, DistanceValue)
	{
		auto p1 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p2 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);

		p1.X()[OpenMps::AXIS_X] = 0.0;
		p1.X()[OpenMps::AXIS_Z] = 0.0;

		p2.X()[OpenMps::AXIS_X] = 1.0;
		p2.X()[OpenMps::AXIS_Z] = 2.0;

		ASSERT_DOUBLE_EQ(R(p1, p2), std::sqrt(1.0*1.0+2.0*2.0));
	}

	// �_����ւ��ɂ��đΏ̂��H
	TEST_F(NumberDensityTest, DistanceSymmetry)
	{
		auto p1 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p2 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);

		p1.X()[OpenMps::AXIS_X] = -10.0;
		p1.X()[OpenMps::AXIS_Z] = 0.1;

		p2.X()[OpenMps::AXIS_X] = 1.5;
		p2.X()[OpenMps::AXIS_Z] = 2.8;

		ASSERT_DOUBLE_EQ(R(p1, p2), R(p2, p1));
	}


	// Vector, Particle�̋����v�Z����v���邩�H
	TEST_F(NumberDensityTest, DistanceOverload)
	{
		auto p1 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p2 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);

		p1.X()[OpenMps::AXIS_X] = -10.0;
		p1.X()[OpenMps::AXIS_Z] = 0.1;

		p2.X()[OpenMps::AXIS_X] = 1.5;
		p2.X()[OpenMps::AXIS_Z] = 2.8;

		ASSERT_DOUBLE_EQ(R(p1.X(), p2.X()), R(p1, p2));
	}

	TEST_F(NumberDensityTest, DistanceTriangleInequality)
	{
		auto p1 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p2 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p3 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);

		p1.X()[OpenMps::AXIS_X] = -10.0;
		p1.X()[OpenMps::AXIS_Z] = 0.1;

		p2.X()[OpenMps::AXIS_X] = 1.5;
		p2.X()[OpenMps::AXIS_Z] = 2.8;

		p3.X()[OpenMps::AXIS_X] = 100.0;
		p3.X()[OpenMps::AXIS_Z] = -30.8;

		// �O�p�s�����͐������邩�H
		ASSERT_LE(R(p1, p2), R(p1, p3) + R(p2, p3));
		ASSERT_LE(R(p2, p3), R(p1, p2) + R(p1, p3));
		ASSERT_LE(R(p3, p1), R(p1, p2) + R(p2, p3));
	}

	// �ߐڂ���2���q�݂͌����ߖT���q�Ƃ��ĕێ����Ă��邩�H
	TEST_F(NumberDensityTest, NeighborSymmetry)
	{
		SearchNeighbor();

		const auto& particles = GetParticles();
		const auto n = particles.size();

		// i: ��ɑ��݂��闱�q�𑖍�
		// j: i�Ԗڗ��q�̋ߖT���q�𑖍�
		for(auto i = decltype(n){0}; i < n; i++)
		{
			if(particles[i].TYPE() != Particle::Type::Disabled)
			{
				for(auto idx = decltype(i){0}; idx < NeighborCount(i); idx++)
				{
					const auto j = Neighbor(i, idx);

					// j���q�̋ߖT��i���q���܂܂�Ă��邩�H
					bool j_has_i = false;
					for (auto idxj = decltype(i){0}; idxj < NeighborCount(j); idxj++)
					{
						j_has_i |= (particles[j].TYPE() != Particle::Type::Disabled && Neighbor(j, idxj) == i);
					}

					ASSERT_TRUE(j_has_i);
				}
			}
		}
	}

	// ���q�͎������g���ߖT���q�Ƃ��Ċ܂�ł��Ȃ����H
	TEST_F(NumberDensityTest, NeighborMyself)
	{
		SearchNeighbor();

		const auto& particles = GetParticles();
		const auto n = particles.size();

		for(auto i = decltype(n){0}; i < n; i++)
		{
			if(particles[i].TYPE() != Particle::Type::Disabled)
			{
				bool has_myself = false;
				for(auto idx = decltype(i){0}; idx < NeighborCount(i); idx++)
				{
					has_myself |= (Neighbor(i, idx) == i);
				}
				ASSERT_FALSE(has_myself);
			}
		}

	}

	// ���q�����x�͗��_�l�ƈ�v���邩�H
	TEST_F(NumberDensityTest, NeighDensity)
	{
		SearchNeighbor();

		ComputeNeighborDensities();

		const auto& particles = GetParticles();

		// �e�L�X�g Koshizuka et al.,2014 �ɂ��l�Ɣ�r
		// ID 24�͒��S���q��\���B�������qID���ߖT�T����������ێ������Ƃ�������ɒ���
		static constexpr double density_koshizuka2014 = 6.539696962;
		ASSERT_NEAR(particles[24].N(), density_koshizuka2014, 1e-5);
	}
}
}
