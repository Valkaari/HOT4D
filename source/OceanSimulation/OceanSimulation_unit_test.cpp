#include "OceanSimulation_decl.h"


#include "maxon/unittest.h"
#include "maxon/lib_math.h"


namespace OceanSimulation
{

	class SimpleOceanSimulationUnitTest : public maxon::UnitTestComponent<SimpleOceanSimulationUnitTest>
	{

		MAXON_COMPONENT();


	public:
		maxon::Result<void> Run()
		{

			iferr_scope;


			MAXON_SCOPE
			{
				
				

				// test simulation
				iferr (OceanSimulation::OceanRef simpleOceanRef = OceanSimulation::Ocean().Create())
				{
					self.AddResult("Create the ocean object"_s, err);
					return err;
				}
				else
				{
					self.AddResult("Create the ocean object"_s, err);

				}
			
				maxon::Result<void> res = simpleOceanRef.Init(128, 500, 0.01, 30, 20, 120, 1, 1, 12345);
				self.AddResult("init the ocean simulation"_s, res);
				
				res = simpleOceanRef.Animate(0.3, 30, 1.0, 200, 1.0, true, true, true, false);
				self.AddResult("Animate the ocean simulation"_s, res);


				maxon::Vector disp, normal;
				maxon::Float jminus;
			
				maxon::Vector p(0);

				res = simpleOceanRef.EvaluatePoint(INTERTYPE::LINEAR, p, disp, normal, jminus);
				self.AddResult(FormatString("Evaluate the point @ and result is @", p , disp  ), res);


			}

			return maxon::OK;
		}

	};

	class SimpleOceanSimulationSpeedTest : public maxon::UnitTestComponent<SimpleOceanSimulationSpeedTest>
	{

		MAXON_COMPONENT();


	public:
		maxon::Result<void> Run()
		{

			iferr_scope;


			MAXON_SCOPE
			{
				// speed test simulation
				maxon::TimeValue   t = maxon::TimeValue::GetTime();
			
				
				ifnoerr (OceanSimulation::OceanRef simpleOceanRef = OceanSimulation::Ocean().Create())
				{	
					self.AddTimingResult("Time to create the ocean Object"_s, err, t.Stop());
				}
				else
				{
					self.AddTimingResult("time to create the ocean Object"_s, err, t.Stop());
					return err;
				}
				

				for (maxon::Int32 j = 7; j < 11; j++)
				{
					t = maxon::TimeValue::GetTime();
					maxon::Int32 resolution = 1 << j;

					maxon::Result<void> res = simpleOceanRef.Init(resolution, 500, 0.01, 30, 20, 120, 1, 1, 12345);

					
					self.AddTimingResult(FormatString("init the ocean res @", resolution ), res, t.Stop());



					for (maxon::Int32 i = 0; i < 10; i++)
					{
						t = maxon::TimeValue::GetTime();
						res = simpleOceanRef.Animate(0.3*i, 30, 1.0, 200, 1.0, true, true, true, false);
						self.AddTimingResult(FormatString("Animate the ocean resolution @", resolution), res, t.Stop());
					}
				}




				maxon::Vector disp, normal;
				maxon::Float jminus;
				maxon::Int32 cnt = 5;
				maxon::LinearCongruentialRandom<maxon::Float64> rand;
				rand.Init(123456);
				INTERTYPE randInter[2] = { INTERTYPE::LINEAR, INTERTYPE::CATMULLROM };


				for (maxon::Int32 i = 0; i < cnt;  i ++)
				{
					maxon::Vector p(rand.Get11()*100, rand.Get11()*100, rand.Get11()*100);
					t = maxon::TimeValue::GetTime();
					INTERTYPE interSwitch = randInter[rand.Get11() < 0 ? 0 : 1];

					maxon::Result<void> res = simpleOceanRef.EvaluatePoint(interSwitch, p, disp, normal, jminus);
					maxon::String interpolation = ( interSwitch == INTERTYPE::LINEAR ? "Linear interpolation"_s : "catmul inerpolation"_s);
					self.AddTimingResult(FormatString("Evaluate the point with @,  @ and result is @", interpolation , p, disp), res, t.Stop());
				}

			


				





			}

			return maxon::OK;
		}

	};


	MAXON_COMPONENT_CLASS_REGISTER(SimpleOceanSimulationUnitTest, maxon::UnitTestClasses, "com.valkaari.OceanSimulation.unittest.SimpleOceanSimulation");
	MAXON_COMPONENT_CLASS_REGISTER(SimpleOceanSimulationSpeedTest, maxon::SpeedTestClasses, "com.valkaari.OceanSimulation.unittest.SimpleOceanSimulationSpeed");
}
	