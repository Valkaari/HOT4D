/*
 *  main.cpp
 *  waves
 *
 *  Created by Manuel MAGALHAES on 23/12/10.
 *  Copyright 2010 Valkaari. All rights reserved.
 *
 */
#include "c4d.h"
#include "main.h"



Bool PluginStart()
{
	VERSIONTYPE versionType = GeGetVersionType();


	if (!RegisterOceanSimulationDescription())
		return false;
	if (!RegisterOceanSimulationDeformer()) 
		return false;

	if (!RegisterOceanSimulationEffector())
		return false;

	
	
	ApplicationOutput("---------------"_s);
	ApplicationOutput("HOT4D For R21 v0.7"_s);
	ApplicationOutput("---------------"_s);


	return true;
}

void PluginEnd()
{
	
}

Bool PluginMessage(Int32 id, void *data)
{
	// use the following lines to set a plugin priority
	//
	switch (id)
	{
		case C4DPL_INIT_SYS:
			
			if (!g_resource.Init()) 
				return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY:
			// react to this message to set a plugin priority (to determine in which order plugins are initialized or loaded
			//	SetPluginPriority(data, C4DPL_INIT_PRIORITY_PLUGINS);
			return true;
		
	}
	
	return false;
}
