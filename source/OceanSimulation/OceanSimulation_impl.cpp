#include "OceanSimulation_decl.h"


#include "maxon/matrix_nxm.h"
#include "maxon/fft.h"
#include "maxon/parallelfor.h"
#include "maxon/atomictypes.h"

// for test atm
#include "maxon/jobgroup.h"

#include "maxon/parallelimage.h"
#include "c4d_thread.h"


namespace OceanSimulation
{




	// this is the start of implementing the job title parallelize cool super fast for hot4D
	class DisplacementJob : public maxon::JobInterfaceTemplate<DisplacementJob, maxon::Bool>
	{

		maxon::Result<void> operator()()
		{
			return SetResult(false);
		}
	};





	const maxon::Float gravity = 9.81;

	//----------------------------------------------------------------------------------------
	/// return a random with gaussian distribution
	//----------------------------------------------------------------------------------------
	static maxon::Float RandomGaussian(maxon::LinearCongruentialRandom<maxon::Float32>& random)
	{

		maxon::Float x, y, lengthsqr;
		do {
			x = random.Get11();
			y = random.Get11();
			lengthsqr = x * x + y * y;


		} while (lengthsqr >= 1.0 || lengthsqr == 0.0);

		return x * maxon::Sqrt(-2.0_f * maxon::Log2(lengthsqr) / lengthsqr); // ILM
		//return y * maxon::Sqrt(-2.0_f * maxon::Ln(lengthsqr) / lengthsqr) * 0.18_f; // maxon random G11


		
	}

	//----------------------------------------------------------------------------------------
	/// calculate a linear interpolation beetween two values
	/// @param[in]  a  : the first value 
	/// @param[in]  b  : the second value 
	/// @param[in]  f  : the weight of the interpolation 
	/// @return the interpolated value
	//----------------------------------------------------------------------------------------
	template<typename T>
	static inline T const LinearInterpolation(const T a, const T b, const T f)
	{
		return a + (b - a)*f;
	}




	//----------------------------------------------------------------------------------------
	/// calculate a catmull Rom interpolation
	/// @param[in]  p0  : the first value 
	/// @param[in]  p1  : the second value
	/// @param[in]  p2  : the third value
	/// @param[in]  p3  : the fourth value
	/// @param[in]  f  : the  balance value
	/// @return the interpolated value
	//----------------------------------------------------------------------------------------
	template<typename T>
	static inline T CatmullRomInterpolation(T p0, T p1, T p2, T p3, T f)
	{
		return 0.5 *((2 * p1) + (-p0 + p2) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f*f + (-p0 + 3 * p1 - 3 * p2 + p3) * f*f*f);
	}




	class OceanImplementation : public maxon::Component<OceanImplementation, OceanInterface>
	{
		MAXON_COMPONENT();
	public:

		MAXON_METHOD maxon::Result<void> Init(maxon::Int32 oceanResolution, maxon::Float oceanSize, maxon::Float shortestWaveLength,
			maxon::Float amplitude, maxon::Float windSpeed, maxon::Float windDirection, maxon::Float alignement, maxon::Float damp,
			maxon::Int32 seed)

		{
			iferr_scope;



			M_ = N_ = oceanResolution;
			Lx_ = Lz_ = oceanSize;
			l_ = shortestWaveLength;
			A_ = amplitude;
			V_ = windSpeed;
			alignement_ = alignement;
			dampReflect_ = damp;
			seed_ = seed;


			kv_.Resize(M_, N_) iferr_return;
			km_.Resize(M_, N_) iferr_return;
			h0_.Resize(M_, N_) iferr_return;
			h0_minus_.Resize(M_, N_) iferr_return;
			htilda_.Resize(M_, N_) iferr_return;
			fftin_.Resize(M_, N_) iferr_return;
			foam_.Resize(M_, N_) iferr_return;
			jMinus_.Resize(M_, N_) iferr_return;
			
			
			maxon::LinearCongruentialRandom<maxon::Float32> randNumber; 
			randNumber.Init(seed);


			WHat_ = GetWindDirectionNormalized(windDirection);


			for (maxon::Int32 i = 0, m_ = -M_ / 2; i < M_; i++, m_++)
			{
				for (maxon::Int32 j = 0, n_ = -N_ / 2; j < N_; j++, n_++)
				{
					maxon::Vector2d k;
					k = maxon::Vector2d(maxon::PI2 * m_ / Lx_, maxon::PI2 * n_ / Lz_);
					kv_(i, j) = k;

					km_(i, j) = k.GetLength();

					maxon::Complex<maxon::Float> rc(RandomGaussian(randNumber), RandomGaussian(randNumber));

					h0_(i, j) = rc * (maxon::Sqrt(Phillips(k)) * maxon::SQRT2_INV);
					h0_minus_(i, j) = rc * (maxon::Sqrt(Phillips(-k)) * maxon::SQRT2_INV);  // no /2.0 but * by 1 / maxon::Sqrt(2.0) 
				}

			}


			scaleNorm_ = CalculateNormalizedScale();


			return maxon::OK;
		};

