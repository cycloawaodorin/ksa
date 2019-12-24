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
	public:
		using RANGE = struct {
			int start, end, skipped;
			Rational center;
		};
		int src_size, dest_size, clip_start, clip_end, var;
		bool extend;
		Rational reversed_scale, correction, weight_scale;
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
			var = (dest_size)/std::gcd(dest_size, src_size-clip_start-clip_end);
			weights.reset(new std::unique_ptr<float[]>[var]);
		}
		void
		set_weights(int start, int end)
		{
			for (int i=start; i<end; i++) {
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
		x.calc_range(dx, &xrange);
		y.calc_range(dy, &yrange);
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		const float *wxs = x.weights[ dx % (x.var) ].get();
		const float *wys = y.weights[ dy % (y.var) ].get();
		for ( int sy=(yrange.start); sy<=(yrange.end); sy++ ) {
			float wy = wys[sy-(yrange.start)+(yrange.skipped)];
			for ( int sx=(xrange.start); sx<=(xrange.end); sx++ ) {
				float wxy = wy*wxs[sx-(xrange.start)+(xrange.skipped)];
				const PIXEL_BGRA *s_px = src + ( sy*(x.src_size)+sx );
				float wxya = wxy*s_px->a;
				b += s_px->b*wxya;
				g += s_px->g*wxya;
				r += s_px->r*wxya;
				a += wxya;
				w += wxy;
			}
		}
		PIXEL_BGRA *d_px = dest + ( dy*(x.dest_size)+dx );
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/w);
	}
public:
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	XY x, y;
	static void
	invoke_set_weights(ClipResize *p, int i, int n_th)
	{
		p->x.set_weights(( i*(p->x.var) )/n_th, ( (i+1)*(p->x.var) )/n_th);
		p->y.set_weights(( i*(p->y.var) )/n_th, ( (i+1)*(p->y.var) )/n_th);
	}
	static void
	invoke_interpolate(ClipResize *p, int i, int n_th)
	{
		int y_start = ( i*(p->y.dest_size) )/n_th;
		int y_end = ( (i+1)*(p->y.dest_size) )/n_th;
		for (int dy=y_start; dy<y_end; dy++) {
			for (int dx=0; dx<(p->x.dest_size); dx++) {
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
	p->x.src_size = lua_tointeger(L, ++i);
	p->y.src_size = lua_tointeger(L, ++i);
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->x.dest_size = lua_tointeger(L, ++i);
	p->y.dest_size = lua_tointeger(L, ++i);
	p->y.clip_start = lua_tointeger(L, ++i);
	p->y.clip_end = lua_tointeger(L, ++i);
	p->x.clip_start = lua_tointeger(L, ++i);
	p->x.clip_end = lua_tointeger(L, ++i);
	int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// パラメータ計算
	p->x.calc_params();
	p->y.calc_params();
	parallel_do(ClipResize::invoke_set_weights, p.get(), n_th);
	
	// 本処理
	parallel_do(ClipResize::invoke_interpolate, p.get(), n_th);
	
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
	invoke_interpolate(ClipDouble *p, int i, int n_th)
	{
		int y_start = ( i*(p->dh) )/n_th;
		int y_end = ( (i+1)*(p->dh) )/n_th;
		for (int dy=y_start; dy<y_end; dy++) {
			for (int dx=0; dx<(p->dw); dx++) {
				p->interpolate(dx, dy);
			}
		}
	}
};
constexpr float ClipDouble::WEIGHTS_E[] = {
	0.007355926047194188f, -0.0677913359005429f, 0.27018982304623407f,
	0.8900670517104946f, -0.13287101836506404f, 0.03002109144958156f
};
constexpr float ClipDouble::WEIGHTS_O[] = {
	0.03002109144958156f, -0.13287101836506404f, 0.8900670517104946f,
	0.27018982304623407f, -0.0677913359005429f, 0.007355926047194188f
};
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
	int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// パラメータ計算
	p->dw = (p->sw - p->cl - p->cr)*2;
	p->dh = (p->sh - p->ct - p->cb)*2;
	
	// 本処理
	parallel_do(ClipDouble::invoke_interpolate, p.get(), n_th);
	
	return 0;
}

class DiNN {
public:
	PIXEL_BGRA *dest;
	const PIXEL_BGRA *src;
	int w, h;
	bool top;
	void
	doubling()
	{
		if ( top ) {
			for (int y=0; y<h; y+=2) {
				memcpy(dest+(y*w), src+((y+1)*w), sizeof(PIXEL_BGRA)*w);
			}
		} else {
			for (int y=1; y<h; y+=2) {
				memcpy(dest+(y*w), src+((y-1)*w), sizeof(PIXEL_BGRA)*w);
			}
		}
	}
};
static int
ksa_deinterlace_nn(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<DiNN> p(new DiNN());
	int i=0;
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->w = lua_tointeger(L, ++i);
	p->h = lua_tointeger(L, ++i);
	p->top = !( lua_tointeger(L, ++i) );
	
	// 本処理
	p->doubling();
	
	return 0;
}

class DiSpatial {
private:
	void
	interpolate(int x, int y)
	{
		int start=y-5, end=y+6, skip=0;
		if ( start<0 ) {
			if ( top ) {
				skip = -start+1;
			} else {
				skip = -start;
			}
		}
		if ( h<end ) {
			end = h;
		}
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, ww=0.0f;
		for (int sy=start+skip; sy<end; sy+=2) {
			float wy = WEIGHTS[(sy-start)>>1];
			const PIXEL_BGRA *s_px = src+(sy*w+x);
			float wya = wy*s_px->a;
			b += s_px->b*wya;
			g += s_px->g*wya;
			r += s_px->r*wya;
			a += wya;
			ww += wy;
		}
		PIXEL_BGRA *d_px = dest+(y*w+x);
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/ww);
	}
public:
	static const float WEIGHTS[6];
	PIXEL_BGRA *dest;
	const PIXEL_BGRA *src;
	int w, h;
	bool top;
	static void
	invoke_interpolate(DiSpatial *p, int i, int n_th)
	{
		int x_start = ( i*(p->w) )/n_th;
		int x_end = ( (i+1)*(p->w) )/n_th;
		if ( p->top ) {
			for (int y=0; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate(x, y);
				}
			}
		} else {
			for (int y=1; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate(x, y);
				}
			}
		}
	}
};
constexpr float DiSpatial::WEIGHTS[] = {
	0.024456521739130432f, -0.1358695652173913f, 0.6114130434782609f,
	0.6114130434782609f, -0.1358695652173913f, 0.024456521739130432f
};
static int
ksa_deinterlace_spatial(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<DiSpatial> p(new DiSpatial());
	int i=0;
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->w = lua_tointeger(L, ++i);
	p->h = lua_tointeger(L, ++i);
	p->top = !( lua_tointeger(L, ++i) );
	int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// 本処理
	parallel_do(DiSpatial::invoke_interpolate, p.get(), n_th);
	
	return 0;
}

