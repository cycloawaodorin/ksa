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
	int src_size, dest_size, clip_start, clip_end;
	bool extend;
	float reversed_scale, correction, weight_scale;
} CR_XY_PARAM;
typedef struct {
	PIXEL_BGRA *src, *dest;
	CR_XY_PARAM x, y;
} CRESIZE_PARAM;
typedef struct {
	int start, end;
	float center;
} CR_RANGE;
static void
calc_range(CR_RANGE *range, float dest, CR_XY_PARAM *xy)
{
	range->center = dest*(xy->reversed_scale) + (xy->correction) + static_cast<float>(xy->clip_start);
	if ( xy->extend ) {
		range->start = static_cast<int>( ceilf(range->center-3.0f) );
		range->end = static_cast<int>( floorf(range->center+3.0f) );
	} else {
		range->start = static_cast<int>( ceilf((dest-3.0f)*(xy->reversed_scale)+(xy->correction)) ) + (xy->clip_start);
		range->end = static_cast<int>( floorf((dest+3.0f)*(xy->reversed_scale)+(xy->correction)) ) + (xy->clip_start);
	}
	if ( range->start < xy->clip_start ) {
		range->start = xy->clip_start;
	}
	if ( (xy->src_size)-(xy->clip_end)-1 < range->end ) {
		range->end = (xy->src_size)-(xy->clip_end)-1;
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
	calc_range(&xrange, static_cast<float>(dx), &(p->x));
	calc_range(&yrange, static_cast<float>(dy), &(p->y));
	float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
	float wxs[xrange.end-xrange.start+1];
	for ( int sx=xrange.start; sx<=xrange.end; sx++ ) {
		wxs[sx-xrange.start] = lanczos3( (static_cast<float>(sx)-(xrange.center))*(p->x.weight_scale) );
	}
	for ( int sy=yrange.start; sy<=yrange.end; sy++ ) {
		float wy = lanczos3( (static_cast<float>(sy)-(yrange.center))*(p->y.weight_scale) );
		for ( int sx=xrange.start; sx<=xrange.end; sx++ ) {
			float wxy = wy*wxs[sx-xrange.start];
			PIXEL_BGRA *s_px = p->src + ( sy*(p->x.src_size)+sx );
			float wxya = wxy*s_px->a;
			b += s_px->b*wxya;
			g += s_px->g*wxya;
			r += s_px->r*wxya;
			a += wxya;
			w += wxy;
		}
	}
	PIXEL_BGRA *d_px = p->dest + ( dy*(p->x.dest_size)+dx );
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
	p.x.src_size = lua_tointeger(L, ++i);
	p.y.src_size = lua_tointeger(L, ++i);
	p.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p.x.dest_size = lua_tointeger(L, ++i);
	p.y.dest_size = lua_tointeger(L, ++i);
	p.y.clip_start = lua_tointeger(L, ++i);
	p.y.clip_end = lua_tointeger(L, ++i);
	p.x.clip_start = lua_tointeger(L, ++i);
	p.x.clip_end = lua_tointeger(L, ++i);
	
	// パラメータ計算
	p.x.reversed_scale = static_cast<float>(p.x.src_size-p.x.clip_start-p.x.clip_end)/static_cast<float>(p.x.dest_size);
	p.y.reversed_scale = static_cast<float>(p.y.src_size-p.y.clip_start-p.y.clip_end)/static_cast<float>(p.y.dest_size);
	p.x.extend = ( p.x.reversed_scale <= 1.0f );
	p.y.extend = ( p.y.reversed_scale <= 1.0f );
	p.x.correction = 0.5f*p.x.reversed_scale-0.5f;
	p.y.correction = 0.5f*p.y.reversed_scale-0.5f;
	p.x.weight_scale = p.x.extend ? 1.0f : 1.0f/p.x.reversed_scale;
	p.y.weight_scale = p.y.extend ? 1.0f : 1.0f/p.y.reversed_scale;
	
	// 本処理
	for (int dy=0; dy<p.y.dest_size; dy++) {
		for (int dx=0; dx<p.x.dest_size; dx++) {
			interpolate(&p, dx, dy);
		}
	}
	
	return 0;
}
