//
//  jacob_shader.h
//  wavedispshader
//
//  Created by Manuel MAGALHAES on 15/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//

#ifndef wavedispshader_jacob_shader_h
#define wavedispshader_jacob_shader_h







class XJacob : public ShaderData
{
public:
	
	virtual Bool                Init		(GeListNode *node);
	virtual	Vector              Output      (BaseShader *chn, ChannelData *cd);
	virtual	INITRENDERRESULT    InitRender  (BaseShader *sh, const InitRenderStruct &irs);
	virtual	void                FreeRender  (BaseShader *sh);
	
	static NodeData *Alloc(void) { return gNew XJacob; }
    
    
    drw::Ocean        *_ocean;
    drw::OceanContext *_ocean_context;
    float              _ocean_scale;
    
    BaseList2D           *link;
    waveMesh           *hot4d; 
    bool _ocean_needs_rebuild;
    
private:
    double                      OceanSize, WindSpeed, WindDirection, ShrtWaveLenght, WaveHeight, chopAmount, DampReflection, WindAlign, OceanDepth, Time;
    int                         OceanResolution, Seed;
    bool                        doCatmuInter, doJacobian, doChopyness, doNormals;
    Real                        _MapRange(Real value, Real min_input, Real max_input, Real min_output, Real max_output);
    
    
    // This is where all the wave action takes place
    
};

/*
 
 init is from NodeDATA CLASS.
 Called when a new instance of the node plugin has been allocated. You can use this function to for example fill the BaseContainer of the connected node with default values:
 
 Output is from shaderData class
 Called for each point of the visible surface of a shaded object. Here you should calculate and return the channel color for the point cd->p.
 Important: No OS calls are allowed during this function. Doing so could cause a crash, since it can be called in a multi-processor context.
 
 InitRender is from ShaderData CLass :
 Called to precalculate data for rendering. You can store the allocated data as a member of your class.
 Note: If you allocate any resources then you need to supply the FreeRender() callback to free those resources.
 
 FreeRender is from shaderData Class :
 Free any resources used for the precalculated data from InitRender().
 
 
 
 */


#endif
