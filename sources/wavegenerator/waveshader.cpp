/*
 *  waveshader.cpp
 *  waves
 *
 *  Created by Manuel MAGALHAES on 31/01/11.
 *  Copyright 2011 Valkaari. All rights reserved.
 *
 */
#include "c4d.h"
#include "c4d_symbols.h"

#include <vector>
#include "c4d_baselist.h"
#include <cmath>
#include "Xhot4ddisp.h"
#include "waveshader.h"




Bool WaveShaderDisp::Init(GeListNode *node)
{
	BaseContainer *data = ((BaseShader*)node)->GetDataInstance();
    data->SetInt32(DISP_TYPE, TAN_DISP);
    data->SetInt32(OCEAN_RESOLUTION,6);
    data->SetInt32(SEED,12345);
    
    data->SetFloat(OCEAN_SIZE,1);
	data->SetFloat(WIND_SPEED,60);
    data->SetFloat(WIND_DIRECTION,120.0);
    data->SetFloat(SHRT_WAVELENGHT,0.01);
    data->SetFloat(WAVE_HEIGHT,30);
    data->SetFloat(CHOPAMOUNT,0.97);
    data->SetFloat(DAMP_REFLECT,1.0);
    data->SetFloat(WIND_ALIGNMENT,1.0);
    data->SetFloat(OCEAN_DEPTH,200.0);
    data->SetFloat(TIME,0.0);

    data->SetBool(DO_CATMU_INTER,FALSE);
    data->SetBool(DO_JACOBIAN, FALSE);
    data->SetBool(DO_CHOPYNESS, TRUE);
   	return TRUE;
}
Float WaveShaderDisp::_MapRange(Float value, Float min_input, Float max_input, Float min_output, Float max_output)

{
    
    Float inrange = max_input - min_input;
    
    if (CompareFloatTolerant(value, 0.0)) value =  0.0;  // Prevent DivByZero error
    
    else value = (value - min_input) / inrange;    // Map input range to [0.0 ... 1.0]
    
    
    
    if (value > max_output) {return max_output;}
    if (value < min_output) {return min_output;}
    return  min_output + (max_output - min_output) * value; // Map to output range and return result
    
}


INITRENDERRESULT WaveShaderDisp::InitRender(BaseShader *sh, const InitRenderStruct &irs)

{
    _ocean = 0;
    _ocean_context = 0;
    _ocean_scale = 1.0f;
    
    _ocean_needs_rebuild = true;
    
    
    
	BaseContainer *data = sh->GetDataInstance();
	// get the data from UI
    OceanResolution =    1 <<   data->GetInt32(OCEAN_RESOLUTION);
    Seed =                  data->GetInt32(SEED);
    
    OceanSize =             data->GetFloat(OCEAN_SIZE);
	WindSpeed =             data->GetFloat(WIND_SPEED);
    WindDirection =         DegToRad(data->GetFloat(WIND_DIRECTION));
    ShrtWaveLenght =        data->GetFloat(SHRT_WAVELENGHT)/1000;
    WaveHeight =            data->GetFloat(WAVE_HEIGHT);
    chopAmount =            data->GetFloat(CHOPAMOUNT);
    DampReflection =        data->GetFloat(DAMP_REFLECT);
    WindAlign =             data->GetFloat(WIND_ALIGNMENT);
    OceanDepth =            data->GetFloat(OCEAN_DEPTH);
    Time =                  data->GetFloat(TIME);
    
    doCatmuInt32er =          !data->GetBool(DO_CATMU_INTER);
    doJacobian =            data->GetBool(DO_JACOBIAN);
    doChopyness =           data->GetBool(DO_CHOPYNESS);
    doNormals=              data->GetBool(DO_NORMALS) &&  !doChopyness;
    disp_type=              data->GetInt32(DISP_TYPE);
    
    stepsize = OceanSize / (Float)OceanResolution;
    oneOverStepSize = 1 / stepsize;
    
    if (_ocean) {DeleteMem (_ocean);}
    if (_ocean_context) {DeleteMem(_ocean_context);}
    
    _ocean = NewObjClear(drw::Ocean,OceanResolution, OceanResolution, stepsize, stepsize , WindSpeed , ShrtWaveLenght , 1.0 , WindDirection , 1-DampReflection , WindAlign , OceanDepth , Seed);
    
    _ocean_scale = _ocean->get_height_normalize_factor();
    
    _ocean_context =_ocean->new_context(true, doChopyness, doNormals, doJacobian);
    
    _ocean->update(Time, *_ocean_context, true, doChopyness, doNormals, doJacobian, _ocean_scale *WaveHeight, chopAmount);
    
    //GeFree(padr);
	DeleteMem(padr);

    pcnt = (OceanResolution + 1 ) * (OceanResolution +1);
    //padr = GeAllocTypeNC(Vector,pcnt );
	iferr (padr = NewMem(Vector, pcnt)) return INITRENDERRESULT::OUTOFMEMORY;
    
    
    Int32 i,j;
    Float x,y;
    
    for (x = 0 ,  j = 0 ; x <= OceanResolution ; x++,j++) {
        for ( y= 0, i=0; y <= OceanResolution ; y++,i++) {
            if (doCatmuInt32er) {
                _ocean_context->eval_xz(x * stepsize, y*stepsize);
            }
            else {
                _ocean_context->eval2_xz(x * stepsize, y*stepsize);    
            }
            
            
            if (doJacobian) {
                padr[i + j * (OceanResolution + 1)] = Vector(_ocean_context->Jminus,_ocean_context->Eminus[0],_ocean_context->Eminus[1]);
                
            }
            else {
                padr[i + j * (OceanResolution + 1)] = Vector(_ocean_context->disp[0],_ocean_context->disp[1],_ocean_context->disp[2]);
            }
            
           // padr[i+j*(OceanResolution + 1)] = Vector(_MapRange(x*y, 0, OceanResolution*OceanResolution, 0, 1));   
        }
    }
  	return INITRENDERRESULT::OK;
}

