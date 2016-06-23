#ifndef MISSION_DEFS_HPP
#define MISSION_DEFS_HPP

#ifndef M_PI
#define M_PI 3.14159265358979323846
#define POOL_D 16
#endif

enum
{
	M_ZERO = 0,
	M_GATE_X,
	M_GATE_Y,
	M_RBUOY_X,
	M_RBUOY_Y,
	M_RBUOY_D,
	M_GBUOY_X,
	M_GBUOY_Y,
	M_GBUOY_D,
	M_YBUOY_X,
	M_YBUOY_Y,
	M_YBUOY_D,
	M_PVC_X,
	M_PVC_Y,
	M_OBIN_X,
	M_OBIN_Y,
	M_CBIN_X,
	M_CBIN_Y,
	M_TORPEDO_X,
	M_TORPEDO_Y,
	M_H1_X,
	M_H1_Y,
	M_H2_X,
	M_H2_Y,
	M_H3_X,
	M_H3_Y,
	M_H4_X,
	M_H4_Y,
	M_PINGER_X,
	M_PINGER_Y,
	NUM_VARS
};

enum
{
	I_FRONT,
	I_DOWN,
	NUM_IMAGES
};

enum
{
	F_BUOYS,
	F_BINS,
	F_TORPS,
	F_PVC,
	NUM_FLAGS
};

#endif

