/*
 *  main.cpp
 *  waves
 *
 *  Created by Manuel MAGALHAES on 23/12/10.
 *  Copyright 2010 Valkaari. All rights reserved.
 *
 */
#include "c4d.h"
#include "c4d_symbols.h"


C4D_CrashHandler old_handler;

// forward declarations


Bool RegisterWaveShaderDisp(void);
Bool RegisterwaveMesh(void);
//Bool RegisterJacobShader(void);



Bool PluginStart(void)
{

	//if (!RegisterWaveShaderDisp()) return FALSE;
    
//    if (!RegisterJacobShader()) return FALSE;

	GePrint(maxon::String("---------------"));
	GePrint(maxon::String("HOT4D For R20 v0.4"));
	GePrint(maxon::String("---------------"));
	if (!RegisterwaveMesh()) return FALSE;
	if (!RegisterWaveShaderDisp()) return FALSE;


	return TRUE;
}

void PluginEnd(void)
{
	
}

Bool PluginMessage(Int32 id, void *data)
{
	//use the following lines to set a plugin priority
	//
	switch (id)
	{
		case C4DPL_INIT_SYS:
			
			if (!g_resource.Init()) return FALSE; // don't start plugin without resource
			return TRUE;
			
		
	}
	
	return FALSE;
}
