
#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_baseeffectordata.h"


// #include "c4d_baseeffectorplugin.h"



#include "OceanDescription.h"
#include "OceanSimulation/OceanSimulation_decl.h"




#include "OceanSimulationEffector.h"


#define ID_OCEAN_SIMULATION_EFFECTOR 1051489

Bool OceanSimulationEffector::GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
{
	if (id[0].id == CURRENTTIME)
	{
		// current Time have to be disable if auto anim is on
		GeData data;
		node->GetParameter(DescID(AUTO_ANIM_TIME), data, DESCFLAGS_GET::NONE);
		return !data.GetBool();
	}

	return SUPER::GetDEnabling(node, id, t_data, flags, itemdesc);

}

// Int32 	OceanSimulationEffector::GetEffectorFlags()
// {
//
//	// don't work as i want =)
//	return  EFFECTORFLAGS_TIMEDEPENDENT;
// }




Bool 	OceanSimulationEffector::AddToExecution(BaseObject *op, PriorityList *list)
{
	list->Add(op, EXECUTIONPRIORITY_EXPRESSION, EXECUTIONFLAGS::NONE);

	return true;
}

EXECUTIONRESULT 	OceanSimulationEffector::Execute(BaseObject *op, BaseDocument *doc, BaseThread *bt, Int32 priority, EXECUTIONFLAGS flags)
{

	

	if (priority != EXECUTIONPRIORITY_EXPRESSION)
		return EXECUTIONRESULT::OK;
	

	maxon::Bool doAutoTime;

	GeData							uiData;
	op->GetParameter(DescID(AUTO_ANIM_TIME), uiData, DESCFLAGS_GET::NONE);
	doAutoTime = uiData.GetBool();


	if (doAutoTime)
	{
		BaseTime btCurrentTime;
		maxon::Float    currentFrame;
		btCurrentTime = doc->GetTime();
		currentFrame = (maxon::Float)btCurrentTime.GetFrame(doc->GetFps());
		if (currentTime_ != currentFrame)
		{
			currentTime_ = currentFrame;
			op->SetDirty(DIRTYFLAGS::DATA);
		}
	}

	return EXECUTIONRESULT::OK;


}