void WaveShaderDisp::FreeRender(BaseShader *sh)
{
    if (_ocean)          DeleteMem(_ocean);
    if (_ocean_context ) DeleteMem(_ocean_context);
    //GeFree(padr);
    
}



Vector WaveShaderDisp::Output(BaseShader *chn, ChannelData *cd)
{
    
    

    Float r,g,b;
    Float px, py;
    // be sure that 0= < (u,v) < 1
    px = FMod((Float)cd->p.x,1.0);
    py = FMod((Float)cd->p.y,1.0);
    if (px < 0) {px+=1.0;}
    if (py < 0) {py+=1.0;}
    
    px *= OceanResolution;
    py *= OceanResolution;
    
    Int32 Int32x = (Int32)floor(px) ;
    Int32 Int32y = (Int32)floor(py) ;
    Int32 Int32x1 = Int32x + 1;
    Int32 Int32y1 = Int32y + 1;
    
    Float wx = px - Int32x;
    Float wy = py - Int32y;
    

    Int32x = Int32x % OceanResolution;
    Int32y = Int32y % OceanResolution;
    Int32x1 = Int32x1 % OceanResolution;
    Int32y1 = Int32y1 % OceanResolution;
    
    Vector poInt32Value = Vector(0.0);
    

    //bilinear Int32erpolation
 //   poInt32Value += padr[Int32x + Int32y* OceanResolution]* (1-wx) * (1-wy);
/*    
    if (Int32x1   < OceanResolution) {

        poInt32Value += padr[Int32x1 + Int32y * OceanResolution] * wx * (1-wy);
    }
    if( Int32y1  < OceanResolution ) {

        poInt32Value += padr[Int32x + Int32y1* OceanResolution] * (1-wx) * wy;
    }
    
    if (( Int32x1 < OceanResolution) and (Int32y1 < OceanResolution)) {

        poInt32Value += padr[Int32x1 + Int32y1*OceanResolution] * wx * wy;
    }

    if (Int32x1> OceanResolution) {        return Vector(1,0,0);}
    if (Int32y1 > OceanResolution) {        return Vector(1,0,0);}
    
    */
    
    poInt32Value += padr[Int32x + Int32y   * (OceanResolution+1) ]* (1-wx) * (1-wy);
    poInt32Value += padr[Int32x1 + Int32y  * (OceanResolution+1) ] * wx * (1-wy);
    poInt32Value += padr[Int32x + Int32y1  * (OceanResolution+1) ] * (1-wx) * wy;
    poInt32Value += padr[Int32x1 + Int32y1 * (OceanResolution+1) ] * wx * wy;
//    poInt32Value =  padr [Int32x + Int32y * (OceanResolution+1)];

    
    if(doJacobian) {
        r = g = b = _MapRange(-poInt32Value.x,-2, 1, 0, 1);
        
    }
    else {
        if (disp_type == TAN_DISP) {
            r = _MapRange(poInt32Value.x, -WaveHeight, WaveHeight, 0, 1);
            g = _MapRange(poInt32Value.z, -WaveHeight, WaveHeight, 0, 1);
            b = _MapRange(poInt32Value.y, -WaveHeight, WaveHeight, 0, 1);
        }
        else if (disp_type== WORLD_DISP) {
            r = _MapRange(poInt32Value.x, -WaveHeight, WaveHeight, 0, 1);
            g = _MapRange(poInt32Value.y, -WaveHeight, WaveHeight, 0, 1);
            b = _MapRange(poInt32Value.z, -WaveHeight, WaveHeight, 0, 1) * -1;
            
        }
    }
   
   // r=g=b=poInt32Value.y;

    
    
    
    return Vector (r,g,b);
	
   
}

// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_WAVESHADER_DISP	1026641

Bool RegisterWaveShaderDisp(void)
{

	return RegisterShaderPlugin(ID_WAVESHADER_DISP,"Wave Shader"_s,0,WaveShaderDisp::Alloc,"Xhot4ddisp"_s,0);
}

