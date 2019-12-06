#include <math.h>
#define PI 3.141592653589793f

typedef struct {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
} PIXEL_BGRA;

// 透明グラデーション
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

// クリッピング & Lanczos3 拡大縮小
static float
sinc(float x)
{
	if ( x == 0.0f ) {
		return 1.0f;
	} else {
		float px = PI*x;
		return sinf(px)/(px);
	}
}
static float
lanczos3(float x)
{
	return sinc(x)*sinc(x/3.0f);
}
typedef struct {
	PIXEL_BGRA *src, *dest;
	int sw, sh, dw, dh, ct, cb, cl, cr;
	bool x_ex, y_ex;
	float rsx, rsy, cx, cy, wsx, wsy;
} CRESIZE_PARAM;
typedef struct {
	int start, end;
	float center;
} CR_RANGE;
static void
calc_range(CR_RANGE *range, float dest, bool extend, float reverse_scale, float correction, int min, int max)
{
	range->center = dest*reverse_scale + correction + static_cast<float>(min);
	if ( extend ) {
		range->start = static_cast<int>( ceilf(range->center-3.0f) );
		range->end = static_cast<int>( floorf(range->center+3.0f) );
	} else {
		range->start = static_cast<int>( ceilf((dest-3.0f)*reverse_scale+correction) ) + min;
		range->end = static_cast<int>( floorf((dest+3.0f)*reverse_scale+correction) ) + min;
	}
	if ( range->start < min ) {
		range->start = min;
	}
	if ( max < range->end ) {
		range->end = max;
	}
}
static unsigned char
uc_cast(float x)
{
	if ( x < 0.0f || isnan(x) ) {
		return static_cast<unsigned char>(0);
	} else if ( 255.0f < x ) {
		return static_cast<unsigned char>(255);
	} else {
		return static_cast<unsigned char>(roundf(x));
	}
}
static void
interpolate(CRESIZE_PARAM *p, int dx, int dy)
{
	CR_RANGE xrange, yrange;
	calc_range(&xrange, static_cast<float>(dx), p->x_ex, p->rsx, p->cx, p->cl, (p->sw)-(p->cr)-1);
	calc_range(&yrange, static_cast<float>(dy), p->y_ex, p->rsy, p->cy, p->ct, (p->sh)-(p->cb)-1);
	float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
	float wxs[xrange.end-xrange.start+1];
	for ( int sx=xrange.start; sx<=xrange.end; sx++ ) {
		wxs[sx-xrange.start] = lanczos3( (static_cast<float>(sx)-(xrange.center))*(p->wsx) );
	}
	for ( int sy=yrange.start; sy<=yrange.end; sy++ ) {
		float wy = lanczos3( (static_cast<float>(sy)-(yrange.center))*(p->wsy) );
		for ( int sx=xrange.start; sx<=xrange.end; sx++ ) {
			float wxy = wy*wxs[sx-xrange.start];
			PIXEL_BGRA *s_px = p->src + ( sy*(p->sw)+sx );
			float wxya = wxy*s_px->a;
			b += s_px->b*wxya;
			g += s_px->g*wxya;
			r += s_px->r*wxya;
			a += wxya;
			w += wxy;
		}
	}
	PIXEL_BGRA *d_px = p->dest + ( dy*(p->dw)+dx );
	d_px->b = uc_cast(b/a);
	d_px->g = uc_cast(g/a);
	d_px->r = uc_cast(r/a);
	d_px->a = uc_cast(a/w);
}

static int
ksa_clip_resize(lua_State *L)
{
	// 引数受け取り
	CRESIZE_PARAM p;
	int i=0;
	p.src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p.sw = lua_tointeger(L, ++i);
	p.sh = lua_tointeger(L, ++i);
	p.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p.dw = lua_tointeger(L, ++i);
	p.dh = lua_tointeger(L, ++i);
	p.ct = lua_tointeger(L, ++i);
	p.cb = lua_tointeger(L, ++i);
	p.cl = lua_tointeger(L, ++i);
	p.cr = lua_tointeger(L, ++i);
	
	// パラメータ計算
	p.rsx = static_cast<float>(p.sw-p.cl-p.cr)/static_cast<float>(p.dw);
	p.rsy = static_cast<float>(p.sh-p.ct-p.cb)/static_cast<float>(p.dh);
	p.x_ex = ( p.rsx <= 1.0f );
	p.y_ex = ( p.rsy <= 1.0f );
	p.cx = 0.5f*p.rsx-0.5f;
	p.cy = 0.5f*p.rsy-0.5f;
	p.wsx = p.x_ex ? 1.0f : 1.0f/p.rsx;
	p.wsy = p.y_ex ? 1.0f : 1.0f/p.rsy;
	
	// 本処理
	for (int dy=0; dy<p.dh; dy++) {
		for (int dx=0; dx<p.dw; dx++) {
			interpolate(&p, dx, dy);
		}
	}
	
	return 0;
}
