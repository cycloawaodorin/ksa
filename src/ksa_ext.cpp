#include "lua/lua.hpp"
#include <math.h>

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
calc_grad(float x, float y, TRSGRAD_PARAM *p)
{
	float d = p->sx * ( x - p->cx ) + p->sy * ( y - p->cy );
	if ( d < -0.5f ) {
		return p->a0;
	} else if ( 0.5f < d ) {
		return p->a1;
	} else {
		return (p->a_int+p->a_cef*d);
	}
}

static int
ksa_trsgrad(lua_State *L)
{
	// 引数受け取り
	int i=0;
	PIXEL_BGRA *data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	int w = static_cast<int>(lua_tointeger(L, ++i));
	int h = static_cast<int>(lua_tointeger(L, ++i));
	TRSGRAD_PARAM p;
	p.cx = static_cast<float>(lua_tonumber(L, ++i));
	p.cy = static_cast<float>(lua_tonumber(L, ++i));
	float angle = static_cast<float>(lua_tonumber(L, ++i));
	float gwidth = static_cast<float>(lua_tonumber(L, ++i));
	p.a0 = static_cast<float>(lua_tonumber(L, ++i));
	p.a1 = static_cast<float>(lua_tonumber(L, ++i));
	
	// パラメータ計算
	p.sx = -sinf(angle)/gwidth;
	p.sy = cosf(angle)/gwidth;
	p.a_cef = p.a1-p.a0;
	p.a_int = (p.a0+p.a1)*0.5f;
	
	// グラデーション反映
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			data->a = static_cast<unsigned char>( data->a * calc_grad(static_cast<float>(x), static_cast<float>(y), &p) );
			data++;
		}
	}
	
	return 0;
}

static const luaL_Reg ksa_ext[] = {
	{ "trsgrad", ksa_trsgrad },
	{ nullptr, nullptr }
};

extern "C" {
__declspec(dllexport) int
luaopen_ksa_ext(lua_State *L)
{
	luaL_register(L, "ksa_ext", ksa_ext);
	return 1;
}
}
