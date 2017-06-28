//
//  jacob_shader.cpp
//  wavedispshader
//
//  Created by Manuel MAGALHAES on 15/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//

#include <iostream>
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
#include "c4d_baselist.h"

#include <vector>
#include <cmath>
#include "Ocean.h"
#include "wavemesh.h"

#include "Xjacob.h"
#include "jacob_shader.h"




Bool XJacob::Init(GeListNode *node)
{
	BaseContainer *data = ((BaseShader*)node)->GetDataInstance();
      	return TRUE;
}
Real XJacob::_MapRange(Real value, Real min_input, Real max_input, Real min_output, Real max_output)

{
    
    Real inrange = max_input - min_input;
    
    if (CompareFloatTolerant(value, RCO 0.0)) value = RCO 0.0;  // Prevent DivByZero error
    
    else value = (value - min_input) / inrange;    // Map input range to [0.0 ... 1.0]
    if (value > max_output) return max_output;
    if (value < min_input) return min_input;
    return  min_output + (max_output - min_output) * value; // Map to output range and return result
    
}


INITRENDERRESULT XJacob::InitRender(BaseShader *sh, const InitRenderStruct &irs)

{
    BaseContainer  *data = sh->GetDataInstance();   
    BaseDocument   *doc = sh->GetDocument();
    
    link = data->GetLink(ID_LINKHOT4D, doc);
    if (!link) return INITRENDERRESULT_UNKNOWNERROR;
    hot4d = (waveMesh*)link->GetNodeData();
    if (!hot4d) return INITRENDERRESULT_UNKNOWNERROR;
    
    
   	return INITRENDERRESULT_OK;
}

void XJacob::FreeRender(BaseShader *sh)
{
        
    link = NULL;
    hot4d = NULL;
    
}



Vector XJacob::Output(BaseShader *chn, ChannelData *cd)
{
    
    Real r,g,b,px, py, pz;
    
    px = cd->p.x ;
    py = cd->p.y ;
    pz = cd->p.z ;

    
    Real value = 0.5;
    if (value < 0.5) {value = 0.0;}
    else {value = 1.0;}

    
    r = value;
    g = value;
    b = value;

    Vector color = Vector (r,g,b);
    return color;
	
    
}

// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_WAVESHADER_DISP	1029666

Bool RegisterJacobShader(void)
{
	return RegisterShaderPlugin(ID_WAVESHADER_DISP,"Xjacob",0,XJacob::Alloc,"XJacob",0);
}