		MAXON_METHOD maxon::Bool NeedUpdate(const maxon::Int32 oceanResolution, const maxon::Float oceanSize, const maxon::Float shortestWaveLength,
			const maxon::Float amplitude, const maxon::Float windSpeed, const maxon::Float windDirection, const maxon::Float alignement, const maxon::Float damp,
			const maxon::Int32 seed) const 
		{

			if (M_ != oceanResolution || N_ != oceanResolution)
				return true;
			if (Lx_ != oceanSize || Lz_ != oceanSize)
				return true;
			if (l_ != shortestWaveLength)
				return true;
			if (A_ != amplitude)
				return true;
			if (V_ != windSpeed)
				return true;
			if (WHat_ != GetWindDirectionNormalized(windDirection))
				return true;
			if (alignement_ != alignement)
				return true;
			if (dampReflect_ != damp)
				return true;
			if (seed_ != seed)
				return true;

			return false;

		};

		MAXON_METHOD maxon::Result<void> Animate(maxon::Float currentTime, maxon::Int32 loopPeriod, maxon::Float timeScale, maxon::Float oceanDepth, maxon::Float chopAmount, maxon::Bool doDisp, maxon::Bool doChop, maxon::Bool doJacob, maxon::Bool doNormals)
		{
			iferr_scope_handler
			{
				
				return err;
				
			};


			// for now ParallelFor is faster for a resolution at 128, after that, ParallelImage is faster.
			

			// for other functions like eval_UV() or Omega()
			Omega0_ = maxon::PI2 / (loopPeriod*timeScale);
			oceanDepth_ = oceanDepth;
			doDisp_ = doDisp;
			doNormals_ = doNormals;
			doChop_ = doChop;
			doJacob_ = doJacob;

			

			// should be update with title loop or Job Group.
			// maxon::JobGroupRef group = maxon::JobGroupRef::Create() iferr_return;


			
			
			

			/*auto prepareDisp = [&currentTime, &timeScale, this](maxon::Int32 l)
			{
				maxon::Int32 i = l / M_;
				maxon::Int32 j = maxon::Mod(l, M_);
				maxon::Float omega = Omega(km_(i, j));
				maxon::Complex<maxon::Float> complexOmega;
				maxon::Complex<maxon::Float> complexMinusOmega;
				complexOmega.SetExp(omega * currentTime * timeScale);
				complexMinusOmega.SetExp(-omega * currentTime * timeScale);

				htilda_(i, j) = h0_(i, j)						* complexOmega +
					h0_minus_(i, j).GetConjugate()	* complexMinusOmega;

				fftin_(i, j) = htilda_(i, j)  * scaleNorm_;
			};
			maxon::ParallelFor::Dynamic(0, M_ * N_, prepareDisp);*/
			
			
			
			maxon::Int titleSize = M_ / GeGetCurrentThreadCount();

			auto prepareDisp = [&currentTime, &timeScale, this](maxon::Int i, maxon::Int j)
			{
				maxon::Float omega = Omega(km_(i, j));
				maxon::Complex<maxon::Float> complexOmega;
				maxon::Complex<maxon::Float> complexMinusOmega;
				complexOmega.SetExp(omega * currentTime * timeScale);
				complexMinusOmega.SetExp(-omega * currentTime * timeScale);

				htilda_(i, j) = h0_(i, j)						* complexOmega +
					h0_minus_(i, j).GetConjugate()	* complexMinusOmega;

				fftin_(i, j) = htilda_(i, j)  * scaleNorm_;
			};
			maxon::ParallelImage::Process(M_, N_, titleSize, prepareDisp);


			// prepare the fft Transform reference.
			const maxon::FFTRef KissFFT = maxon::FFTClasses::Kiss().Create() iferr_return;
			const maxon::FFT_SUPPORT options = KissFFT.GetSupportOptions();
			if (!(options & maxon::FFT_SUPPORT::TRANSFORM_2D))
				return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

			const maxon::Float sign[2] = { 1.0 , -1.0 };


			if (doDisp_)
			{
				KissFFT.Transform2D(fftin_, dispY_, fft_flags_) iferr_return;

				auto inverSign = [&sign, this](maxon::Int32 l)
				{
					maxon::Int32 i = l / M_;
					maxon::Int32 j = maxon::Mod(l, M_);
					dispY_(i, j) *= sign[(i + j) & 1];
				};

				maxon::ParallelFor::Dynamic(0, M_ * N_, inverSign);

			}

			if (doChop_)
			{
				//auto prepareChopX = [this, &chopAmount](maxon::Int32 l)
				//{
				//	maxon::Int32 i = l / M_;
				//	maxon::Int32 j = maxon::Mod(l, M_);
				//	fftin_(i, j) = chopAmount * -scaleNorm_ * maxon::Complex<maxon::Float>(0, -1) * htilda_(i, j) *
				//		(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).x / km_(i, j));
				//};

				//maxon::ParallelFor::Dynamic(0, M_ * N_, prepareChopX);

				
				auto prepareChopX = [this, &chopAmount](maxon::Int i, maxon::Int j)
				{
					fftin_(i, j) = chopAmount * -scaleNorm_ * maxon::Complex<maxon::Float>(0, -1) * htilda_(i, j) *
						(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).x / km_(i, j));
				};
				maxon::ParallelImage::Process(M_, N_, titleSize, prepareChopX);
				

				KissFFT.Transform2D(fftin_, dispX_, fft_flags_) iferr_return;

				//auto prepareChopZ = [this, &chopAmount](maxon::Int32 l)
				//{
				//	maxon::Int32 i = l / M_;
				//	maxon::Int32 j = maxon::Mod(l, M_);
				//	fftin_(i, j) = chopAmount * -scaleNorm_ * maxon::Complex<maxon::Float>(0, -1) * htilda_(i, j) *
				//		(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).y / km_(i, j));
				//};
				//maxon::ParallelFor::Dynamic(0, M_ * N_, prepareChopZ);
				auto prepareChopZ = [this, &chopAmount](maxon::Int i, maxon::Int j)
				{

					fftin_(i, j) = chopAmount * -scaleNorm_ * maxon::Complex<maxon::Float>(0, -1) * htilda_(i, j) *
						(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).y / km_(i, j));
				};
				maxon::ParallelImage::Process(M_, N_, titleSize, prepareChopZ);
				KissFFT.Transform2D(fftin_, dispZ_, fft_flags_) iferr_return;


