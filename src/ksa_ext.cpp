#include "lua/lua.hpp"

typedef struct {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
} PIXEL_BGRA;

typedef struct {
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
} TRSGRAD_PARAM;

static float
calc_grad(int x, int y, TRSGRAD_PARAM *p)
{
	float d = p->sx * ( x - p->cx ) + p->sy * ( y - p->cy );
	if ( d < -0.5f ) {
		return p->a0;
	} else if ( 0.5f < d ) {
		return p->a1;
	} else {
		return (a_int+a_cef*d);
	}
}

static int
ksa_trsgrad(lua_State *L)
{
	TRSGRAD_PARAM p;
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			data->a = static_cast<unsigned char>( data->a * calc_grad(x, y, &p) );
		}
	}
}

static const luaL_Reg ksa_ext[] = {
	{ nullptr, nullptr }
};

extern "C" {
__declspec(dllexport) int
luaopen_ksa_ext(lua_State *L)
{
	luaL_register(L, nullptr, ksa_ext);
	return 1;
}
}
