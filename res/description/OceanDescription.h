#ifndef _OCEAN_DESCRIPTION_H_
#define _OCEAN_DESCRIPTION_H_

enum
{
    ID_OCEAN_DESCRIPTION = 1051486,
    OCEAN_RESOLUTION = 10000,
	OCEAN_SIZE,
    WIND_SPEED,
    WIND_DIRECTION,
    SHRT_WAVELENGHT,
    WAVE_HEIGHT,
    SEED,
    CHOPAMOUNT,
    DAMP_REFLECT,
    WIND_ALIGNMENT,
    OCEAN_DEPTH,
    CURRENTTIME,
    TIMELOOP,
    TIMESCALE,
    AUTO_ANIM_TIME,
    DO_CHOPYNESS,
    DO_CATMU_INTER,
    DO_NORMALS,
    DO_JACOBIAN,
    JACOBMAP,
    ID_GRP_FOAM,
    FOAMMAP,
    PRE_RUN_FOAM,
    JACOB_THRES,
    FOAM_THRES,
    PSEL_PARTICLES,
    PSEL_THRES,
    ACTIVE_DEFORM,

    // for the cycle, create power of 2
    RESO_4 =4,
    RESO_5 =5,
    RESO_6 =6,
    RESO_7 =7,
    RESO_8 =8,
    RESO_9 =9,
    RESO_10 =10,
    RESO_11 = 11,
    RESO_12 = 12,
    RESO_13 = 13,
    RESO_14 = 14
    
};

#endif