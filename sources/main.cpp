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
Bool RegisterJacobShader(void);
void SDKCrashHandler(CHAR *crashinfo)
{
	
	// don't forget to call the original handler!!!
	if (old_handler) (*old_handler)(crashinfo);
}



Bool PluginStart(void)
{

	if (!RegisterWaveShaderDisp()) return FALSE;
    if (!RegisterwaveMesh()) return FALSE;
//    if (!RegisterJacobShader()) return FALSE;
	return TRUE;
}

void PluginEnd(void)
{
	
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	//
	switch (id)
	{
		case C4DPL_INIT_SYS:
			
			if (!resource.Init()) return FALSE; // don't start plugin without resource
			return TRUE;
			
		case C4DMSG_PRIORITY: 
			return TRUE;
			
			
	}
	
	return FALSE;
}
