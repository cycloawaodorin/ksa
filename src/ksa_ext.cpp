#include <memory>
#include <thread>
#include <cmath>
#include <cstdint>

namespace KSA {

constexpr float PI = 3.141592653589793f;
using PIXEL_BGRA = struct pixel_bgra {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
};
static unsigned char
uc_cast(float x)
{
	if ( x < 0.0f || std::isnan(x) ) {
		return static_cast<unsigned char>(0);
	} else if ( 255.0f < x ) {
		return static_cast<unsigned char>(255);
	} else {
		return static_cast<unsigned char>(std::round(x));
	}
}
class Rational {
private:
	std::int64_t numerator, denominator;
	static std::int64_t
	gcd(std::int64_t a, std::int64_t b)
	{
		if ( a < b ) {
			if ( a == 0 ) {
				return b;
			} else {
				b = b%a;
			}
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
	Rational(std::int64_t num, std::int64_t den)
	{
		std::int64_t c = gcd(std::abs(num), std::abs(den));
		if ( den < 0 ) {
			numerator = -num/c;
			denominator = -den/c;
		} else {
			numerator = num/c;
			denominator = den/c;
		}
	}
	Rational(std::int64_t i)
	{
		numerator = i;
		denominator = 1;
	}
	Rational()
	{
		numerator = 0;
		denominator = 1;
	}
	std::int64_t
	get_numerator()
	const {
		return numerator;
	}
	std::int64_t
	get_denominator()
	const {
		return denominator;
	}
	Rational
	operator +(const Rational &other)
	const {
		std::int64_t c = gcd(denominator, other.denominator);
		std::int64_t s_d = denominator/c, o_d = other.denominator/c;
		return Rational(numerator*o_d+other.numerator*s_d, denominator*o_d);
	}
	Rational
	operator +(std::int64_t other)
	const {
		return Rational(numerator+other*denominator, denominator);
	}
	Rational
	operator -(const Rational &other)
	const {
		std::int64_t c = gcd(denominator, other.denominator);
		std::int64_t s_d = denominator/c, o_d = other.denominator/c;
		return Rational(numerator*o_d-other.numerator*s_d, denominator*o_d);
	}
	Rational
	operator -(std::int64_t other)
	const {
		return Rational(numerator-other*denominator, denominator);
	}
	Rational
	operator *(const Rational &other)
	const {
		std::int64_t ca = gcd(std::abs(numerator), other.denominator);
		std::int64_t cb = gcd(denominator, std::abs(other.numerator));
		return Rational((numerator/ca) * (other.numerator/cb), (denominator/cb) * (other.denominator/ca));
	}
	Rational
	operator *(std::int64_t other)
	const {
		std::int64_t c = gcd(std::abs(other), denominator);
		return Rational(numerator*(other/c), denominator/c);
	}
	Rational
	operator /(const Rational &other)
	const {
		std::int64_t ca = gcd(std::abs(numerator), std::abs(other.numerator));
		std::int64_t cb = gcd(denominator, other.denominator);
		return Rational((numerator/ca) * (other.denominator/cb), (denominator/cb) * (other.numerator/ca));
	}
	Rational
	operator /(std::int64_t other)
	const {
		std::int64_t c = gcd(std::abs(numerator), std::abs(other));
		return Rational(numerator/c, denominator*(other/c));
	}
	Rational
	reciprocal()
	const {
		return Rational(denominator, numerator);
	}
	std::int64_t
	floor()
	const {
		std::int64_t r = numerator % denominator;
		if ( r < 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::int64_t
	floor_eps()
	const {
		std::int64_t r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::int64_t
	ceil()
	const {
		std::int64_t r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1 );
		}
	}
	std::int64_t
	ceil_eps()
	const {
		std::int64_t r = numerator % denominator;
		if ( r < 0 ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1 );
		}
	}
	float
	to_float()
	const {
		return( static_cast<float>(numerator) / static_cast<float>(denominator) );
	}
};

// 透明グラデーション
class Trsgrad {
public:
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
	float
	calc_grad(float x, float y)
	{
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
	const int w = lua_tointeger(L, ++i);
	const int h = lua_tointeger(L, ++i);
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
		static float
		sinc(float x)
		{
			if ( x == 0.0f ) {
				return 1.0f;
			} else {
				return std::sin(x)/x;
			}
		}
		static float
		lanczos3(float x)
		{
			return sinc(PI*x)*sinc((PI/3.0f)*x);
		}
		static int
		gcd(int a, int b)
		{
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
		using RANGE = struct {
			int start, end;
			Rational center;
			int skipped;
		};
		int src_size, dest_size, clip_start, clip_end;
		bool extend;
		Rational reversed_scale, correction, weight_scale;
		int var;
		std::unique_ptr<std::unique_ptr<float[]>[]> weights;
		void
		calc_range(int dest, RANGE *range)
		{
			range->center = reversed_scale*dest+correction;
			if ( extend ) {
				range->start = static_cast<int>( range->center.ceil_eps() ) - 3;
				range->end = static_cast<int>( range->center.floor_eps() ) + 3;
			} else {
				range->start = static_cast<int>( ( range->center - reversed_scale*3 ).ceil_eps() );
				range->end = static_cast<int>( ( range->center + reversed_scale*3 ).floor_eps() );
			}
			range->skipped = 0;
			if ( range->start < clip_start ) {
				range->skipped = clip_start - range->start;
				range->start = clip_start;
			}
			if ( src_size - clip_end - 1 < range->end ) {
				range->end = src_size - clip_end - 1;
			}
		}
		void
		calc_params()
		{
			reversed_scale = Rational(src_size-clip_start-clip_end, dest_size);
			extend = ( reversed_scale.get_numerator() <= reversed_scale.get_denominator() );
			correction = (reversed_scale-1)/2 + clip_start;
			weight_scale = extend ? Rational(1) : reversed_scale.reciprocal();
		}
		void
		set_weights()
		{
			var = (dest_size)/gcd(dest_size, src_size-clip_start-clip_end);
			weights.reset(new std::unique_ptr<float[]>[var]);
			for ( int i=0; i<var; i++ ) {
				Rational c = reversed_scale*i + correction;
				int s, e;
				if ( extend ) {
					s = static_cast<int>( c.ceil_eps() ) - 3;
					e = static_cast<int>( c.floor_eps() ) + 3;
				} else {
					s = static_cast<int>( ( c - reversed_scale*3 ).ceil_eps() );
					e = static_cast<int>( ( c + reversed_scale*3 ).floor_eps() );
				}
				weights[i].reset(new float[e-s+1]);
				for ( int sxy = s; sxy <= e; sxy++ ) {
					weights[i][sxy-s] = lanczos3( ((c-sxy)*weight_scale).to_float() );
				}
			}
		}
	};
	void
	interpolate(int dx, int dy)
	{
		XY::RANGE xrange, yrange;
		x->calc_range(dx, &xrange);
		y->calc_range(dy, &yrange);
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		const float *wxs = x->weights[ dx % (x->var) ].get();
		const float *wys = y->weights[ dy % (y->var) ].get();
		for ( int sy=(yrange.start); sy<=(yrange.end); sy++ ) {
			float wy = wys[sy-(yrange.start)+(yrange.skipped)];
			for ( int sx=(xrange.start); sx<=(xrange.end); sx++ ) {
				float wxy = wy*wxs[sx-(xrange.start)+(xrange.skipped)];
				const PIXEL_BGRA *s_px = src + ( sy*(x->src_size)+sx );
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
public:
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	std::unique_ptr<XY> x, y;
	ClipResize()
	{
		x.reset(new XY());
		y.reset(new XY());
	}
	static void
	invoke_interpolate(ClipResize *p, int y_start, int y_end)
	{
		for (int dy=y_start; dy<y_end; dy++) {
			for (int dx=0; dx<(p->x->dest_size); dx++) {
				p->interpolate(dx, dy);
			}
		}
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
	int n_th = lua_tointeger(L, ++i);
	
	// パラメータ，重み計算
	if ( n_th <= 0 ) {
		n_th += std::thread::hardware_concurrency();
		if ( n_th <= 0 ) {
			n_th = 1;
		}
	}
	p->x->calc_params();
	p->y->calc_params();
	p->x->set_weights();
	p->y->set_weights();
	
	// 本処理
	std::unique_ptr<std::unique_ptr<std::thread>[]> threads(new std::unique_ptr<std::thread>[n_th]);
	for (int t=0; t<n_th; t++) {
		threads[t].reset(new std::thread(ClipResize::invoke_interpolate, p.get(), ( t*(p->y->dest_size) )/n_th, ( (t+1)*(p->y->dest_size) )/n_th));
	}
	for (int t=0; t<n_th; t++) {
		threads[t]->join();
	}
	
	return 0;
}

// クリッピング&倍角
class ClipDouble {
private:
	using RANGE = struct {
		int start, end;
		const float *weights;
	};
	static void
	calc_range(RANGE *range, int dxy, int clip_start, int smax)
	{
		if ( dxy%2 == 0 ) {
			range->start = dxy/2 - 3 + clip_start;
			range->end = dxy/2 + 2 + clip_start;
		} else {
			range->start = dxy/2 - 2 + clip_start;
			range->end = dxy/2 + 3 + clip_start;
		}
		int skipped=0;
		if ( range->start < clip_start ) {
			skipped = clip_start - range->start;
			range->start = clip_start;
		}
		if ( dxy%2 == 0 ) {
			range->weights = WEIGHTS_E + skipped;
		} else {
			range->weights = WEIGHTS_O + skipped;
		}
		if ( smax < range->end ) {
			range->end = smax;
		}
	}
	void
	interpolate(int dx, int dy)
	{
		RANGE xrange, yrange;
		calc_range(&xrange, dx, cl, sw-cr-1);
		calc_range(&yrange, dy, ct, sh-cb-1);
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		for ( int sy=yrange.start; sy<=yrange.end; sy++ ) {
			float wy = yrange.weights[ sy - yrange.start ];
			for ( int sx=xrange.start; sx<=xrange.end; sx++ ) {
				float wxy = wy*xrange.weights[ sx - xrange.start ];
				const PIXEL_BGRA *s_px = src + ( sy*sw+sx );
				float wxya = wxy*s_px->a;
				b += s_px->b*wxya;
				g += s_px->g*wxya;
				r += s_px->r*wxya;
				a += wxya;
				w += wxy;
			}
		}
		PIXEL_BGRA *d_px = dest + ( dy*dw+dx );
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/w);
	}
public:
	static const float WEIGHTS_E[6], WEIGHTS_O[6];
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	int sw, sh, dw, dh, ct, cb, cl, cr;
	static void
	invoke_interpolate(ClipDouble *p, int y_start, int y_end)
	{
		for (int dy=y_start; dy<y_end; dy++) {
			for (int dx=0; dx<(p->dw); dx++) {
				p->interpolate(dx, dy);
			}
		}
	}
};
constexpr float ClipDouble::WEIGHTS_E[] = {0.007355926047194188f, -0.0677913359005429f, 0.27018982304623407f, 0.8900670517104946f, -0.13287101836506404f, 0.03002109144958156f};
constexpr float ClipDouble::WEIGHTS_O[] = {0.03002109144958156f, -0.13287101836506404f, 0.8900670517104946f, 0.27018982304623407f, -0.0677913359005429f, 0.007355926047194188f};
static int
ksa_clip_double(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<ClipDouble> p(new ClipDouble());
	int i=0;
	p->src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->sw = lua_tointeger(L, ++i);
	p->sh = lua_tointeger(L, ++i);
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->ct = lua_tointeger(L, ++i);
	p->cb = lua_tointeger(L, ++i);
	p->cl = lua_tointeger(L, ++i);
	p->cr = lua_tointeger(L, ++i);
	int n_th = lua_tointeger(L, ++i);
	
	// パラメータ計算
	p->dw = (p->sw - p->cl - p->cr)*2;
	p->dh = (p->sh - p->ct - p->cb)*2;
	if ( n_th <= 0 ) {
		n_th += std::thread::hardware_concurrency();
		if ( n_th <= 0 ) {
			n_th = 1;
		}
	}
	
	// 本処理
	std::unique_ptr<std::unique_ptr<std::thread>[]> threads(new std::unique_ptr<std::thread>[n_th]);
	for (int t=0; t<n_th; t++) {
		threads[t].reset(new std::thread(ClipDouble::invoke_interpolate, p.get(), ( t*(p->dh) )/n_th, ( (t+1)*(p->dh) )/n_th));
	}
	for (int t=0; t<n_th; t++) {
		threads[t]->join();
	}
	
	return 0;
}

};