Bool OceanSimulationEffector::InitEffector(GeListNode* node)
{
	BaseObject		*op = (BaseObject*)node;
	if (!op)
		return false;


	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;


	op->SetParameter(DescID(OCEAN_RESOLUTION), GeData(7), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(SEED), GeData(12345), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(OCEAN_SIZE), GeData(400.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(WIND_SPEED), GeData(20.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(WIND_DIRECTION), GeData(120.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(SHRT_WAVELENGHT), GeData(0.01), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(WAVE_HEIGHT), GeData(30.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(CHOPAMOUNT), GeData(0.5), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(DAMP_REFLECT), GeData(1.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(WIND_ALIGNMENT), GeData(1.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(OCEAN_DEPTH), GeData(200.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(CURRENTTIME), GeData(0.0), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(TIMELOOP), GeData(90), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(TIMESCALE), GeData(0.5), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(AUTO_ANIM_TIME), GeData(true), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(DO_CATMU_INTER), GeData(false), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(DO_JACOBIAN), GeData(false), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(DO_CHOPYNESS), GeData(true), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(PSEL_THRES), GeData(0.1), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(JACOB_THRES), GeData(0.5), DESCFLAGS_SET::NONE);
	op->SetParameter(DescID(FOAM_THRES), GeData(0.03), DESCFLAGS_SET::NONE);

	
	bc->SetFloat(ID_MG_BASEEFFECTOR_MINSTRENGTH, -1.0);
	bc->SetBool(ID_MG_BASEEFFECTOR_POSITION_ACTIVE, true);
	bc->SetVector(ID_MG_BASEEFFECTOR_POSITION, Vector(50.0));
	
	iferr_scope_handler{
		err.DiagOutput();
		
		return false;
	};

	if (oceanSimulationRef_ == nullptr)
	{
		oceanSimulationRef_ = OceanSimulation::Ocean().Create() iferr_return;
	}

	

	return true;
}

void OceanSimulationEffector::InitPoints(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, MoData* md, BaseThread* thread)
{
	
	BaseContainer* bc = op->GetDataInstance();
	if (!bc)
		return;

	iferr_scope_handler{
		err.DiagOutput();
		
		return;
	};

	if (oceanSimulationRef_ == nullptr)
	{
		oceanSimulationRef_ = OceanSimulation::Ocean().Create() iferr_return;
	}

	maxon::Float					oceanSize, windSpeed, windDirection, shrtWaveLenght, waveHeight, chopAmount, dampReflection, windAlign, oceanDepth, timeScale;
	maxon::Int32					oceanResolution, seed, timeLoop;
	maxon::Bool						doJacobian, doChopyness, doAutoTime;

	GeData							uiData;
	op->GetParameter(DescID(OCEAN_RESOLUTION), uiData, DESCFLAGS_GET::NONE);
	oceanResolution = 1 << uiData.GetInt32();

	op->GetParameter(DescID(OCEAN_SIZE), uiData, DESCFLAGS_GET::NONE);
	oceanSize = uiData.GetFloat();

	op->GetParameter(DescID(SHRT_WAVELENGHT), uiData, DESCFLAGS_GET::NONE);
	shrtWaveLenght = uiData.GetFloat();

	op->GetParameter(DescID(WAVE_HEIGHT), uiData, DESCFLAGS_GET::NONE);
	waveHeight = uiData.GetFloat();

	op->GetParameter(DescID(WIND_SPEED), uiData, DESCFLAGS_GET::NONE);
	windSpeed = uiData.GetFloat();

	op->GetParameter(DescID(WIND_DIRECTION), uiData, DESCFLAGS_GET::NONE);
	windDirection = DegToRad(uiData.GetFloat());

	op->GetParameter(DescID(WIND_ALIGNMENT), uiData, DESCFLAGS_GET::NONE);
	windAlign = uiData.GetFloat();

	op->GetParameter(DescID(DAMP_REFLECT), uiData, DESCFLAGS_GET::NONE);
	dampReflection = uiData.GetFloat();

	op->GetParameter(DescID(SEED), uiData, DESCFLAGS_GET::NONE);
	seed = uiData.GetInt32();

	op->GetParameter(DescID(OCEAN_DEPTH), uiData, DESCFLAGS_GET::NONE);
	oceanDepth = uiData.GetFloat();

	op->GetParameter(DescID(CHOPAMOUNT), uiData, DESCFLAGS_GET::NONE);
	chopAmount = uiData.GetFloat();

	op->GetParameter(DescID(TIMELOOP), uiData, DESCFLAGS_GET::NONE);
	timeLoop = uiData.GetInt32();

	op->GetParameter(DescID(TIMESCALE), uiData, DESCFLAGS_GET::NONE);
	timeScale = uiData.GetFloat();


	op->GetParameter(DescID(AUTO_ANIM_TIME), uiData, DESCFLAGS_GET::NONE);
	doAutoTime = uiData.GetBool();

	

	if (!doAutoTime)
	{
		op->GetParameter(DescID(CURRENTTIME), uiData, DESCFLAGS_GET::NONE);
		currentTime_ = uiData.GetFloat();
	}

	

	op->GetParameter(DescID(DO_CHOPYNESS), uiData, DESCFLAGS_GET::NONE);
	doChopyness = uiData.GetBool();



	


	if (oceanSimulationRef_.NeedUpdate(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed))
	{
		oceanSimulationRef_.Init(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed) iferr_return;
	}

	oceanSimulationRef_.Animate(currentTime_, timeLoop, timeScale, oceanDepth, chopAmount, true, doChopyness, false, false) iferr_return;
	
}

maxon::Result<void> OceanSimulationEffector::EvaluatePoint(BaseObject* op, const maxon::Vector p, maxon::Vector &displacement) const
{
	iferr_scope;

	maxon::Float waveHeight;
	maxon::Bool doCatmuInter;

	GeData							uiData;
	op->GetParameter(DescID(WAVE_HEIGHT), uiData, DESCFLAGS_GET::NONE);
	waveHeight = uiData.GetFloat();



	op->GetParameter(DescID(DO_CATMU_INTER), uiData, DESCFLAGS_GET::NONE);
	doCatmuInter = uiData.GetBool();

	OceanSimulation::INTERTYPE interType = OceanSimulation::INTERTYPE::LINEAR;
	if (doCatmuInter)
		interType = OceanSimulation::INTERTYPE::CATMULLROM;

	maxon::Vector normal;
	maxon::Float jMinus;
	oceanSimulationRef_.EvaluatePoint(interType, p, displacement, normal, jMinus) iferr_return;
	displacement /= waveHeight; // scale down the result by the wavelegnth so the result should be beetween -1 and 1
	// jMinus /= waveHeight;
	return  maxon::OK;
}


void OceanSimulationEffector::CalcPointValue(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight)
{

	
	iferr_scope_handler
	{
		err.DbgStop();
		return;
	};
	maxon::Vector disp;
	

	
	EvaluatePoint(op, globalpos, disp) iferr_return;

	EffectorStrengths* es = (EffectorStrengths*)data->strengths;
	
	es->pos = disp;
	es->rot = disp;
	es->scale = disp;

}

Vector OceanSimulationEffector::CalcPointColor(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight)
{
	iferr_scope_handler
	{
		err.DbgStop();
		return Vector(0);
	};
	maxon::Vector disp;
	
	

	EvaluatePoint(op, globalpos, disp) iferr_return;


	return disp;

}



Bool RegisterOceanSimulationEffector();
Bool RegisterOceanSimulationEffector()

{
	return RegisterEffectorPlugin(ID_OCEAN_SIMULATION_EFFECTOR, "Ocean Simulation Effector"_s, OBJECT_CALL_ADDEXECUTION,
									OceanSimulationEffector::Alloc, "OOceanEffector"_s, AutoBitmap("hot4D_eff.tif"_s), 0);
}



