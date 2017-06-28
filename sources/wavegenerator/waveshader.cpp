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
    data->SetLong(DISP_TYPE, TAN_DISP);
    data->SetLong(OCEAN_RESOLUTION,6);
    data->SetLong(SEED,12345);
    
    data->SetReal(OCEAN_SIZE,1);
	data->SetReal(WIND_SPEED,60);
    data->SetReal(WIND_DIRECTION,120.0);
    data->SetReal(SHRT_WAVELENGHT,0.01);
    data->SetReal(WAVE_HEIGHT,30);
    data->SetReal(CHOPAMOUNT,0.97);
    data->SetReal(DAMP_REFLECT,1.0);
    data->SetReal(WIND_ALIGNMENT,1.0);
    data->SetReal(OCEAN_DEPTH,200.0);
    data->SetReal(TIME,0.0);

    data->SetBool(DO_CATMU_INTER,FALSE);
    data->SetBool(DO_JACOBIAN, FALSE);
    data->SetBool(DO_CHOPYNESS, TRUE);
   	return TRUE;
}
Real WaveShaderDisp::_MapRange(Real value, Real min_input, Real max_input, Real min_output, Real max_output)

{
    
    Real inrange = max_input - min_input;
    
    if (CompareFloatTolerant(value, RCO 0.0)) value = RCO 0.0;  // Prevent DivByZero error
    
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
    OceanResolution =    1 <<   data->GetLong(OCEAN_RESOLUTION);
    Seed =                  data->GetLong(SEED);
    
    OceanSize =             data->GetReal(OCEAN_SIZE);
	WindSpeed =             data->GetReal(WIND_SPEED);
    WindDirection =         Rad(data->GetReal(WIND_DIRECTION));
    ShrtWaveLenght =        data->GetReal(SHRT_WAVELENGHT)/1000;
    WaveHeight =            data->GetReal(WAVE_HEIGHT);
    chopAmount =            data->GetReal(CHOPAMOUNT);
    DampReflection =        data->GetReal(DAMP_REFLECT);
    WindAlign =             data->GetReal(WIND_ALIGNMENT);
    OceanDepth =            data->GetReal(OCEAN_DEPTH);
    Time =                  data->GetReal(TIME);
    
    doCatmuInter =          !data->GetBool(DO_CATMU_INTER);
    doJacobian =            data->GetBool(DO_JACOBIAN);
    doChopyness =           data->GetBool(DO_CHOPYNESS);
    doNormals=              data->GetBool(DO_NORMALS) &&  !doChopyness;
    disp_type=              data->GetLong(DISP_TYPE);
    
    stepsize = OceanSize / (Real)OceanResolution;
    oneOverStepSize = 1 / stepsize;
    
    if (_ocean) {gDelete(_ocean);}
    if (_ocean_context) {gDelete(_ocean_context);}
    
    _ocean = gNew drw::Ocean(OceanResolution, OceanResolution, stepsize, stepsize , WindSpeed , ShrtWaveLenght , 1.0 , WindDirection , 1-DampReflection , WindAlign , OceanDepth , Seed);
    
    _ocean_scale = _ocean->get_height_normalize_factor();
    
    _ocean_context =_ocean->new_context(true, doChopyness, doNormals, doJacobian);
    
    _ocean->update(Time, *_ocean_context, true, doChopyness, doNormals, doJacobian, _ocean_scale *WaveHeight, chopAmount);
    
    GeFree(padr);
    
    pcnt = (OceanResolution + 1 ) * (OceanResolution +1);
    padr = GeAllocTypeNC(Vector,pcnt );
    if (!padr) return INITRENDERRESULT_OUTOFMEMORY;
    
    LONG i,j;
    Real x,y;
    
    for (x = 0 ,  j = 0 ; x <= OceanResolution ; x++,j++) {
        for ( y= 0, i=0; y <= OceanResolution ; y++,i++) {
            if (doCatmuInter) {
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
  	return INITRENDERRESULT_OK;
}

void WaveShaderDisp::FreeRender(BaseShader *sh)
{
    if (_ocean)          gDelete(_ocean);
    if (_ocean_context ) gDelete(_ocean_context);
    GeFree(padr);
    
}



Vector WaveShaderDisp::Output(BaseShader *chn, ChannelData *cd)
{
    
    

    Real r,g,b;
    Real px, py;
    // be sure that 0= < (u,v) < 1
    px = FMod((Real)cd->p.x,1.0);
    py = FMod((Real)cd->p.y,1.0);
    if (px < 0) {px+=1.0;}
    if (py < 0) {py+=1.0;}
    
    px *= OceanResolution;
    py *= OceanResolution;
    
    LONG intx = (LONG)floor(px) ;
    LONG inty = (LONG)floor(py) ;
    LONG intx1 = intx + 1;
    LONG inty1 = inty + 1;
    
    Real wx = px - intx;
    Real wy = py - inty;
    

    intx = intx % OceanResolution;
    inty = inty % OceanResolution;
    intx1 = intx1 % OceanResolution;
    inty1 = inty1 % OceanResolution;
    
    Vector pointValue = Vector(0.0);
    

    //bilinear interpolation
 //   pointValue += padr[intx + inty* OceanResolution]* (1-wx) * (1-wy);
/*    
    if (intx1   < OceanResolution) {

        pointValue += padr[intx1 + inty * OceanResolution] * wx * (1-wy);
    }
    if( inty1  < OceanResolution ) {

        pointValue += padr[intx + inty1* OceanResolution] * (1-wx) * wy;
    }
    
    if (( intx1 < OceanResolution) and (inty1 < OceanResolution)) {

        pointValue += padr[intx1 + inty1*OceanResolution] * wx * wy;
    }

    if (intx1> OceanResolution) {        return Vector(1,0,0);}
    if (inty1 > OceanResolution) {        return Vector(1,0,0);}
    
    */
    
    pointValue += padr[intx + inty   * (OceanResolution+1) ]* (1-wx) * (1-wy);
    pointValue += padr[intx1 + inty  * (OceanResolution+1) ] * wx * (1-wy);
    pointValue += padr[intx + inty1  * (OceanResolution+1) ] * (1-wx) * wy;
    pointValue += padr[intx1 + inty1 * (OceanResolution+1) ] * wx * wy;
//    pointValue =  padr [intx + inty * (OceanResolution+1)];

    
    if(doJacobian) {
        r = g = b = _MapRange(-pointValue.x,-2, 1, 0, 1);
        
    }
    else {
        if (disp_type == TAN_DISP) {
            r = _MapRange(pointValue.x, -WaveHeight, WaveHeight, 0, 1);
            g = _MapRange(pointValue.z, -WaveHeight, WaveHeight, 0, 1);
            b = _MapRange(pointValue.y, -WaveHeight, WaveHeight, 0, 1);
        }
        else if (disp_type== WORLD_DISP) {
            r = _MapRange(pointValue.x, -WaveHeight, WaveHeight, 0, 1);
            g = _MapRange(pointValue.y, -WaveHeight, WaveHeight, 0, 1);
            b = _MapRange(pointValue.z, -WaveHeight, WaveHeight, 0, 1) * -1;
            
        }
    }
   
   // r=g=b=pointValue.y;

    
    
    
    return Vector (r,g,b);
	
   
}

// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_WAVESHADER_DISP	1026641

Bool RegisterWaveShaderDisp(void)
{
	return RegisterShaderPlugin(ID_WAVESHADER_DISP,"Wave Shader",0,WaveShaderDisp::Alloc,"Xhot4ddisp",0);
}

