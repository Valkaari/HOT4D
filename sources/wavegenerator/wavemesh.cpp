
/*
 *  waveMesh.cpp
 *  waves
 *
 *  Created by Manuel MAGALHAES on 23/12/10.
 *  Copyright 2010 Valkaari. All rights reserved.
 *
 */

#include "c4d.h"
#include "c4d_symbols.h"
//#include "ge_vector.h"

#include "wavemesh.h"
#include "Owavemesh.h"



waveMesh::~waveMesh() {
	if (_ocean)   DeleteMem(_ocean);
	if (_ocean_context) DeleteMem(_ocean_context);
}


Float waveMesh::_MapRange(Float value, Float min_input, Float max_input, Float min_output, Float max_output)
{
    Float inrange = max_input - min_input;
    
    //if (CompareFloatTolerant(value, RCO 0.0)) value = RCO 0.0;  // Prevent DivByZero error
	if (CompareFloatTolerant(value, 0.0)) value = 0.0;  // Prevent DivByZero error
	else value = (value - min_input) / inrange;    // Map input range to [0.0 ... 1.0]
    
    
    if (value > max_output) {return max_output;}
    if (value < min_output) {return min_output;}
    return  min_output + (max_output - min_output) * value; // Map to output range and return result
    
}



Bool waveMesh::Message(GeListNode *node, Int32 type, void *t_data)
{
	switch (type) {
       
		case MSG_MENUPREPARE: {
                ((BaseObject*)node)->SetDeformMode(true);
    
			break;
		}
		/*case MSG_DESCRIPTION_GETINLINEOBJECT: {
			DescriptionInlineObjectMsg *msgData = (DescriptionInlineObjectMsg*)t_data;
			if (msgData->objects->GetCount() > 0) {

				BaseObject *firstObj = (BaseObject*)msgData->objects->GetIndex(0);
				String name = firstObj->GetName();

			}
			


			node->SetDirty(DIRTYFLAGS::ALL);
			break;
		}*/
        
            
		default:
			break;
	}

	
	
	return TRUE;
}




Bool waveMesh::Init(GeListNode *node)
{
	// init the object with some variables in its basecontainer.
	
	BaseObject		*op   = (BaseObject*)node;
	
	BaseContainer *bc = op->GetDataInstance();
   
    op->SetParameter(DescID(OCEAN_RESOLUTION), GeData(7), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(SEED), GeData(12345), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(OCEAN_SIZE), GeData(500.0), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(WIND_SPEED), GeData(20), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(WIND_DIRECTION), GeData(120), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(SHRT_WAVELENGHT), GeData(0.01), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(WAVE_HEIGHT), GeData(30.0), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(CHOPAMOUNT), GeData(0.5), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(DAMP_REFLECT), GeData(1.0), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(WIND_ALIGNMENT), GeData(1.0), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(OCEAN_DEPTH), GeData(200), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(TIME), GeData(0.0), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(DO_CATMU_INTER), GeData(FALSE), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(DO_JACOBIAN), GeData(FALSE), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(DO_CHOPYNESS), GeData(TRUE), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(PSEL_THRES), GeData(0.1), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(JACOB_THRES), GeData(0.5), DESCFLAGS_SET::NONE);
    op->SetParameter(DescID(FOAM_THRES), GeData(0.03), DESCFLAGS_SET::NONE);
    _ocean_context = 0;
    _ocean_scale = 1.0f;
	if (falloff) 
		if (!falloff->InitFalloff(bc, NULL, op)) return false;
	

	
	return true;
}



Bool waveMesh::GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
{
	BaseObject *op = (BaseObject*)node;
	if (!op) return false;
	BaseContainer *bc = op->GetDataInstance();
	if (!bc) return false;

	if (!description->LoadDescription(op->GetType())) return false;

	//---------------------------------
	//Add the falloff interface
	if (falloff)
	{
		if (!falloff->SetMode(FIELDS, bc)) return false; //The falloff parameters have to have been setup before it can be added to the description, this like makes sure of that
		if (!falloff->AddFalloffToDescription(description, bc, DESCFLAGS_DESC::NONE)) return false;
	}

	flags |= DESCFLAGS_DESC::LOADED;

	return true;
}