				auto inverSign = [&sign, this](maxon::Int32 l)
				{
					maxon::Int32 i = l / M_;
					maxon::Int32 j = maxon::Mod(l, M_);
					dispX_(i, j) *= sign[(i + j) & 1];
					dispZ_(i, j) *= sign[(i + j) & 1];
				};

				maxon::ParallelFor::Dynamic(0, M_ * N_, inverSign);


			} // end do chop

			if (doJacob_)
			{
				// jxx
				//auto prepareJXX = [this, &chopAmount](maxon::Int32 l)
				//{
				//	maxon::Int32 i = l / M_;
				//	maxon::Int32 j = maxon::Mod(l, M_);
				//	fftin_(i, j) = chopAmount * -scaleNorm_ * htilda_(i, j) *
				//		(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).x * kv_(i, j).x / km_(i, j));
				//};
				//maxon::ParallelFor::Dynamic(0, M_ * N_, prepareJXX);
				auto prepareJXX = [this, &chopAmount](maxon::Int i, maxon::Int j)
				{

					fftin_(i, j) = chopAmount * -scaleNorm_ * htilda_(i, j) *
						(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).x * kv_(i, j).x / km_(i, j));
				};
				maxon::ParallelImage::Process(M_, N_, titleSize, prepareJXX);
				


