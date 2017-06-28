//
//  wavemesh.h
//  wavedispshader
//
//  Created by Manuel MAGALHAES on 14/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//

#ifndef wavemesh_h
#define wavemesh_h

#include "Ocean.h"

class waveMesh : public ObjectData
{
	private :
    
    double                      OceanSize, WindSpeed, WindDirection, ShrtWaveLenght, WaveHeight, chopAmount, DampReflection, WindAlign, OceanDepth, Time;
    int                         OceanResolution, Seed;
    bool                        doCatmuInter, doJacobian, doChopyness, doNormals;
    float                       stepsize;
    
    
    
    ~waveMesh(void);
	
public:
    
    // This is where all the wave action takes place
    drw::Ocean                  *_ocean;
    drw::OceanContext           *_ocean_context;
    float                       _ocean_scale;
    bool                        _ocean_needs_rebuild;
    
    Real                            _MapRange(Real value, Real min_input, Real max_input, Real min_output, Real max_output);

	
   

	virtual Bool                Init				(GeListNode *node);
	virtual Bool                Message				(GeListNode *node, LONG type, void *t_data);
	virtual Bool                ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Real lod, LONG flags, BaseThread *thread);
    
	
	static NodeData *Alloc(void) { return gNew waveMesh; }
};




#endif