class DiTemporal {
private:
	void
	interpolate(int x, int y)
	{
		int idx = y*w+x;
		PIXEL_BGRA *px_d = dest+idx;
		const PIXEL_BGRA *px_p = past+idx, *px_f = future+idx;
		if ( px_p->a == 255 && px_f->a == 255 ) {
			px_d->b = static_cast<unsigned char>( (px_p->b>>1) + (px_f->b>>1) + ((px_p->b&1)&(px_f->b&1)) );
			px_d->g = static_cast<unsigned char>( (px_p->g>>1) + (px_f->g>>1) + ((px_p->g&1)&(px_f->g&1)) );
			px_d->r = static_cast<unsigned char>( (px_p->r>>1) + (px_f->r>>1) + ((px_p->r&1)&(px_f->r&1)) );
			px_d->a = static_cast<unsigned char>(255);
		} else {
			float pa = px_p->a, fa = px_f->a;
			float pafa = pa+fa;
			px_d->b = uc_cast( ( px_p->b*pa + px_f->b*fa ) / pafa );
			px_d->g = uc_cast( ( px_p->g*pa + px_f->g*fa ) / pafa );
			px_d->r = uc_cast( ( px_p->r*pa + px_f->r*fa ) / pafa );
			px_d->a = static_cast<unsigned char>( (px_p->a>>1) + (px_f->a>>1) + ((px_p->a&1)&(px_f->a&1)) );
		}
	}
public:
	PIXEL_BGRA *dest;
	const PIXEL_BGRA *past, *future;
	int w, h;
	bool top;
	static void
	invoke_interpolate(DiTemporal *p, int i, int n_th)
	{
		int x_start = ( i*(p->w) )/n_th;
		int x_end = ( (i+1)*(p->w) )/n_th;
		if ( p->top ) {
			for (int y=0; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate(x, y);
				}
			}
		} else {
			for (int y=1; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate(x, y);
				}
			}
		}
	}
};
static int
ksa_deinterlace_temporal(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<DiTemporal> p(new DiTemporal());
	int i=0;
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->past = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->future = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->w = lua_tointeger(L, ++i);
	p->h = lua_tointeger(L, ++i);
	p->top = !( lua_tointeger(L, ++i) );
	int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// 本処理
	parallel_do(DiTemporal::invoke_interpolate, p.get(), n_th);
	
	return 0;
}
