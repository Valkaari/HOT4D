//#include "maxon/commandbase.h"
//#include "maxon/apibase.h"

#include "maxon/objectbase.h"
#include "maxon/vector.h"
#include "maxon/vector2d.h"


namespace OceanSimulation {
	//----------------------------------------------------------------------------------------
	/// interpolation enumeration
	//----------------------------------------------------------------------------------------
	enum class INTERTYPE // : int // to avoid error ? 
	{
		LINEAR = 1,
		CATMULLROM
	} MAXON_ENUM_LIST(INTERTYPE);






	class OceanInterface : MAXON_INTERFACE_BASES(maxon::Object)
	{
		MAXON_INTERFACE(OceanInterface, MAXON_REFERENCE_NORMAL, "com.valkaari.OceanSimulation.interfaces.ocean");

	public: 
		

		//----------------------------------------------------------------------------------------
		/// Init function to initialise ocean simulation
		/// @param[in]  oceanResolution  : the ocean resolution in on direction M or N
		/// @param[in]  oceanSize  : the ocean size in unit lenght 
		/// @param[in]  shortestWaveLength  : the shortest possible wave (smaller wave will be removed 
		/// @param[in]  amplitude  : the desired height of wave 
		/// @param[in]  windSpeed  : the wind speed 
		/// @param[in]  windDirection  : the wind direction will be transform to a vector cos(windDirection, sin(windDirection) 
		/// @param[in]  alignement  : define if wave must go to the wind direction 
		/// @param[in]  damp  : remove or not the wave that are going reverse to the wind direction 
		/// @param[in]  seed  : the random seed for the simulation 
		/// @return		maxon::OK on success
		//----------------------------------------------------------------------------------------
		MAXON_METHOD maxon::Result<void> Init(maxon::Int32 oceanResolution, maxon::Float oceanSize, maxon::Float shortestWaveLength,
			maxon::Float amplitude, maxon::Float windSpeed, maxon::Float windDirection, maxon::Float alignement, maxon::Float damp,
			maxon::Int32 seed);


		//----------------------------------------------------------------------------------------
		/// check if parameters inside object need to be updated
		//----------------------------------------------------------------------------------------
		MAXON_METHOD maxon::Bool NeedUpdate(const maxon::Int32 oceanResolution, const maxon::Float oceanSize, const maxon::Float shortestWaveLength,
			const maxon::Float amplitude, const maxon::Float windSpeed, const maxon::Float windDirection, const maxon::Float alignement, const maxon::Float damp,
			const maxon::Int32 seed) const;

		//----------------------------------------------------------------------------------------
		/// update the wave height field at time t
		/// @param[in]  currentTime  : the new time to calculate the update 
		/// @param[in]  loopPeriod  : the period after the simulation loop
		/// @param[in]  timeScale  : the scale of time or wave speed 
		/// @param[in]  oceanDepth  : the ocean's depth 
		/// @param[in]  chopAmount  : the chop amount for chopiness. 
		/// @param[in]  doDisp  : do displacement have to be calculate 
		/// @param[in]  doChop  : do chopiness have to be calculate  
		/// @param[in]  doJacob  : do jacobian have to be calculate 
		/// @param[in]  doNormals  : do normals have to be calculate 
		/// @return		maxon::OK on success.
		//----------------------------------------------------------------------------------------
		MAXON_METHOD maxon::Result<void>			Animate(maxon::Float currentTime, maxon::Int32 loopPeriod, maxon::Float timeScale, maxon::Float oceanDepth, maxon::Float chopAmount, maxon::Bool doDisp, maxon::Bool doChop, maxon::Bool doJacob, maxon::Bool doNormals);

		//----------------------------------------------------------------------------------------
		/// Evaluate the result of the simulation and return the vectors for displacemenet, normals and jacobian
		/// @param[in] type : interpolation type that have to be used.
		/// @param[in]  p  :  coordinate of the point to evaluate (only the x and z will be taken into account
		/// @param[out]  displacement  : reference to store the displacement result 
		/// @param[out]  normal  : reference to store the normals result
		/// @param[out]  jMinus : reference to store the minus result
		/// @return		maxon::OK on success
		//----------------------------------------------------------------------------------------
		MAXON_METHOD maxon::Result<void>			EvaluatePoint(const INTERTYPE type, const maxon::Vector p, maxon::Vector &displacement, maxon::Vector &normal, maxon::Float &jMinus) const;

		//----------------------------------------------------------------------------------------
		/// Evaluate the result of the simulation and return the vectors for displacemenet, normals and jacobian
		/// @param[in] type : interpolation type that have to be used.
		/// @param[in]  uv  :  UV coordinate of the point to evaluate
		/// @param[out]  displacement  : reference to store the displacement result 
		/// @param[out]  normal  : reference to store the normals result
		/// @param[out]  jMinus : reference to store the minus result
		/// @return		maxon::OK on success
		//----------------------------------------------------------------------------------------
		MAXON_METHOD maxon::Result<void>			EvaluateUV(const INTERTYPE type, maxon::Vector2d uv, maxon::Vector &displacement, maxon::Vector &normal, maxon::Float &jMinus) const;

	};

	

#include "OceanSimulation_decl1.hxx"

	MAXON_DECLARATION(maxon::Class<OceanRef>, Ocean, "com.valkaari.OceanSimulation.ocean");

#include "OceanSimulation_decl2.hxx"

} // end of namespace OceanSimulation

