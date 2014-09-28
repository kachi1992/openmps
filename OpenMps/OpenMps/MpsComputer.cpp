#include "MpsComputer.hpp"
#include <algorithm>
#include <cmath>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef USE_VIENNACL
#include <viennacl/linalg/prod.hpp>
#include <viennacl/linalg/inner_prod.hpp>
#include <viennacl/linalg/norm_inf.hpp>
#endif

namespace OpenMps
{
	MpsComputer::MpsComputer(
#ifndef PRESSURE_EXPLICIT
		const double allowableResidual,
#endif	
		const MpsEnvironment& env,
		const Particle::List& particles)
		: environment(env),
		particles(particles),
		grid(env.R_e)
	{
#ifndef PRESSURE_EXPLICIT
		// 圧力方程式の許容誤差を設定
		ppe.allowableResidual = allowableResidual;
#endif
	}

	void MpsComputer::ForwardTime()
	{
		// グリッドに登録
		StoreGrid();

		// 時間刻みを設定
		DetermineDt();

		// 粒子数密度を計算する
		ComputeNeighborDensities();

		// 第一段階の計算
		ComputeExplicitForces();
		
#ifdef MODIFY_TOO_NEAR
		// 過剰接近粒子の補正
		ModifyTooNear();
#endif

		// グリッドに登録
		StoreGrid();

		// 粒子数密度を計算する
		ComputeNeighborDensities();

		// 第二段階の計算
		ComputeImplicitForces();

		// 時間を進める
		environment.SetNextT();
	}

	void MpsComputer::StoreGrid()
	{
		// TODO: 毎回全消去しない高速化しよう
		grid.Clear();

		// 全粒子について
		for(int i = 0; i < (int)particles.size(); i++)
		{
			// グリッドに登録
			grid.AddParticle(particles[i].VectorX(), i);
		}
	}

	void MpsComputer::DetermineDt()
	{
		namespace ublas = boost::numeric::ublas;

		// 最大速度を取得
		auto maxUParticle = *std::max_element(particles.cbegin(), particles.cend(),
			[](const Particle& base, const Particle& target)
			{
				auto baseU = base.VectorU();
				auto targetU = target.VectorU();
				return ( ublas::inner_prod(baseU, baseU) < ublas::inner_prod(targetU, targetU));
			});
		auto maxU = ublas::norm_2(maxUParticle.VectorU());

		// 時間刻みを設定
		environment.SetDt(maxU);
	}

	void MpsComputer::ComputeNeighborDensities()
	{
		const double r_e = environment.R_e;

#ifdef _OPENMP
		#pragma omp parallel for
#endif
		for(int i = 0; i < (int)particles.size(); i++)
		{
			// 粒子数密度を計算する
			particles[i].UpdateNeighborDensity(particles, grid, r_e);
		}
	}

	void MpsComputer::ComputeExplicitForces()
	{
		const auto n0 = environment.N0();
		const auto r_e = environment.R_e;
		const auto lambda = environment.Lambda();
		const auto nu = environment.Nu;
		const auto dt = environment.Dt();
		const auto g = environment.G;

		// 加速度を全初期化
		auto& a = du;
		a.clear();
		a.resize(particles.size());

#ifdef _OPENMP
		#pragma omp parallel
#endif
		{
#ifdef _OPENMP
			#pragma omp for
#endif
			// 全粒子で
			for(int i = 0; i < (int)particles.size(); i++)
			{
				// 粘性項の計算
				auto vis = particles[i].GetViscosity(particles, grid, n0, r_e, lambda, nu, dt);

				// 重力＋粘性項
				a[i] = g + vis;
			}

			// 全粒子で
#ifdef _OPENMP
			#pragma omp for
#endif
			for(int i = 0; i < (int)particles.size(); i++)
			{
				// 位置・速度を修正
				Vector thisA = a[i];
				particles[i].Move(particles[i].VectorU() * dt + a[i]*dt*dt/2);
				particles[i].Accelerate(a[i] * dt);
			}
		}
	}

