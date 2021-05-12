
#include "c4d.h"
#include "c4d_symbols.h"

#include "description/OceanDescription.h"



Bool RegisterOceanSimulationDescription();
Bool RegisterOceanSimulationDescription()
{

	return RegisterDescription(ID_OCEAN_DESCRIPTION, "OceanDescription"_s);

}
