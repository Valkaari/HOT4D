#pragma once
//  wavemesh.h
//  
//  Created by Manuel MAGALHAES on 14/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//


#include "c4d_falloffdata.h"

class OceanSimulationDeformer : public ObjectData
{
	INSTANCEOF(OceanSimulationDeformer, ObjectData);
	
private:

	

	// This is where all the wave action takes place
	//newOcean::Ocean						*ocean_;  ///< ocean main object.
	maxon::Float						currentTime_; ///< store the current time of the animation used in check dirty
	OceanSimulation::OceanRef			oceanSimulationRef_; ///< ocean reference

	// manage falloff
	AutoAlloc<C4D_Falloff>		falloff_; ///< the falloff object to be compatible with fields.
	maxon::Int32				falloffDirtyCheck_; ///< store the checkdirty to see if the fields have changed.
	
	maxon::Float				MapRange(maxon::Float value, const maxon::Float min_input, const maxon::Float max_input, const maxon::Float min_output, const maxon::Float max_output) const;
public:




	virtual Int32				GetHandleCount(BaseObject *op);
	virtual void				GetHandle(BaseObject *op, Int32 i, HandleInfo &info);
	virtual void				SetHandle(BaseObject *op, Int32 i, Vector p, const HandleInfo &info);
	virtual Bool				CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn);
	virtual DRAWRESULT			Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	
	virtual Bool				GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);

	virtual void				CheckDirty(BaseObject* op, BaseDocument* doc);
	virtual Bool				GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc);


	// end of function for managing falloff.

	virtual Bool                Init(GeListNode *node);
	// removed free because of the use of reference of ocean object
	// virtual void				Free(GeListNode *node);
	virtual Bool                Message(GeListNode *node, Int32 type, void *t_data);
	virtual Bool                ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);


	
	static NodeData *Alloc() { return NewObjClear(OceanSimulationDeformer); }
};
