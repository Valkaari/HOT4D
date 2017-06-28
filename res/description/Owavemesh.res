CONTAINER Owavemesh
{
	NAME Owavemesh;
	INCLUDE Obase;

	GROUP ID_OBJECTPROPERTIES
	{
        
      DEFAULT 1;
        COLUMNS 1;
        LONG OCEAN_RESOLUTION{ 
         CYCLE {
                RESO_4;
                RESO_5;
                RESO_6;
                RESO_7;
                RESO_8;
                RESO_9;
                RESO_10;
                RESO_11;
         
            }
         
           }
        REAL OCEAN_SIZE { MIN 1; MAX 100000000;STEP 1;}
        REAL WIND_SPEED { MIN 0; MAX 100000; STEP 1; MINEX;}
        REAL WIND_DIRECTION { MIN 0; MAX 360;STEP 1; CUSTOMGUI REALSLIDER;}
        REAL SHRT_WAVELENGHT { MIN 0.01; MAX 10; STEP 0.1; CUSTOMGUI REALSLIDER;}
        REAL WAVE_HEIGHT { MIN 0.01; MAX 100000;STEP 1;}
        LONG SEED { MIN 0; MAX 1000000;STEP 1;}
        BOOL DO_CHOPYNESS {}
        REAL CHOPAMOUNT {MINEX; MIN 0; MAX 5; STEP 0.1; CUSTOMGUI REALSLIDER;}
        REAL DAMP_REFLECT { MIN 0; MAX 1; STEP 0.1; CUSTOMGUI REALSLIDER;}
        REAL WIND_ALIGNMENT { MIN 0; MAX 1;STEP 0.1; CUSTOMGUI REALSLIDER;}
        REAL OCEAN_DEPTH { MIN 0; MAX 10000;STEP 1;}
        REAL TIME { MIN 0; MAX 10000000000000; STEP 1;}
        
        BOOL DO_CATMU_INTER { }
        BOOL DO_NORMALS { }
        BOOL DO_JACOBIAN { }
        LINK JACOBMAP {ACCEPT { 5682; } }
        REAL JACOB_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
        LINK FOAMMAP  {ACCEPT { 5682; } }
        REAL FOAM_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
        LINK PSEL_PARTICLES {ACCEPT  {Tpointselection; }}
        REAL PSEL_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
        
        



	}



	
}
