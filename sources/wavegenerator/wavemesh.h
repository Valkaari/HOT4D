//  wavemesh.h
//  wavedispshader
//
//  Created by Manuel MAGALHAES on 14/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//

#ifndef wavemesh_h
#define wavemesh_h

#include "Ocean.h"
#include "c4d_falloffdata.h"

class waveMesh : public ObjectData
{
	private :
    
    Float                      OceanSize, WindSpeed, WindDirection, ShrtWaveLenght, WaveHeight, chopAmount, DampReflection, WindAlign, OceanDepth, Time;
    Int32                         OceanResolution, Seed;
    Bool                        doCatmuInter, doJacobian, doChopyness, doNormals;
    float                       stepsize;
    
    
    
    ~waveMesh(void);
	
public:
    
    // This is where all the wave action takes place
    drw::Ocean                  *_ocean;
    drw::OceanContext           *_ocean_context;
    float                       _ocean_scale;
    Bool                        _ocean_needs_rebuild;
    
    Float                            _MapRange(Float value, Float min_input, Float max_input, Float min_output, Float max_output);


	// manage falloff
	AutoAlloc<C4D_Falloff>		falloff;
	Int32						falloffDirtyCheck;

	virtual Int32				GetHandleCount(BaseObject *op);
	virtual void				GetHandle(BaseObject *op, Int32 i, HandleInfo &info);
	virtual void				SetHandle(BaseObject *op, Int32 i, Vector p, const HandleInfo &info);
	virtual Bool				CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn);
	virtual DRAWRESULT			Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	//virtual Bool				AddToExecution(BaseObject *op, PriorityList *list);
	//virtual EXECUTIONRESULT		Execute(BaseObject *op, BaseDocument *doc, BaseThread *bt, Int32 priority, EXECUTIONFLAGS flags);
	virtual Bool				GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);

	virtual void				CheckDirty(BaseObject* op, BaseDocument* doc);



   // end of function for managing falloff.

	virtual Bool                Init				(GeListNode *node);
	virtual Bool                Message				(GeListNode *node, Int32 type, void *t_data);
	virtual Bool                ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);
	
	
	//static NodeData *Alloc(void) { return gNew waveMesh; }
	static NodeData *Alloc(void) { return NewObjClear(waveMesh); }
};




#endif
