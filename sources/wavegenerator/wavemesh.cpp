
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
#include "ge_vector.h"

#include "wavemesh.h"
#include "Owavemesh.h"








waveMesh::~waveMesh() {
    if (_ocean)          gDelete(_ocean);
    if (_ocean_context ) gDelete(_ocean_context);
  
}


Real waveMesh::_MapRange(Real value, Real min_input, Real max_input, Real min_output, Real max_output)

{
    
    Real inrange = max_input - min_input;
    
    if (CompareFloatTolerant(value, RCO 0.0)) value = RCO 0.0;  // Prevent DivByZero error
    
    else value = (value - min_input) / inrange;    // Map input range to [0.0 ... 1.0]
    
    
    
    if (value > max_output) {return max_output;}
    if (value < min_output) {return min_output;}
    return  min_output + (max_output - min_output) * value; // Map to output range and return result
    
}



Bool waveMesh::Message(GeListNode *node, LONG type, void *t_data)
{
	switch (type) {
       
		case MSG_MENUPREPARE: {
                ((BaseObject*)node)->SetDeformMode(TRUE);
    
			break;
		}
        
            
		default:
			break;
	}
	
	
	return TRUE;
}




Bool waveMesh::Init(GeListNode *node)
{
	// init the object with some variables in its basecontainer.

	

	
	BaseObject		*op   = (BaseObject*)node;

   
    op->SetParameter(DescID(OCEAN_RESOLUTION), GeData(7), DESCFLAGS_SET_0);
    op->SetParameter(DescID(SEED), GeData(12345), DESCFLAGS_SET_0);
    op->SetParameter(DescID(OCEAN_SIZE), GeData(500.0), DESCFLAGS_SET_0);
    op->SetParameter(DescID(WIND_SPEED), GeData(20), DESCFLAGS_SET_0);
    op->SetParameter(DescID(WIND_DIRECTION), GeData(120), DESCFLAGS_SET_0);
    op->SetParameter(DescID(SHRT_WAVELENGHT), GeData(0.01), DESCFLAGS_SET_0);
    op->SetParameter(DescID(WAVE_HEIGHT), GeData(30.0), DESCFLAGS_SET_0);
    op->SetParameter(DescID(CHOPAMOUNT), GeData(0.5), DESCFLAGS_SET_0);
    op->SetParameter(DescID(DAMP_REFLECT), GeData(1.0), DESCFLAGS_SET_0);
    op->SetParameter(DescID(WIND_ALIGNMENT), GeData(1.0), DESCFLAGS_SET_0);
    op->SetParameter(DescID(OCEAN_DEPTH), GeData(200), DESCFLAGS_SET_0);
    op->SetParameter(DescID(TIME), GeData(0.0), DESCFLAGS_SET_0);
    op->SetParameter(DescID(DO_CATMU_INTER), GeData(FALSE), DESCFLAGS_SET_0);
    op->SetParameter(DescID(DO_JACOBIAN), GeData(FALSE), DESCFLAGS_SET_0);
    op->SetParameter(DescID(DO_CHOPYNESS), GeData(TRUE), DESCFLAGS_SET_0);
    op->SetParameter(DescID(PSEL_THRES), GeData(0.1), DESCFLAGS_SET_0);
    op->SetParameter(DescID(JACOB_THRES), GeData(0.5), DESCFLAGS_SET_0);
    op->SetParameter(DescID(FOAM_THRES), GeData(0.03), DESCFLAGS_SET_0);
    _ocean_context = 0;
    _ocean_scale = 1.0f;
    
   		
	return TRUE;
}