//Bool waveMesh::AddToExecution(BaseObject *op, PriorityList *list) {
//	list->Add(op, EXECUTIONPRIORITY_INITIAL, EXECUTIONFLAGS::NONE);
//	return TRUE;
//
//}
//EXECUTIONRESULT waveMesh::Execute(BaseObject *op, BaseDocument *doc, BaseThread *bt, Int32 priority, EXECUTIONFLAGS flags)
//{
//	BaseContainer *bc = op->GetDataInstance();
//	if (!bc) return EXECUTIONRESULT::USERBREAK;
//	if (falloff)
//		if (!falloff->InitFalloff(bc, doc, op)) return EXECUTIONRESULT::OUTOFMEMORY;
//
//	return EXECUTIONRESULT::OK;
//
//}

Bool waveMesh::CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn)
{
	waveMesh *df = (waveMesh*)dest;
	if (!df) return FALSE;
	if (falloff && df->falloff)
		if (!falloff->CopyTo(df->falloff)) return FALSE;
	return ObjectData::CopyTo(dest, snode, dnode, flags, trn);
}

DRAWRESULT waveMesh::Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
	if (!op->GetDeformMode()) return DRAWRESULT::SKIP;
	BaseContainer *bc = op->GetDataInstance();
	if (!bc) return DRAWRESULT::FAILURE;
	if (falloff) falloff->Draw(bd, bh, drawpass, bc);
	return ObjectData::Draw(op, drawpass, bd, bh);
}

Int32 waveMesh::GetHandleCount(BaseObject *op)
{
	BaseContainer *bc = op->GetDataInstance();
	if (!bc) return 0;
	if (falloff) return falloff->GetHandleCount(bc);
	return 0;
}

void waveMesh::GetHandle(BaseObject *op, Int32 i, HandleInfo &info)
{
	BaseContainer *bc = op->GetDataInstance();
	if (!bc) return;
	if (falloff)  falloff->GetHandle(i, bc, info);
}

void waveMesh::SetHandle(BaseObject *op, Int32 i, Vector p, const HandleInfo &info)
{

	BaseContainer *bc = op->GetDataInstance();
	if (!bc) return;
	if (falloff) falloff->SetHandle(i, p, bc, info);
}

void waveMesh::CheckDirty(BaseObject* op, BaseDocument* doc)
{
	if (falloff)
	{
		BaseContainer *data = op->GetDataInstance();
		Int32 dirty = falloff->GetDirty(doc,data);
		if (dirty == falloffDirtyCheck) return;
		op->SetDirty(DIRTYFLAGS::DATA);
		falloffDirtyCheck = dirty;
	}
		
}