				KissFFT.Transform2D(fftin_, jxx_, fft_flags_) iferr_return;

				// jzz
				//auto prepareJZZ = [this, &chopAmount](maxon::Int32 l)
				//{
				//	maxon::Int32 i = l / M_;
				//	maxon::Int32 j = maxon::Mod(l, M_);
				//	fftin_(i, j) = chopAmount * -scaleNorm_ * htilda_(i, j) *
				//		(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).y * kv_(i, j).y / km_(i, j));
				//};
				//maxon::ParallelFor::Dynamic(0, M_ * N_, prepareJZZ);
				auto prepareJZZ = [this, &chopAmount](maxon::Int i, maxon::Int j)
				{
					fftin_(i, j) = chopAmount * -scaleNorm_ * htilda_(i, j) *
						(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).y * kv_(i, j).y / km_(i, j));
				};
				maxon::ParallelImage::Process(M_, N_, titleSize, prepareJZZ);
				KissFFT.Transform2D(fftin_, jzz_, fft_flags_) iferr_return;


				// jxz
				//auto prepareJXZ = [this, &chopAmount](maxon::Int32 l)
				//{
				//	maxon::Int32 i = l / M_;
				//	maxon::Int32 j = maxon::Mod(l, M_);
				//	fftin_(i, j) = chopAmount * -scaleNorm_ * htilda_(i, j) *
				//		(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).x * kv_(i, j).y / km_(i, j));
				//};
				//maxon::ParallelFor::Dynamic(0, M_ * N_, prepareJXZ);
				auto prepareJXZ = [this, &chopAmount](maxon::Int i, maxon::Int j)
				{
					fftin_(i, j) = chopAmount * -scaleNorm_ * htilda_(i, j) *
						(km_(i, j) == 0.0 ? maxon::Complex<maxon::Float>(0, 0) : kv_(i, j).x * kv_(i, j).y / km_(i, j));
				};
				maxon::ParallelImage::Process(M_, N_, titleSize, prepareJXZ);
				KissFFT.Transform2D(fftin_, jxz_, fft_flags_) iferr_return;

				auto inverSign = [&sign, this](maxon::Int32 l)
				{
					maxon::Int32 i = l / M_;
					maxon::Int32 j = maxon::Mod(l, M_);
					jxx_(i, j) *= sign[(i + j) & 1];// +1.0; bugg
					jzz_(i, j) *= sign[(i + j) & 1];// +1.0;
					jxz_(i, j) *= sign[(i + j) & 1];
				};
				maxon::ParallelFor::Dynamic(0, M_ * N_, inverSign);

				// calcultate jminus for each point
				auto calculateJminus = [this](maxon::Int32 l)
				{
					maxon::Int32 i = l / M_;
					maxon::Int32 j = maxon::Mod(l, M_);
					
					maxon::Float a, b;
					// maxon::Float qplus, qminus;
					a = jxx_(i, j) + jzz_(i, j);
					b = maxon::Sqrt((jxx_(i, j) - jzz_(i, j))*(jxx_(i, j) - jzz_(i, j)) + 4.0 * jxz_(i, j) * jxz_(i, j));  // (equation 32)

					jMinus_(i, j) = 0.5*(a - b);
				};
				maxon::ParallelFor::Dynamic(0, M_ * N_, calculateJminus);
				//auto calculateJminus = [this](maxon::Int i, maxon::Int j)
				//{
				//	maxon::Float a, b;
				//	// maxon::Float qplus, qminus;
				//	a = jxx_(i, j) + jzz_(i, j);
				//	b = maxon::Sqrt((jxx_(i, j) - jzz_(i, j))*(jxx_(i, j) - jzz_(i, j)) + 4.0 * jxz_(i, j) * jxz_(i, j));  // (equation 32)

				//	jMinus_(i, j) = 0.5*(a - b);
				//};
				//maxon::ParallelImage::Process(M_, N_, titleSize, calculateJminus);

			} // end do jacob


			lastFrameCompute_ = currentTime;

			return maxon::OK;
		};

		MAXON_METHOD maxon::Result<void> EvaluatePoint(const INTERTYPE type, const maxon::Vector p, maxon::Vector &displacement, maxon::Vector &normal, maxon::Float &jMinus) const
		{
			if (Lx_ == 0.0 || Lz_ == 0.0)
				return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);
			return	self.EvaluateUV(type, maxon::Vector2d(p.x / Lx_, p.z / Lz_), displacement, normal, jMinus);
		};
		
		MAXON_METHOD maxon::Result<void> EvaluateUV(const INTERTYPE type, maxon::Vector2d uv, maxon::Vector &displacement, maxon::Vector &normal, maxon::Float &jMinus) const
		{
			iferr_scope;

			maxon::Int32 i0, i1, j0, j1;
			maxon::Int32 ia, i2, ja, j2;
			maxon::Float frac_x, frac_z;

			// first wrap the texture so 0 <= (u,v) < 1
			uv.x = maxon::FMod(uv.x, 1.0);
			uv.y = maxon::FMod(uv.y, 1.0);

			if (uv.x < 0)
				uv.x += 1.0;
			if (uv.y < 0)
				uv.y += 1.0;

			maxon::Float uu = uv.x * M_;
			maxon::Float vv = uv.y * N_;


			i0 = maxon::SafeConvert<maxon::Int32>(maxon::Floor(uu)); // SAFEINT32(maxon::Floor(uu));
			j0 = maxon::SafeConvert<maxon::Int32>(maxon::Floor(vv)); // SAFEINT32(maxon::Floor(vv));

			i1 = i0 + 1;
			j1 = j0 + 1;

			frac_x = uu - i0;
			frac_z = vv - j0;

			i0 = maxon::Mod(i0, M_);
			j0 = maxon::Mod(j0, N_);

			i1 = maxon::Mod(i1, M_);
			j1 = maxon::Mod(j1, N_);

			maxon::BaseArray<maxon::Int32> points;
			points.EnsureCapacity(8) iferr_ignore("we will use append anyway");
			points.Append(i0) iferr_return;
			points.Append(i1) iferr_return;
			points.Append(j0) iferr_return;
			points.Append(j1) iferr_return;



			if (type == INTERTYPE::CATMULLROM)
			{


				ia = i0 - 1;
				i2 = i1 + 1;
				ia = ia < 0 ? ia + M_ : ia;
				i2 = i2 >= M_ ? i2 - M_ : i2;

				ja = j0 - 1;
				j2 = j1 + 1;
				ja = ja < 0 ? ja + N_ : ja;
				j2 = j2 >= N_ ? j2 - N_ : j2;

				points.Flush();
				points.Append(ia) iferr_return;
				points.Append(i0) iferr_return;
				points.Append(i1) iferr_return;
				points.Append(i2) iferr_return;
				points.Append(ja) iferr_return;
				points.Append(j0) iferr_return;
				points.Append(j1) iferr_return;
				points.Append(j2) iferr_return;

			}

			if (doDisp_)
			{
				displacement.y = Interpolation(type, dispY_, points, frac_x, frac_z);  //BILERP(dispY_);
			}
			if (doNormals_)
			{
				normal[0] = Interpolation(type, normX_, points, frac_x, frac_z);
				normal[1] = Interpolation(type, normY_, points, frac_x, frac_z);
				normal[2] = Interpolation(type, normZ_, points, frac_x, frac_z);
			}
			if (doChop_)
			{
				displacement.x = Interpolation(type, dispX_, points, frac_x, frac_z); ;
				displacement.z = Interpolation(type, dispZ_, points, frac_x, frac_z); ;
			}
			else
			{
				displacement.x = 0.0;
				displacement.z = 0.0;
			}
			if (doJacob_)
			{
				//ComputeEigen(Interpolation(type, jxx_, points, frac_x, frac_z), Interpolation(type, jzz_, points, frac_x, frac_z), Interpolation(type, jxz_, points, frac_x, frac_z), jMinus);
				jMinus = Interpolation(type, jMinus_, points, frac_x, frac_z);
			}


			return maxon::OK;
		};



	private: 
		
		maxon::Int32									LT_;			///< phase of the simulation (looping time)
		maxon::Float									Omega0_;		///< omega 0 (w0)  note (17) 2PI/T
		maxon::Int32									M_;				///< the resolution of ocean in the X direction
		maxon::Int32									N_;				///< the resolution of ocean in the Z direction
		maxon::Float									Lx_;			///< the ocean size on unit in the x direction
		maxon::Float									Lz_;			///< the ocean size on unit in the z direction 
		maxon::Float									l_;				///< the shortest wave length
		maxon::Float									A_;				///< Amplitude, desired wave height
		maxon::Float									V_;				///< Wind Speed  in m/s
		maxon::Vector2d									WHat_;			///< wind direction
		maxon::Float									CA_;			///< Chop amount for the chopiness
		maxon::Float									oceanDepth_;	///< ocean's depth
		maxon::Int32									seed_;			///< seed for the generator

		maxon::MatrixNxM<maxon::Vector2d>				kv_;			///< store the vector k
		maxon::MatrixNxM<maxon::Float>					km_;			///< store the magnitude of vector k


		maxon::Float									alignement_;	///< alignement of the wave in the wind direciton 
		maxon::Float									dampReflect_;	///< weight of damped waves
		maxon::Float									scaleNorm_;		///< normalized scaled.


		maxon::Bool										doDisp_;		///< have to calculate the displacement
		maxon::Bool										doChop_;		///< have to calculate the chopiness
		maxon::Bool										doNormals_;		///< have to calculate normals
		maxon::Bool										doJacob_;		///< have to calculate jacobian



		maxon::MatrixNxM<maxon::Complex<maxon::Float>> h0_;			///< h0  (used in 26)
		maxon::MatrixNxM<maxon::Complex<maxon::Float>> h0_minus_;		///< h0 minus (used in 26)
		maxon::MatrixNxM<maxon::Complex<maxon::Float>> htilda_;		///< htilda (used in 26)
		maxon::MatrixNxM<maxon::Complex<maxon::Float>> fftin_;			///< the array that will feed the FFT



		maxon::MatrixNxM<maxon::Float>					dispX_; ///< the displacement result in x direction (chopiness)
		maxon::MatrixNxM<maxon::Float>					dispY_; ///< the displacement result in y direciton (height)
		maxon::MatrixNxM<maxon::Float>					dispZ_;	///< the displacement result in z direction (chopiness)

		maxon::MatrixNxM<maxon::Float>					normX_; ///< the normal x vector
		maxon::MatrixNxM<maxon::Float>					normY_; ///< the normal y vector
		maxon::MatrixNxM<maxon::Float>					normZ_; ///< the normal z vector

		maxon::MatrixNxM<maxon::Float>					jxx_; ///< the recult of jacobian x direction
		maxon::MatrixNxM<maxon::Float>					jzz_; ///< the recult of jacobian y direction
		maxon::MatrixNxM<maxon::Float>					jxz_; ///< the recult of jacobian z direction

		maxon::MatrixNxM<maxon::Float>					foam_; ///< the foam for actual frame
		maxon::MatrixNxM<maxon::Float>					jMinus_; ///< store the jminus of the frame
		maxon::Float									lastFrameCompute_; ///< time of the last frame computed.

		const  maxon::FFT_FLAGS fft_flags_ = maxon::FFT_FLAGS::CALC_INVERSE; ///< the FFT flags --- can be maxon::FFT_FLAGS::CALC_INVERSE | maxon::FFT_FLAGS::SUPPRESS_PADDING


		//----------------------------------------------------------------------------------------
		/// calculate the Dispersion Relation  (18)
		/// @param[in]  k  : the vector to calculate the omega 
		/// return		the calculated Omega
		//----------------------------------------------------------------------------------------
		maxon::Float				Omega(maxon::Float k)
		{
			// if depth >50 tanh(k * depth) become 1.0 so negligeable  
			// if wave are small -> 1 + maxon::Sqr(k) * maxon::Sqr(L) (L =  uniths of length)
			// calculate dispersion
			maxon::Float omegaK = maxon::Sqrt(gravity * k  * maxon::Tanh(k * oceanDepth_));
			// quantatize frequencies to repeat at timeloop
			return  maxon::Floor(omegaK / Omega0_) * Omega0_;
		};

		//----------------------------------------------------------------------------------------
		/// calculate the philip spectrum of the vector k 
		/// @param[in]  k  : the vector to calculate the philip spectrum
		/// return		the Phillips spectrum
		//----------------------------------------------------------------------------------------
		maxon::Float				Phillips(maxon::Vector2d k)
		{
			maxon::Float k2 = k.GetSquaredLength();
			maxon::Vector2d knorm = k.GetNormalized();
			if (k2 == 0.0)
				return 0.0;
			maxon::Float L = maxon::Sqr(V_) / gravity;
			maxon::Float k4 = maxon::Sqr(k2);
			maxon::Float dot_k_wHat = knorm.x*WHat_.x + knorm.y * WHat_.y;


			if (dot_k_wHat < 0.0) // remove damp waves
				dot_k_wHat *= maxon::Clamp01(1.0 - dampReflect_);


			return A_ * maxon::Exp(-1.0 / (k2 * maxon::Sqr(L)) - k2 * maxon::Sqr(l_)) / k4 * maxon::Pow(maxon::Abs(dot_k_wHat), alignement_);  // from houdini
			//return A_ * (maxon::Exp(-1.0 / (k2*maxon::Sqr(L)) ) / k4)  * (maxon::Exp(-k2 * maxon::Sqr(l_))) * maxon::Pow(maxon::Abs(dot_k_wHat),alignement_) ; // from paper


		};

		//----------------------------------------------------------------------------------------
		/// Evaluate the result of the simulation and return the vectors for displacemenet, normals and jacobian
		/// @param[in]  uv  : 2d vector that give the u and v coordinate
		/// @param[out]  displacement  : reference to store the displacement result 
		/// @param[out]  normal  : reference to store the normals result
		/// @param[out]  jacob : reference to store the jacobian result
		/// return maxon::OK on success
		//----------------------------------------------------------------------------------------
		

		//----------------------------------------------------------------------------------------
		/// Interpolation function, change interpolation by type
		/// @param[in]  type  : type of interpolation to use 
		/// @param[in]  m  : the matrix of data 
		/// @param[in]  pointIndex  : an array that store the points to interpolate 
		/// @param[in]  f1  : the first scalar 
		/// @param[in]  f2  : the second scalar 
		/// return		the interpolated value
		//----------------------------------------------------------------------------------------
		maxon::Float				Interpolation(const INTERTYPE type, const maxon::MatrixNxM<maxon::Float> &m, const maxon::BaseArray<maxon::Int32> &pointIndex, const maxon::Float f1, const maxon::Float  f2) const
		{
			// MAXON_SWITCH_CHECKALLENUMS_BEGIN;
			switch (type)
			{
				case  INTERTYPE::LINEAR:
				{
					// do linear interpolation

					if (pointIndex.GetCount() < 4)
						return 1.0;

					maxon::Int32 i0, i1, j0, j1;

					i0 = pointIndex[0];
					i1 = pointIndex[1];
					j0 = pointIndex[2];
					j1 = pointIndex[3];


					return LinearInterpolation(
						LinearInterpolation(m(i0, j0), m(i1, j0), f1),
						LinearInterpolation(m(i0, j1), m(i1, j1), f1),
						f2
					);
				}

				case INTERTYPE::CATMULLROM:
				{
					if (pointIndex.GetCount() < 8)
						return 1.0;

					maxon::Int32 i0, i1, i2, i3, j0, j1, j2, j3;
					i0 = pointIndex[0];
					i1 = pointIndex[1];
					i2 = pointIndex[2];
					i3 = pointIndex[3];

					j0 = pointIndex[4];
					j1 = pointIndex[5];
					j2 = pointIndex[6];
					j3 = pointIndex[7];

					return CatmullRomInterpolation(
						CatmullRomInterpolation(m(i0, j0), m(i1, j0), m(i2, j0), m(i3, j0), f1),
						CatmullRomInterpolation(m(i0, j1), m(i1, j1), m(i2, j1), m(i3, j1), f1),
						CatmullRomInterpolation(m(i0, j2), m(i1, j2), m(i2, j2), m(i3, j2), f1),
						CatmullRomInterpolation(m(i0, j3), m(i1, j3), m(i2, j3), m(i3, j3), f1),
						f2
					);



				}

				default:
					return 0.0;
			}
			// MAXON_SWITCH_CHECKALLENUMS_END;
		};

		//----------------------------------------------------------------------------------------
		/// calculate the scale of the wave based on the parameter already given in the constructor
		/// @return the 1/max value on the simulation
		//----------------------------------------------------------------------------------------
		maxon::Float				CalculateNormalizedScale()
		{
			maxon::Float res = 1.0;

			iferr_scope_handler
			{
				DiagnosticOutput("Error: @", err);
				return res;
			};

			maxon::Complex<maxon::Float> complexOmega(1, 0);		// this is calculate at time 0 so 
			maxon::Complex<maxon::Float> complexMinusOmega(1, 0); // omega*time or -omega * time -> Complex.setExp will end with  (1,0)  cos(0) and sin(0)


			auto prepareFFTIN = [this, &complexOmega, &complexMinusOmega](maxon::Int32 l)
			{
				maxon::Int32 i = l / M_;
				maxon::Int32 j = maxon::Mod(l, M_);
				fftin_(i, j) = h0_(i, j) * complexOmega +
					h0_minus_(i, j).GetConjugate()	* complexMinusOmega;
			};

			maxon::ParallelFor::Dynamic(0, M_ * N_, prepareFFTIN);


			const maxon::FFTRef KissFFT = maxon::FFTClasses::Kiss().Create() iferr_return;
			const maxon::FFT_SUPPORT options = KissFFT.GetSupportOptions();
			if (!(options & maxon::FFT_SUPPORT::TRANSFORM_2D))
				return res; // return a scale of 1.0


			KissFFT.Transform2D(fftin_, dispY_, fft_flags_) iferr_return;


			maxon::Float maxHeight = std::numeric_limits<maxon::Float>::min();

			// avoid using parallel loop to get max value.
			for (maxon::Int32 i = 0; i < dispY_.GetXCount(); i++)
				for (maxon::Int32 j = 0; j < dispY_.GetYCount(); j++)
					maxHeight = maxon::Max(maxHeight, maxon::Abs(dispY_(i, j)));

			if (maxHeight == 0.0)
				maxHeight = 0.00001; // avoid 0
			// should be "res = 1.0 / maxHeight" but this is already scaled by A_ so we devide A_ by maxHeight
			res = A_ / maxHeight;



			return res;
		};

		//----------------------------------------------------------------------------------------
		/// compute the jacobian variable
		/// @param[in]  jxx  : the jacobian xx param 
		/// @param[in]  jzz  : the jacobian zz param 
		/// @param[in]  jxz  : the jacobian xz param 
		/// @param[out] jMinus : the result of jacob
		/// return maxon::OK on success
		//----------------------------------------------------------------------------------------
		maxon::Result<void>			ComputeEigen(const maxon::Float &jxx, const maxon::Float &jzz, const maxon::Float &jxz, maxon::Float &jMinus) const
		{
			maxon::Float a, b;
			// maxon::Float qplus, qminus;
			a = jxx + jzz;
			b = maxon::Sqrt((jxx - jzz)*(jxx - jzz) + 4.0 * jxz * jxz);  // (equation 32)

			jMinus = 0.5*(a - b);


			return maxon::OK;
		};



		//----------------------------------------------------------------------------------------
		/// @param[in]  windDirection  : the wind Direction that have to be translate to a Vector 
		/// @return the wind Direciton's vector normalized.
		//----------------------------------------------------------------------------------------
		maxon::Vector2d				GetWindDirectionNormalized(const maxon::Float windDirection) const
		{
			maxon::Vector2d windVector = maxon::Vector2d(maxon::Cos(windDirection), - maxon::Sin(windDirection));
			windVector.Normalize();
			return windVector;
		}


		


	}; // end of OceanImplementation

	
	MAXON_COMPONENT_CLASS_REGISTER(OceanImplementation, Ocean);

} // end of namespace OceanSimulation