Bool waveMesh::ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Real lod, LONG flags, BaseThread *thread)
{

	if (!op->IsInstanceOf(Opoint)) return TRUE;


	Vector                  p;
    LONG                    pcnt;		
    GeData                  data;
    VertexMapTag            *jacobmaptag    = NULL;
    VertexMapTag            *foammaptag     = NULL;
    Vector                  *padr           = NULL;
    SReal                    *jacobpoint     = NULL;
    SReal                    *foampoint      = NULL;
    BaseSelect              *bsp            = NULL;
    SelectionTag            *stag           = NULL;  
    SReal                   *weight         = NULL;
    Real                    psel_thres , minvm, maxvm;
    Real                    jacob_thres, foam_thres;
    Vector                  dispvalue;
    
    minvm = maxvm = 0.0;
    
    
    padr = ToPoint(op)->GetPointW();
    pcnt = ToPoint(op)->GetPointCount(); if (!pcnt) return TRUE;
    weight = ToPoint(op)->CalcVertexMap(mod);

    
    mod->GetParameter(DescID(OCEAN_RESOLUTION), data, DESCFLAGS_GET_0);
    OceanResolution =    1 <<   data.GetLong();
   
    mod->GetParameter(DescID(SEED), data, DESCFLAGS_GET_0);
    Seed =                  data.GetLong();
    
    mod->GetParameter(DescID(OCEAN_SIZE), data, DESCFLAGS_GET_0);
    OceanSize =             data.GetReal();

    mod->GetParameter(DescID(WIND_SPEED), data, DESCFLAGS_GET_0);
	WindSpeed =             data.GetReal();
    
    mod->GetParameter(DescID(WIND_DIRECTION), data, DESCFLAGS_GET_0);
    WindDirection =         Rad(data.GetReal());
    
    mod->GetParameter(DescID(SHRT_WAVELENGHT), data, DESCFLAGS_GET_0);
    ShrtWaveLenght =        data.GetReal();
    
    mod->GetParameter(DescID(WAVE_HEIGHT), data, DESCFLAGS_GET_0);
    WaveHeight =            data.GetReal();
    
    mod->GetParameter(DescID(CHOPAMOUNT), data, DESCFLAGS_GET_0);
    chopAmount =            data.GetReal();
    
    mod->GetParameter(DescID(DAMP_REFLECT), data, DESCFLAGS_GET_0);
    DampReflection =        data.GetReal();
    
    mod->GetParameter(DescID(WIND_ALIGNMENT), data, DESCFLAGS_GET_0);
    WindAlign =             data.GetReal();
    
    mod->GetParameter(DescID(OCEAN_DEPTH), data, DESCFLAGS_GET_0);
    OceanDepth =            data.GetReal();
    
    mod->GetParameter(DescID(TIME), data, DESCFLAGS_GET_0);
    Time =                  data.GetReal();
    
    mod->GetParameter(DescID(DO_CATMU_INTER), data, DESCFLAGS_GET_0);
    doCatmuInter =          !data.GetBool();
    
    mod->GetParameter(DescID(DO_JACOBIAN), data, DESCFLAGS_GET_0);
    doJacobian =            data.GetBool();
    
    mod->GetParameter(DescID(DO_CHOPYNESS), data, DESCFLAGS_GET_0);
    doChopyness =           data.GetBool();
    
    mod->GetParameter(DescID(DO_NORMALS), data, DESCFLAGS_GET_0);
    doNormals=              data.GetBool() &&  !doChopyness;
    
    mod->GetParameter(DescID(JACOBMAP), data, DESCFLAGS_GET_0);
    jacobmaptag = (VertexMapTag*)data.GetLink(doc,Tvertexmap);
    
    mod->GetParameter(DescID(FOAMMAP), data, DESCFLAGS_GET_0);
    foammaptag = (VertexMapTag*)data.GetLink(doc,Tvertexmap);

    mod->GetParameter(DescID(PSEL_PARTICLES), data, DESCFLAGS_GET_0);
    stag = (SelectionTag*)data.GetLink(doc,Tpointselection);

    mod->GetParameter(DescID(PSEL_THRES), data, DESCFLAGS_GET_0);
    psel_thres = data.GetReal();
    
    mod->GetParameter(DescID(JACOB_THRES), data, DESCFLAGS_GET_0);
    jacob_thres = data.GetReal();
    
    mod->GetParameter(DescID(FOAM_THRES), data, DESCFLAGS_GET_0);
    foam_thres = data.GetReal();
    
    if (jacobmaptag)  {
        if (jacobmaptag->GetDataCount() == pcnt) {
                jacobpoint = (SReal*)jacobmaptag->GetDataAddressW();   
        }
        else {
            jacobmaptag = NULL;
            jacobpoint  = NULL;
        }
    }
  
    if (foammaptag)  {
        if (foammaptag->GetDataCount() == pcnt) {
            foampoint = (SReal*)foammaptag->GetDataAddressW();   
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

    if (_ocean) {gDelete(_ocean);}
    if (_ocean_context) {gDelete(_ocean_context);}
    
    _ocean = gNew drw::Ocean(OceanResolution, OceanResolution, stepsize, stepsize , WindSpeed , ShrtWaveLenght , 1.0 , WindDirection , 1-DampReflection , WindAlign , OceanDepth , Seed);
    
    _ocean_scale = _ocean->get_height_normalize_factor();
    
    _ocean_context =_ocean->new_context(true, doChopyness, doNormals, doJacobian);
    
    _ocean->update(Time, *_ocean_context, true, doChopyness, doNormals, doJacobian, _ocean_scale *WaveHeight, chopAmount);    
    
       
    
    for (LONG i = 0; i < pcnt; i++) {
        p = padr[i];
        
        
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
        
        if (doChopyness) {
            p +=dispvalue;
        }
        else {
            p.y += dispvalue.y;
        }
        
        if (doJacobian) {
            Real jminusvalue = _ocean_context->Jminus;
            
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
        for (LONG i = 0 ; i < pcnt; i++) {
            jacobpoint[i] = 1 - _MapRange(jacobpoint[i], minvm, maxvm, 0, 1);                
            
        }
        jacobmaptag->SetDirty(DIRTYFLAGS_0);

    }
    
    if (foammaptag && foampoint && jacobpoint && jacobmaptag) {
        BaseTime currentTime;
        LONG     currentFrame;
        currentTime = doc->GetTime();
        currentFrame = currentTime.GetFrame(doc->GetFps());        
        
        
        for (LONG i = 0; i < pcnt; i++) {
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
        stag->SetDirty(DIRTYFLAGS_0);
        stag->Message(MSG_UPDATE);
    }
       
    
    
       op->Message(MSG_UPDATE);

    GeFree(weight);
    
	return TRUE;
  
}



// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_WAVEMESHGENE 1026624

Bool RegisterwaveMesh(void)
{
		
	return RegisterObjectPlugin(ID_WAVEMESHGENE,"HOT4D",OBJECT_MODIFIER,waveMesh::Alloc,"OwaveMesh",AutoBitmap("hot4D.tif"),0);
}

// helpers