Bool waveMesh::ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread)
{
	GePrint("modify object"_s);
	if (!op->IsInstanceOf(Opoint) || !falloff) return true;


	Vector                  p;
    Int32                    pcnt;		
    GeData                  data;
    VertexMapTag            *jacobmaptag    = nullptr;
    VertexMapTag            *foammaptag     = nullptr;
    Vector                  *padr           = nullptr;
    Float32                    *jacobpoint     = nullptr;
    Float32                    *foampoint      = nullptr;
    BaseSelect              *bsp            = nullptr;
    SelectionTag            *stag           = nullptr;  
    Float32                   *weight         = nullptr;
    Float                    psel_thres , minvm, maxvm;
    Float                    jacob_thres, foam_thres;
    Vector                  dispvalue;
    
    minvm = maxvm = 0.0;
    
    
    padr = ToPoint(op)->GetPointW();
    pcnt = ToPoint(op)->GetPointCount(); if (!pcnt) return TRUE;
    weight = ToPoint(op)->CalcVertexMap(mod);

    
    mod->GetParameter(DescID(OCEAN_RESOLUTION), data, DESCFLAGS_GET::NONE);
    OceanResolution =    1 <<   data.GetInt32();
   
    mod->GetParameter(DescID(SEED), data, DESCFLAGS_GET::NONE);
    Seed =                  data.GetInt32();
    
    mod->GetParameter(DescID(OCEAN_SIZE), data, DESCFLAGS_GET::NONE);
    OceanSize =             data.GetFloat();

    mod->GetParameter(DescID(WIND_SPEED), data, DESCFLAGS_GET::NONE);
	WindSpeed =             data.GetFloat();
    
    mod->GetParameter(DescID(WIND_DIRECTION), data, DESCFLAGS_GET::NONE);
    WindDirection =			DegToRad(data.GetFloat());
	
    
    mod->GetParameter(DescID(SHRT_WAVELENGHT), data, DESCFLAGS_GET::NONE);
    ShrtWaveLenght =        data.GetFloat();
    
    mod->GetParameter(DescID(WAVE_HEIGHT), data, DESCFLAGS_GET::NONE);
    WaveHeight =            data.GetFloat();
    
    mod->GetParameter(DescID(CHOPAMOUNT), data, DESCFLAGS_GET::NONE);
    chopAmount =            data.GetFloat();
    
    mod->GetParameter(DescID(DAMP_REFLECT), data, DESCFLAGS_GET::NONE);
    DampReflection =        data.GetFloat();
    
    mod->GetParameter(DescID(WIND_ALIGNMENT), data, DESCFLAGS_GET::NONE);
    WindAlign =             data.GetFloat();
    
    mod->GetParameter(DescID(OCEAN_DEPTH), data, DESCFLAGS_GET::NONE);
    OceanDepth =            data.GetFloat();
    
    mod->GetParameter(DescID(TIME), data, DESCFLAGS_GET::NONE);
    Time =                  data.GetFloat();
    
    mod->GetParameter(DescID(DO_CATMU_INTER), data, DESCFLAGS_GET::NONE);
    doCatmuInter =          !data.GetBool();
    
    mod->GetParameter(DescID(DO_JACOBIAN), data, DESCFLAGS_GET::NONE);
    doJacobian =            data.GetBool();
    
    mod->GetParameter(DescID(DO_CHOPYNESS), data, DESCFLAGS_GET::NONE);
    doChopyness =           data.GetBool();
    
    mod->GetParameter(DescID(DO_NORMALS), data, DESCFLAGS_GET::NONE);
    doNormals=              data.GetBool() &&  !doChopyness;
    
    mod->GetParameter(DescID(JACOBMAP), data, DESCFLAGS_GET::NONE);
    jacobmaptag = (VertexMapTag*)data.GetLink(doc,Tvertexmap);
    
    mod->GetParameter(DescID(FOAMMAP), data, DESCFLAGS_GET::NONE);
    foammaptag = (VertexMapTag*)data.GetLink(doc,Tvertexmap);

    mod->GetParameter(DescID(PSEL_PARTICLES), data, DESCFLAGS_GET::NONE);
    stag = (SelectionTag*)data.GetLink(doc,Tpointselection);

    mod->GetParameter(DescID(PSEL_THRES), data, DESCFLAGS_GET::NONE);
    psel_thres = data.GetFloat();
    
    mod->GetParameter(DescID(JACOB_THRES), data, DESCFLAGS_GET::NONE);
    jacob_thres = data.GetFloat();
    
    mod->GetParameter(DescID(FOAM_THRES), data, DESCFLAGS_GET::NONE);
    foam_thres = data.GetFloat();
    
    if (jacobmaptag)  {
        if (jacobmaptag->GetDataCount() == pcnt) {
                jacobpoint = (Float32*)jacobmaptag->GetDataAddressW();   
        }
        else {
            jacobmaptag = NULL;
            jacobpoint  = NULL;
        }
    }
  
    if (foammaptag)  {
        if (foammaptag->GetDataCount() == pcnt) {
            foampoint = (Float32*)foammaptag->GetDataAddressW();   
        }
        else {
            foammaptag = NULL;
            foampoint  = NULL;
        }
    }
    

    if (stag) {
        bsp = stag->GetBaseSelect();
        if (bsp) {bsp->DeselectAll();}
    }
    

      
    stepsize = OceanSize / (float)OceanResolution;

    //if (_ocean) {gDelete(_ocean);}
    //if (_ocean_context) {gDelete(_ocean_context);}

	if (_ocean)   DeleteMem(_ocean);
	if (_ocean_context) DeleteMem(_ocean_context);
    
    //_ocean = gNew drw::Ocean(OceanResolution, OceanResolution, stepsize, stepsize , WindSpeed , ShrtWaveLenght , 1.0 , WindDirection , 1-DampReflection , WindAlign , OceanDepth , Seed);
	_ocean = NewObjClear(drw::Ocean, OceanResolution, OceanResolution, stepsize, stepsize, WindSpeed, ShrtWaveLenght, 1.0, WindDirection, 1 - DampReflection, WindAlign, OceanDepth, Seed);
	
    
    _ocean_scale = _ocean->get_height_normalize_factor();
    
    _ocean_context =_ocean->new_context(true, doChopyness, doNormals, doJacobian);
    
    _ocean->update(Time, *_ocean_context, true, doChopyness, doNormals, doJacobian, _ocean_scale *WaveHeight, chopAmount);    
    
	/*
	FieldInput inputs;
	for (Int32 i = 0; i < pcnt; i++)
	{
		input
	}
	
	falloff->PreSample(doc, op, inputs, FIELDSAMPLE_FLAG::VALUE);


	
	*/

	FieldInput inputs(padr,pcnt,op_mg);
	
	
	Bool outputsOK = falloff->PreSample(doc, mod, inputs, FIELDSAMPLE_FLAG::VALUE);
	


	//const FieldOutput* outputs = falloff->GetSamples();

	Float fallOffSampleValue(1.0);
	//outputsOK = outputs->GetCount() == pcnt && outputsOK;
	    
	


    for (Int32 i = 0; i < pcnt; i++) {
        p = padr[i];
		

		
		if (outputsOK)
		//	//fallOffSampleValue = outputs->_value[i];
			falloff->Sample(p, &fallOffSampleValue,true, 0.0, nullptr, i);
		
        
        if (doCatmuInter) {
            _ocean_context->eval_xz(p.x, p.z);
        }
        else {
            _ocean_context->eval2_xz(p.x, p.z);
        }
        
        if (weight) {
            dispvalue = Vector(_ocean_context->disp[0]* weight[i], _ocean_context->disp[1]* weight[i] ,_ocean_context->disp[2]* weight[i]) ; 
        }
        else {
            dispvalue = Vector(_ocean_context->disp[0],_ocean_context->disp[1],_ocean_context->disp[2]); 
        }
        
		dispvalue *= fallOffSampleValue; // falloff

        if (doChopyness) {
            p +=dispvalue;
        }
        else {
            p.y += dispvalue.y;
        }
        
        if (doJacobian) {
            Float jminusvalue = _ocean_context->Jminus;
            
            if (weight) {jminusvalue *=weight[i];}
            
            if (jacobmaptag && jacobpoint) {
                jacobpoint[i] = jminusvalue;
                if (jminusvalue > maxvm) {maxvm = jminusvalue;}
                if (jminusvalue < minvm) {minvm = jminusvalue;}
                
            }
            if (bsp) {
                if (jminusvalue < psel_thres) {
                    bsp->Toggle(i);
                }
            }
        }
        else if (jacobmaptag && jacobpoint) {
            jacobpoint[i] = 0.0;
        }
        padr[i] = p; 
        
    }
 
    
    if (jacobmaptag && jacobpoint) {
        for (Int32 i = 0 ; i < pcnt; i++) {
            jacobpoint[i] = 1 - _MapRange(jacobpoint[i], minvm, maxvm, 0, 1);                
            
        }
        jacobmaptag->SetDirty(DIRTYFLAGS::NONE);

    }
    
    if (foammaptag && foampoint && jacobpoint && jacobmaptag) {
        BaseTime currentTime;
        Int32     currentFrame;
        currentTime = doc->GetTime();
        currentFrame = currentTime.GetFrame(doc->GetFps());        
        
        
        for (Int32 i = 0; i < pcnt; i++) {
            if (currentFrame == 0) {
                foampoint[i] = 0.0;
            }
            else {
                if (jacobpoint[i] > jacob_thres) {
                    foampoint[i] += _MapRange(jacobpoint[i], jacob_thres, 1, 0, 1) - foam_thres;
                }
                else {
                    foampoint[i] -= foam_thres;
                }
                if (foampoint[i] < 0.0) {foampoint[i]=0.0;}
                if (foampoint[i] > 1.0) {foampoint[i]=1.0;}
            }
            
        }
        
    }
    

    if (stag)  {
        stag->SetDirty(DIRTYFLAGS::NONE);
        stag->Message(MSG_UPDATE);
    }
       
    
    
    op->Message(MSG_UPDATE);

    //GeFree(weight);
	DeleteMem(weight);
    
	return TRUE;
  
}



// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_WAVEMESHGENE 1026624

Bool RegisterwaveMesh(void)
{
		
	return RegisterObjectPlugin(ID_WAVEMESHGENE,"HOT4D"_s,OBJECT_MODIFIER,waveMesh::Alloc,"OwaveMesh"_s, AutoBitmap("hot4D.tif"_s),0);

}

// helpers

