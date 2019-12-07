#include <memory>
#include <cmath>
#define PI 3.141592653589793f

typedef struct {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
} PIXEL_BGRA;

// 透明グラデーション
class Trsgrad {
public:
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
	float calc_grad(float x, float y) {
		float d = sx * ( x - cx ) + sy * ( y - cy );
		if ( d < -0.5f ) {
			return a0;
		} else if ( 0.5f < d ) {
			return a1;
		} else {
			return (a_int+a_cef*d);
		}
	}
};
static int
ksa_trsgrad(lua_State *L)
{
	// 引数受け取り
	int i=0;
	PIXEL_BGRA *data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	int w = lua_tointeger(L, ++i);
	int h = lua_tointeger(L, ++i);
	std::unique_ptr<Trsgrad> p(new Trsgrad);
	p->cx = static_cast<float>(lua_tonumber(L, ++i));
	p->cy = static_cast<float>(lua_tonumber(L, ++i));
	float angle = static_cast<float>(lua_tonumber(L, ++i));
	float gwidth = static_cast<float>(lua_tonumber(L, ++i));
	p->a0 = static_cast<float>(lua_tonumber(L, ++i));
	p->a1 = static_cast<float>(lua_tonumber(L, ++i));
	
	// パラメータ計算
	p->sx = -std::sin(angle)/gwidth;
	p->sy = std::cos(angle)/gwidth;
	p->a_cef = (p->a1)-(p->a0);
	p->a_int = ((p->a0)+(p->a1))*0.5f;
	
	// グラデーション反映
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			data->a = static_cast<unsigned char>( data->a * p->calc_grad(static_cast<float>(x), static_cast<float>(y)) );
			data++;
		}
	}
	
	return 0;
}

// クリッピング & Lanczos3 拡大縮小
class ClipResize {
private:
	class XY {
	private:
		static float sinc(float x) {
			if ( x == 0.0f ) {
				return 1.0f;
			} else {
				return std::sin(x)/(x);
			}
		}
		static float lanczos3(float x) {
			return sinc(PI*x)*sinc((PI/3.0f)*x);
		}
		static int gcd(int a, int b) {
			if ( a < b ) {
				b = b%a;
			}
			while ( b != 0 ) {
				a = a%b;
				if ( a == 0 ) {
					return b;
				}
				b = b%a;
			}
			return a;
		}
	public:
		int src_size, dest_size, clip_start, clip_end;
		bool extend;
		float reversed_scale, correction, weight_scale;
		int var;
		std::unique_ptr<std::unique_ptr<float[]>[]> weights;
		int start, end;
		float center;
		int skipped;
		void calc_range(float dest) {
			center = dest*reversed_scale + correction;
			if ( extend ) {
				start = static_cast<int>( std::ceil( center - 3.0f ) );
				end = static_cast<int>( std::floor( center + 3.0f ) );
			} else {
				start = static_cast<int>( std::ceil( center - 3.0f*reversed_scale ) );
				end = static_cast<int>( std::floor( center + 3.0f*reversed_scale ) );
			}
			skipped = 0;
			if ( start < clip_start ) {
				skipped = clip_start - start;
				start = clip_start;
			}
			if ( src_size - clip_end - 1 < end ) {
				end = src_size - clip_end - 1;
			}
		}
		void calc_params() {
			reversed_scale = static_cast<float>(src_size-clip_start-clip_end)/static_cast<float>(dest_size);
			extend = ( reversed_scale <= 1.0f );
			correction = 0.5f*reversed_scale - 0.5f + static_cast<float>(clip_start);
			weight_scale = extend ? 1.0f : 1.0f/reversed_scale;
		}
		void set_weights() {
			var = (dest_size)/gcd(dest_size, src_size);
			weights.reset(new std::unique_ptr<float[]>[var]);
			for ( int i=0; i<var; i++ ) {
				float c = static_cast<float>(i)*reversed_scale + correction;
				int s, e;
				if ( extend ) {
					s = static_cast<int>( std::ceil(c-3.0f) );
					e = static_cast<int>( std::floor(c+3.0f) );
				} else {
					s = static_cast<int>( std::ceil(c-3.0f*reversed_scale) );
					e = static_cast<int>( std::floor(c+3.0f*reversed_scale) );
				}
				weights[i].reset(new float[e-s+1]);
				for ( int sxy = s; sxy <= e; sxy++ ) {
					weights[i][sxy-s] = lanczos3( (static_cast<float>(sxy)-c)*weight_scale );
				}
			}
		}
	};
	static unsigned char uc_cast(float x) {
		if ( x < 0.0f || std::isnan(x) ) {
			return static_cast<unsigned char>(0);
		} else if ( 255.0f < x ) {
			return static_cast<unsigned char>(255);
		} else {
			return static_cast<unsigned char>(std::round(x));
		}
	}
public:
	PIXEL_BGRA *src, *dest;
	std::unique_ptr<XY> x, y;
	ClipResize() {
		x.reset(new XY());
		y.reset(new XY());
	}
	void interpolate(int dx, int dy) {
		x->calc_range(static_cast<float>(dx));
		y->calc_range(static_cast<float>(dy));
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		float *wxs = x->weights[ dx % (x->var) ].get();
		float *wys = y->weights[ dy % (y->var) ].get();
		for ( int sy=(y->start); sy<=(y->end); sy++ ) {
			float wy = wys[sy-(y->start)+(y->skipped)];
			for ( int sx=(x->start); sx<=(x->end); sx++ ) {
				float wxy = wy*wxs[sx-(x->start)+(x->skipped)];
				PIXEL_BGRA *s_px = src + ( sy*(x->src_size)+sx );
				float wxya = wxy*s_px->a;
				b += s_px->b*wxya;
				g += s_px->g*wxya;
				r += s_px->r*wxya;
				a += wxya;
				w += wxy;
			}
		}
		PIXEL_BGRA *d_px = dest + ( dy*(x->dest_size)+dx );
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/w);
	}
};
static int
ksa_clip_resize(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<ClipResize> p(new ClipResize());
	int i=0;
	p->src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->x->src_size = lua_tointeger(L, ++i);
	p->y->src_size = lua_tointeger(L, ++i);
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->x->dest_size = lua_tointeger(L, ++i);
	p->y->dest_size = lua_tointeger(L, ++i);
	p->y->clip_start = lua_tointeger(L, ++i);
	p->y->clip_end = lua_tointeger(L, ++i);
	p->x->clip_start = lua_tointeger(L, ++i);
	p->x->clip_end = lua_tointeger(L, ++i);
	
	// パラメータ，重み計算
	p->x->calc_params();
	p->y->calc_params();
	p->x->set_weights();
	p->y->set_weights();
	
	// 本処理
	for (int dy=0; dy<(p->y->dest_size); dy++) {
		for (int dx=0; dx<(p->x->dest_size); dx++) {
			p->interpolate(dx, dy);
		}
	}
	
	return 0;
}
