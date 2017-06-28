CONTAINER Xhot4ddisp
{
	NAME Xhot4ddisp;

	INCLUDE Mpreview;
	INCLUDE Xbase;

	GROUP ID_SHADERPROPERTIES
	{
        LONG DISP_TYPE {
            CYCLE {
                WORLD_DISP;
                TAN_DISP;
            }
        
        }
		LONG OCEAN_RESOLUTION{ MIN 1; MAX 20; }
        REAL OCEAN_SIZE { MIN 0; MAX 10000; MINEX;}
        REAL WIND_SPEED { MIN 0.0; MAX 100;}
        REAL WIND_DIRECTION { MIN 0; MAX 360;}
        REAL SHRT_WAVELENGHT { MIN 0.0; MAX 10;}
        REAL WAVE_HEIGHT { MIN 0.0; MAX 1000;}
        LONG SEED { MIN 0; MAX 1000000;}
        BOOL DO_CHOPYNESS {}
        REAL CHOPAMOUNT { MIN 0; MAX 100;}
        REAL DAMP_REFLECT { MIN 0; MAX 1;}
        REAL WIND_ALIGNMENT { MIN 0; MAX 1;}
        REAL OCEAN_DEPTH { MIN 0; MAX 1000;}
        REAL TIME { MIN 0; MAX 100;}
        
        BOOL DO_CATMU_INTER { }
        BOOL DO_NORMALS { }
        BOOL DO_JACOBIAN { }
        
	}
}
