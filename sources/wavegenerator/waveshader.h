/*
 *  waveshader.h
 *  waves
 *
 *  Created by Manuel MAGALHAES on 31/01/11.
 *  Copyright 2011 Valkaari. All rights reserved.
 *
 */




/////////////////////////////////////////////////////////////
// CINEMA 4D SDK                                           //
/////////////////////////////////////////////////////////////
// (c) 1989-2004 MAXON Computer GmbH, all rights reserved  //
/////////////////////////////////////////////////////////////

// example for an easy implementation of a channel shader


#include "Ocean.h"




class WaveShaderDisp : public ShaderData
{
public:
	
	virtual Bool                    Init		(GeListNode *node);
	virtual	Vector                  Output      (BaseShader *chn, ChannelData *cd);
	virtual	INITRENDERRESULT        InitRender  (BaseShader *sh, const InitRenderStruct &irs);
	virtual	void                    FreeRender  (BaseShader *sh);
	
	static NodeData *Alloc() { return NewObjClear(WaveShaderDisp); }
    
    
    drw::Ocean                      *_ocean;
    drw::OceanContext               *_ocean_context;
    Float                           _ocean_scale;
    Vector                          *padr;
    Int32                            pcnt;
    Float                            stepsize;
    Float                            oneOverStepSize;
    Int32                            disp_type;
    Bool                            _ocean_needs_rebuild;

private:
    Float                          OceanSize, WindSpeed, WindDirection, ShrtWaveLenght, WaveHeight, chopAmount, DampReflection, WindAlign, OceanDepth, Time;
    Int32                             OceanResolution, Seed;
    Bool                            doCatmuInt32er, doJacobian, doChopyness, doNormals;
    Float                            _MapRange(Float value, Float min_input, Float max_input, Float min_output, Float max_output);
    
    
    // This is where all the wave action takes place
    
};

/*

 init is from NodeDATA CLASS.
    Called when a new instance of the node plugin has been allocated. You can use this function to for example fill the BaseContainer of the connected node with default values:
 
 Output is from shaderData class
    Called for each poInt32 of the visible surface of a shaded object. Here you should calculate and return the channel color for the poInt32 cd->p.
    Important: No OS calls are allowed during this function. Doing so could cause a crash, since it can be called in a multi-processor context.
 
 InitRender is from ShaderData CLass :
    Called to precalculate data for rendering. You can store the allocated data as a member of your class.
    Note: If you allocate any resources then you need to supply the FreeRender() callback to free those resources.
 
 FreeRender is from shaderData Class :
    Free any resources used for the precalculated data from InitRender().
 
 

*/