	void MpsComputer::ComputeImplicitForces()
	{
#ifdef PRESSURE_EXPLICIT
		// 得た圧力を計算する
#ifdef _OPENMP
		#pragma omp parallel for
#endif
		for(int i = 0; i < (int)particles.size(); i++)
		{
			const auto c = environment.C;
			const auto n0 = environment.N0();
			const auto rho = environment.Rho;

			particles[i].UpdatePressure(c, rho, n0);
		}
#else
		// 圧力方程式を設定
		SetPressurePoissonEquation();

		// 圧力方程式を解く
		SolvePressurePoissonEquation();

		// 得た圧力を代入する
		for(unsigned int i = 0; i < particles.size(); i++)
		{
			const auto n0 = environment.N0();
			const auto surfaceRatio = environment.SurfaceRatio;

			particles[i].P(ppe.x(i), n0, surfaceRatio);
		}
#endif

		// 圧力勾配項を計算する
		ModifyByPressureGradient();
	}

#ifdef MODIFY_TOO_NEAR
	void MpsComputer::ModifyTooNear()
	{
		// 速度修正量を全初期化
		du.clear();
		du.resize(particles.size());

#ifdef _OPENMP
		#pragma omp parallel
#endif
		{

#ifdef _OPENMP
			#pragma omp for
#endif
			// 全粒子で
			for(int i = 0; i < (int)particles.size(); i++)
			{
				const auto r_e = environment.R_e;
				const auto rho = environment.Rho;
				const auto tooNearLength = environment.TooNearLength;
				const auto tooNearCoefficient = environment.TooNearCoefficient;

				// 過剰接近粒子からの速度修正量を計算する
				Vector d = particles[i].GetCorrectionByTooNear(particles, grid, r_e, rho, tooNearLength, tooNearCoefficient);
				du[i] = d;
			}

#ifdef _OPENMP
			#pragma omp for
#endif
			// 全粒子で
			for(int i = 0; i < (int)particles.size(); i++)
			{
				const auto dt = environment.Dt();

				// 位置・速度を修正
				Vector thisDu = du[i];
				particles[i].Accelerate(thisDu);
				particles[i].Move(thisDu * dt);
			}
		}
	}
#endif

#ifndef PRESSURE_EXPLICIT
	void MpsComputer::SetPressurePoissonEquation()
	{
		// 粒子数を取得
		int n = (int)particles.size();

		// 粒子に増減があれば
		if(n != (int)ppe.b.size())
		{
			// サイズを変えて作り直し
			ppe.A = Matrix(n, n);
			ppe.x = LongVector(n);
			ppe.b = LongVector(n);
			ppe.cg.r = LongVector(n);
			ppe.cg.p = LongVector(n);
			ppe.cg.Ap = LongVector(n);	
#ifdef USE_VIENNACL
			ppe.tempA = TempMatrix(n, n);
#endif
#ifdef _OPENMP
			ppe.a_ij.resize(n);
#endif
		}

		// 係数行列初期化
#ifdef USE_VIENNACL
		auto& A = ppe.tempA;
#else
		auto& A = ppe.A;
#endif
		A.clear();

#ifdef _OPENMP
		#pragma omp parallel
#endif
		{
#ifdef _OPENMP
			#pragma omp for
#endif
			// 全粒子で
			for(int i = 0; i < n; i++)
			{
				const auto n0 = environment.N0();
				const auto dt = environment.Dt();
				const auto surfaceRatio = environment.SurfaceRatio;

				// 生成項を計算する
				double b_i = particles[i].GetPpeSource(n0, dt, surfaceRatio);
				ppe.b(i) = b_i;

				// 圧力を未知数ベクトルの初期値にする
				double x_i = particles[i].P();
				ppe.x(i) = x_i;
			}
			// TODO: 以下もそうだけど、圧力方程式を作る際にインデックス指定のfor回さなきゃいけないのが気持ち悪いので、どうにかしたい

			
#ifdef _OPENMP
			#pragma omp for
#endif
			// 全粒子で
			for(int i = 0; i < n; i++)
			{
				// 対角項を初期化
				double a_ii = 0;
#ifdef _OPENMP
				ppe.a_ij[i].clear();
#endif
			
				// 全近傍ブロックで
				for(auto block = grid.cbegin(particles[i].VectorX()); block != grid.cend(particles[i].VectorX()); block++)
				{
					// 近傍ブロック内の粒子を取得
					auto neighborBlock = *block;
					auto neighbors = grid[neighborBlock];

					// 近傍ブロック内の粒子に対して
					for(auto j : neighbors)
					{
						const auto n0 = environment.N0();
						const auto r_e = environment.R_e;
						const auto rho = environment.Rho;
						const auto lambda = environment.Lambda();
						const auto surfaceRatio = environment.SurfaceRatio;
						// 自分以外
						if(i != j)
						{
							// 非対角項を計算
							double a_ij = particles[i].GetPpeMatrix(particles[j], n0, r_e, lambda, rho, surfaceRatio);
							if(a_ij != 0)
							{
#ifdef _OPENMP
								// 非対角項を格納
								ppe.a_ij[i].push_back(Ppe::A_ij(j, a_ij));
#else
								A(i, j) = a_ij;
#endif

								// 対角項も設定
								a_ii -= a_ij;
							}
						}
					}
				}
#ifdef _OPENMP
				// 対角項を格納
				ppe.a_ij[i].push_back(Ppe::A_ij(i, a_ii));
#else
				A(i, i) = a_ii;
#endif
			}
		}

#ifdef _OPENMP
		// 全行の
		for(int i = 0; i < n; i++)
		{
			// 全有効列で
			for(auto k : ppe.a_ij[i])
			{
				// 列番号と値を取得
				int j = k.first;
				double a_ij = k.second;

				// 行列に格納
				A(i, j) = a_ij;
			}
		}
#endif
		
#ifdef USE_VIENNACL
		// 作成した係数行列をデバイス側に複製
		viennacl::copy(ppe.tempA, ppe.A);
#endif
	}

