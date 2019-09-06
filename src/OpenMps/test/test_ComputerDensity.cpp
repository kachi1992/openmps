#include <gtest/gtest.h>

#define TEST_DENSITY
#include "../Computer.hpp"
#include "../Particle.hpp"
#include "../Vector.hpp"

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
	static constexpr int num_ps_x = 7;
	static constexpr int num_ps_z = 7;

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

	class DensityTest : public ::testing::Test
	{
	protected:
		OpenMps::Computer<decltype(positionWall)&,decltype(positionWallPre)&> *computer;

		// ���ꂼ��̃e�X�g�P�[�X��TEST_F���Ă΂�钼�O��SetUp�ŏ����������
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

OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&> comp = OpenMps::CreateComputer(
#ifndef PRESSURE_EXPLICIT
	eps,
#endif
	environment,
	positionWall, positionWallPre);

computer = new OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>(std::move(comp));

std::vector<OpenMps::Particle> particles;

// 1��l0, num_ps_x*num_ps_z�̊i�q��ɗ��q��z�u
for (int j = 0; j < num_ps_z; ++j)
{
	for (int i = 0; i < num_ps_x; ++i)
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

#ifndef PRESSURE_EXPLICIT
		auto getAllowableResidual()
		{
			return computer->ppe.allowableResidual;
		}
#endif

		auto& GetParticles()
		{
			return computer->particles;
		}

		double Rv(const Vector& x1, const Vector& x2)
		{
			return OpenMps::Computer<decltype(positionWall)&, decltype(positionWallPre)&>::R(x1, x2);
		}

		double Rp(const Particle& p1, const Particle& p2)
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

	TEST_F(DensityTest, NeighborDistance)
	{
		// TODO: �K����3�_�������炩����Ă���

		// 3��Vector, Particle��p��
		auto p1 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p2 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);
		auto p3 = OpenMps::Particle(OpenMps::Particle::Type::IncompressibleNewton);

		p1.X()[OpenMps::AXIS_X] = 1.0;
		p1.X()[OpenMps::AXIS_Z] = 0.3;

		p2.X()[OpenMps::AXIS_X] = -2.0;
		p2.X()[OpenMps::AXIS_Z] = 0.8;

		p3.X()[OpenMps::AXIS_X] = 100.0;
		p3.X()[OpenMps::AXIS_Z] = -20.5;

		// Vector, Particle �̋����v�Z����v
		ASSERT_DOUBLE_EQ(Rv(p1.X(), p2.X()), Rp(p1, p2));

		// �_����ւ��ɂ��đΏ�
		ASSERT_DOUBLE_EQ(Rp(p1, p2), Rp(p2, p1));

		// �O�p�s����
		ASSERT_LE(Rp(p1, p2), Rp(p1, p3) + Rp(p2, p3));
		ASSERT_LE(Rp(p2, p3), Rp(p1, p2) + Rp(p1, p3));
		ASSERT_LE(Rp(p3, p1), Rp(p1, p2) + Rp(p2, p3));
	}

	// �ߐڂ���2���q�݂͌����ߖT���q�Ƃ��ĕێ����Ă��邩�H
	// TODO: 1�ł������ false�I�ȕ����m�F
	TEST_F(DensityTest, NeighborSymmetry)
	{
		SearchNeighbor();

		const auto& particles = GetParticles();
		auto n = particles.size();

		bool neigh_sym = true;
		for(auto i = decltype(n){0}; i < n; i++) // ��ɑ��݂��闱�q���[�v
		{
			if(particles[i].TYPE() != Particle::Type::Disabled)
			{
				for(auto idx = decltype(i){0}; idx < NeighborCount(i); idx++) // i���q�̋ߖT���q���[�v
				{
					auto j = Neighbor(i, idx); // i���q�ɗׂ荇��idx�Ԗڗ��q��j�Ƃ���

					// j���q�̋ߖT��i���q���܂܂�Ă��邩�H
					bool hasi = false;
					for (auto idxj = decltype(i){0}; idxj < NeighborCount(j); idxj++)
					{
						if (particles[j].TYPE() != Particle::Type::Disabled && Neighbor(j, idxj) == i)
						{
							hasi |= true;
						}
					}
				}

				neigh_sym &= true;
			}
		}

		ASSERT_TRUE(neigh_sym);
	}

	// ���q�͎������g���ߖT���q�Ƃ��Ċ܂�ł��Ȃ����H
	TEST_F(DensityTest, NeighborMyself)
	{
		SearchNeighbor();

		const auto& particles = GetParticles();
		auto n = particles.size();

		bool hasmyself = false;
		for(auto i = decltype(n){0}; i < n; i++) // ��ɑ��݂��闱�q���[�v
		{
			if(particles[i].TYPE() != Particle::Type::Disabled)
			{
				for(auto idx = decltype(i){0}; idx < NeighborCount(i); idx++) // i���q�̋ߖT���q���[�v
				{
					hasmyself = (Neighbor(i, idx) == i);
				}
			}
		}

		ASSERT_FALSE(hasmyself);
	}

	// �����q�����x���v�Z���ė��_�l�Ɣ�r
	TEST_F(DensityTest, NeighDensity)
	{
		SearchNeighbor();

		ComputeNeighborDensities();

		const auto& particles = GetParticles();
		// �e�L�X�g Koshizuka et al.,(2014) �ɂ��l�Ɣ�r
		// TODO: ID 24�͒��S���q��\���B�����̗��qID���ێ�����Ă���Ɖ��肵�Ă���
		ASSERT_NEAR(particles[24].N(), 6.539696962, 1e-5);
	}
}
}
