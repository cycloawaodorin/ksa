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
	int w = lua_tointeger(L, ++i);
	int h = lua_tointeger(L, ++i);
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

static int
ksa_clip_resize(lua_State *L)
{
	// 引数受け取り
	int i=0;
	PIXEL_BGRA *src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	int sw = lua_tointeger(L, ++i);
	int sh = lua_tointeger(L, ++i);
	PIXEL_BGRA *dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	int dw = lua_tointeger(L, ++i);
	int dh = lua_tointeger(L, ++i);
	int ct = lua_tointeger(L, ++i);
	int cb = lua_tointeger(L, ++i);
	int cl = lua_tointeger(L, ++i);
	int cr = lua_tointeger(L, ++i);
	
	// クリッピング
	src += ct*sw;
	for (int y=0; y<dh; y++) {
		src += cl;
		for (int x=0; x<dw; x++) {
			dest->b = src->b;
			dest->g = src->g;
			dest->r = src->r;
			dest->a = src->a;
			dest++; src++;
		}
		src += cr;
	}
	
	return 0;
}