	void MpsComputer::SolvePressurePoissonEquation()
	{
		// 共役勾配法で解く
		// TODO: 前処理ぐらい入れようよ

		auto& A = ppe.A;
		auto& x = ppe.x;
		auto& b = ppe.b;
		auto& r = ppe.cg.r;
		auto& p = ppe.cg.p;
		auto& Ap = ppe.cg.Ap;

		// 使用する演算を選択
#ifdef USE_VIENNACL
		namespace blas = viennacl::linalg;
#else
		namespace blas = boost::numeric::ublas;
#endif

		// 初期値を設定
		//  (Ap)_0 = A * x
		//  r_0 = b - Ap
		//  p_0 = r_0
		//  rr = r・r
		Ap = blas::prod(A, x);
		r = b - Ap;
		p = r;
		double rr = blas::inner_prod(r, r);

		// 初期値でまだ収束していない場合
		bool isConverged = (blas::norm_inf(r)< ppe.allowableResidual);
		if(!isConverged)
		{
			// 未知数分だけ繰り返す
			for(unsigned int i = 0; i < x.size(); i++)
			{
				// 計算を実行
				//  Ap = A * p
				//  α = rr/(p・Ap)
				//  x' += αp
				//  r' -= αAp
				Ap = blas::prod(A, p);
				double alpha = rr / blas::inner_prod(p, Ap);
				x += alpha * p;
				r -= alpha * Ap;

				// 収束判定（残差＝残差ベクトルの最大要素値）
				double residual = blas::norm_inf(r);
				isConverged = (residual < ppe.allowableResidual);

				// 収束していたら
				if(isConverged)
				{
					// 繰り返し終了
					break;
				}
				// なかったら
				else
				{
					// 残りの計算を実行
					//  r'r' = r'・r'
					//  β= r'r'/rr
					//  p = r' + βp
					//  rr = r'r'
					double rrNew = blas::inner_prod(r, r);
					double beta = rrNew / rr;
					p = r + beta * p;
					rr = rrNew;
				}
			}
		}

		// 理論上は未知数分だけ繰り返せば収束するはずだが、収束しなかった場合は
		if(!isConverged)
		{
			// どうしようもないので例外
			Exception exception;
			exception.Message = "Conjugate Gradient method couldn't solve Pressure Poison Equation";
			throw exception;
		}
	};
#endif

	void MpsComputer::ModifyByPressureGradient()
	{
		// 速度修正量を全初期化
		du.clear();
		du.resize(particles.size());

#ifdef _OPENMP
		#pragma omp parallel
#endif
		{
#ifdef _OPENMP
			#pragma omp for
#endif
			// 全粒子で
			for(int i = 0; i < (int)particles.size(); i++)
			{
				const double r_e = environment.R_e;
				const double dt = environment.Dt();
				const double rho = environment.Rho;
				const double n0 = environment.N0();

				// 圧力勾配を計算する
				Vector d = particles[i].GetPressureGradient(particles, grid, r_e, dt, rho, n0);
				du[i] = d;
			}
#ifdef _OPENMP
			#pragma omp for
#endif

			// 全粒子で
			for(int i = 0; i < (int)particles.size(); i++)
			{
				const double dt = environment.Dt();

				// 位置・速度を修正
				Vector thisDu = du[i];
				particles[i].Accelerate(thisDu);
				particles[i].Move(thisDu * dt);
			}
		}
	}
}
