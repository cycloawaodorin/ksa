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
		float d = this->sx * ( x - this->cx ) + this->sy * ( y - this->cy );
		if ( d < -0.5f ) {
			return this->a0;
		} else if ( 0.5f < d ) {
			return this->a1;
		} else {
			return (this->a_int+this->a_cef*d);
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
				float px = PI*x;
				return std::sin(px)/(px);
			}
		}
		static float lanczos3(float x) {
			return sinc(x)*sinc(x/3.0f);
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
		std::unique_ptr<std::unique_ptr<float[]>[]>weights;
		void set_weights() {
			this->var = (this->dest_size)/gcd(this->dest_size, this->src_size);
			this->weights.reset(new std::unique_ptr<float[]>[this->var]);
			for ( int i=0; i<(this->var); i++ ) {
				float center = static_cast<float>(i)*(this->reversed_scale) + (this->correction);
				int start, end;
				if ( this->extend ) {
					start = static_cast<int>( std::ceil(center-3.0f) );
					end = static_cast<int>( std::floor(center+3.0f) );
				} else {
					start = static_cast<int>( std::ceil((static_cast<float>(i)-3.0f)*(this->reversed_scale)+(this->correction)) );
					end = static_cast<int>( std::floor((static_cast<float>(i)+3.0f)*(this->reversed_scale)+(this->correction)) );
				}
				this->weights[i].reset(new float[end-start+1]);
				for ( int sxy = start; sxy <= end; sxy++ ) {
					this->weights[i][sxy-start] = lanczos3( (static_cast<float>(sxy)-center)*(this->weight_scale) );
				}
			}
		}
	};
	class Range {
	public:
		int start, end;
		float center;
		int skipped;
		Range(float dest, XY *xy) {
			this->center = dest*(xy->reversed_scale) + (xy->correction) + static_cast<float>(xy->clip_start);
			if ( xy->extend ) {
				this->start = static_cast<int>( std::ceil(this->center-3.0f) );
				this->end = static_cast<int>( std::floor(this->center+3.0f) );
			} else {
				this->start = static_cast<int>( std::ceil((dest-3.0f)*(xy->reversed_scale)+(xy->correction)) ) + (xy->clip_start);
				this->end = static_cast<int>( std::floor((dest+3.0f)*(xy->reversed_scale)+(xy->correction)) ) + (xy->clip_start);
			}
			this->skipped = 0;
			if ( this->start < xy->clip_start ) {
				this->start = xy->clip_start;
				this->skipped = (xy->clip_start)-(this->start);
			}
			if ( (xy->src_size)-(xy->clip_end)-1 < this->end ) {
				this->end = (xy->src_size)-(xy->clip_end)-1;
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
		x.reset(new XY);
		y.reset(new XY);
	}
	void interpolate(int dx, int dy) {
		std::unique_ptr<Range> xrange(new Range(static_cast<float>(dx), this->x.get()));
		std::unique_ptr<Range> yrange(new Range(static_cast<float>(dy), this->y.get()));
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		float *wxs = this->x->weights[ dx % (this->x->var) ].get();
		float *wys = this->y->weights[ dy % (this->y->var) ].get();
		for ( int sy=(yrange->start); sy<=(yrange->end); sy++ ) {
			float wy = wys[sy-(yrange->start)+(yrange->skipped)];
			for ( int sx=(xrange->start); sx<=(xrange->end); sx++ ) {
				float wxy = wy*wxs[sx-(xrange->start)+(xrange->skipped)];
				PIXEL_BGRA *s_px = this->src + ( sy*(this->x->src_size)+sx );
				float wxya = wxy*s_px->a;
				b += s_px->b*wxya;
				g += s_px->g*wxya;
				r += s_px->r*wxya;
				a += wxya;
				w += wxy;
			}
		}
		PIXEL_BGRA *d_px = this->dest + ( dy*(this->x->dest_size)+dx );
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
	
	// パラメータ計算
	p->x->reversed_scale = static_cast<float>((p->x->src_size)-(p->x->clip_start)-(p->x->clip_end))/static_cast<float>(p->x->dest_size);
	p->y->reversed_scale = static_cast<float>((p->y->src_size)-(p->y->clip_start)-(p->y->clip_end))/static_cast<float>(p->y->dest_size);
	p->x->extend = ( p->x->reversed_scale <= 1.0f );
	p->y->extend = ( p->y->reversed_scale <= 1.0f );
	p->x->correction = 0.5f*(p->x->reversed_scale)-0.5f;
	p->y->correction = 0.5f*(p->y->reversed_scale)-0.5f;
	p->x->weight_scale = p->x->extend ? 1.0f : 1.0f/(p->x->reversed_scale);
	p->y->weight_scale = p->y->extend ? 1.0f : 1.0f/(p->y->reversed_scale);
	
	// 重みの計算
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
