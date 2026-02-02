// 透明グラデーション
class Trsgrad {
public:
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
	int type;
	float
	calc_grad(const float &x, const float &y)
	const {
		float d = sx * ( x - cx ) + sy * ( y - cy );
		if ( d < -0.5f ) {
			return a0;
		} else if ( 0.5f < d ) {
			return a1;
		} else {
			if ( type == 1 ) {
				d = 0.5f*std::sin(PI*d);
			}
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
	const float angle = static_cast<float>(lua_tonumber(L, ++i));
	const float gwidth = static_cast<float>(lua_tonumber(L, ++i));
	p->a0 = static_cast<float>(lua_tonumber(L, ++i));
	p->a1 = static_cast<float>(lua_tonumber(L, ++i));
	p->type = lua_tointeger(L, ++i);
	
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

// 縁透明グラデーション
class Edgegrad {
private:
	constexpr float
	mag(const float &z)
	const {
		if ( type == 0 ) {
			return z;
		} else if ( type == 1 ) {
			const float omz = 1.0f - z;
			return std::sqrt(1.0f-(omz*omz));
		} else {
			return 0.0f;
		}
	}
	constexpr float
	cw(const float &cx, const float &cy)
	const {
		if ( round ) {
			return std::max(1.0f-std::hypot(1.0f-cx, 1.0f-cy), 0.0f);
		} else {
			return std::min(cx, cy);
		}
	}
	constexpr void
	set_alpha(const int &x, const int &y, const float &z)
	{
		PIXEL_BGRA *tag = data + (y*w+x);
		tag->a = static_cast<unsigned char>(static_cast<float>(tag->a)*mag(z));
	}
	void
	corner()
	{
		for (int y=0; y<t; y++) {
			const float cy = (static_cast<float>(y)+0.5f)/static_cast<float>(t);
			for (int x=0; x<l; x++) {
				const float cx = (static_cast<float>(x)+0.5f)/static_cast<float>(l);
				set_alpha(x, y, cw(cx, cy));
			}
			for (int x=w-r; x<w; x++) {
				const float cx = (static_cast<float>(w-x)-0.5f)/static_cast<float>(r);
				set_alpha(x, y, cw(cx, cy));
			}
		}
		for (int y=h-b; y<h; y++) {
			const float cy = (static_cast<float>(h-y)-0.5f)/static_cast<float>(b);
			for (int x=0; x<l; x++) {
				const float cx = (static_cast<float>(x)+0.5f)/static_cast<float>(l);
				set_alpha(x, y, cw(cx, cy));
			}
			for (int x=w-r; x<w; x++) {
				const float cx = (static_cast<float>(w-x)-0.5f)/static_cast<float>(r);
				set_alpha(x, y, cw(cx, cy));
			}
		}
	}
	void
	top()
	{
		for (int y=0; y<t; y++) {
			const float z = (static_cast<float>(y)+0.5f)/static_cast<float>(t);
			for (int x=l; x<w-r; x++) {
				set_alpha(x, y, z);
			}
		}
	}
	void
	bottom()
	{
		for (int y=h-b; y<h; y++) {
			const float z = (static_cast<float>(h-y)-0.5f)/static_cast<float>(b);
			for (int x=l; x<w-r; x++) {
				set_alpha(x, y, z);
			}
		}
	}
	void
	left()
	{
		for (int x=0; x<l; x++) {
			const float z = (static_cast<float>(x)+0.5f)/static_cast<float>(l);
			for (int y=t; y<h-b; y++) {
				set_alpha(x, y, z);
			}
		}
	}
	void
	right()
	{
		for (int x=w-r; x<w; x++) {
			const float z = (static_cast<float>(w-x)-0.5f)/static_cast<float>(r);
			for (int y=t; y<h-b; y++) {
				set_alpha(x, y, z);
			}
		}
	}
public:
	PIXEL_BGRA *data;
	int w, h, t, b, l, r, type;
	bool round;
	static void
	invoke(Edgegrad *p, int i, const int &n_th)
	{
		if ( i == 0 ) {
			p->corner();
		} else if ( i == 1 ) {
			p->top();
		} else if ( i == 2 ) {
			p->bottom();
		} else if ( i == 3 ) {
			p->left();
		} else {
			p->right();
		}
	}
};
static int
ksa_edgegrad(lua_State *L)
{
	int i=0;
	std::unique_ptr<Edgegrad> p(new Edgegrad);
	p->data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->w = lua_tointeger(L, ++i);
	p->h = lua_tointeger(L, ++i);
	p->t = lua_tointeger(L, ++i);
	p->b = lua_tointeger(L, ++i);
	p->l = lua_tointeger(L, ++i);
	p->r = lua_tointeger(L, ++i);
	p->round = (lua_tointeger(L, ++i)!=0);
	p->type = lua_tointeger(L, ++i);
	
	parallel_do(Edgegrad::invoke, p.get(), 5);
	
	return 0;
}

// クリッピング & Lanczos3 拡大縮小
class ClipResize {
private:
	class XY {
	private:
		constexpr static float
		sinc(const float &x)
		{
			if ( x == 0.0f ) {
				return 1.0f;
			} else {
				return std::sin(x)/x;
			}
		}
		constexpr static float
		lanczos3(const float &x)
		{
			return sinc(PI*x)*sinc((PI/3.0f)*x);
		}
	public:
		struct RANGE {
			int start, end, skipped;
			Rational center;
		};
		int src_size, dest_size, clip_start, clip_end, var;
		bool extend;
		Rational reversed_scale, correction, weight_scale;
		std::unique_ptr<std::unique_ptr<float[]>[]> weights;
		void
		calc_range(const int &dest, RANGE *range)
		const {
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
		set_weights(const int &start, const int &end)
		{
			for (int i=start; i<end; i++) {
				const Rational c = reversed_scale*i + correction;
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
	interpolate(const int &dx, const int &dy)
	{
		XY::RANGE xrange, yrange;
		x.calc_range(dx, &xrange);
		y.calc_range(dy, &yrange);
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		const float *wxs = x.weights[ dx % (x.var) ].get();
		const float *wys = y.weights[ dy % (y.var) ].get();
		for ( int sy=(yrange.start); sy<=(yrange.end); sy++ ) {
			const float wy = wys[sy-(yrange.start)+(yrange.skipped)];
			for ( int sx=(xrange.start); sx<=(xrange.end); sx++ ) {
				const float wxy = wy*wxs[sx-(xrange.start)+(xrange.skipped)];
				const PIXEL_BGRA *s_px = src + ( sy*(x.src_size)+sx );
				const float wxya = wxy*s_px->a;
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
	invoke_set_weights(ClipResize *p, int i, const int &n_th)
	{
		p->x.set_weights(( i*(p->x.var) )/n_th, ( (i+1)*(p->x.var) )/n_th);
		p->y.set_weights(( i*(p->y.var) )/n_th, ( (i+1)*(p->y.var) )/n_th);
	}
	static void
	invoke_interpolate(ClipResize *p, int i, const int &n_th)
	{
		const int y_start = ( i*(p->y.dest_size) )/n_th;
		const int y_end = ( (i+1)*(p->y.dest_size) )/n_th;
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
	const int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// パラメータ計算
	p->x.calc_params();
	p->y.calc_params();
	parallel_do(ClipResize::invoke_set_weights, p.get(), n_th);
	
	// 本処理
	parallel_do(ClipResize::invoke_interpolate, p.get(), n_th);
	
	return 0;
}

// クリッピング & 画素平均法 拡大縮小
class ClipResizeAve {
private:
	class XY {
	public:
		int src_size, dest_size, clip_start, clip_end, sc, dc;
		struct RANGE {
			int start, end;
		};
		constexpr void
		calc_range(const int &dest, RANGE *range)
		const {
			range->start = dest*dc;
			range->end = (dest+1)*dc;
		}
		constexpr void
		calc_params()
		{
			const int ss = src_size-clip_start-clip_end;
			const int c = std::gcd(dest_size, ss);
			sc = dest_size/c;
			dc = ss/c;
		}
	};
	void
	interpolate(const int &dx, const int &dy)
	{
		XY::RANGE xrange, yrange;
		x.calc_range(dx, &xrange);
		y.calc_range(dy, &yrange);
		std::int64_t b=0, g=0, r=0, a=0;
		for ( int sy=(yrange.start); sy<(yrange.end); sy++ ) {
			const int xs = (sy/y.sc+y.clip_start)*(x.src_size) + x.clip_start;
			for ( int sx=(xrange.start); sx<(xrange.end); sx++ ) {
				const PIXEL_BGRA *s_px = src + ( xs+(sx/x.sc) );
				const std::int64_t wa=static_cast<std::int64_t>(s_px->a);
				b += s_px->b*wa;
				g += s_px->g*wa;
				r += s_px->r*wa;
				a += wa;
			}
		}
		PIXEL_BGRA *d_px = dest + ( dy*(x.dest_size)+dx );
		d_px->b = uc_cast(b, a);
		d_px->g = uc_cast(g, a);
		d_px->r = uc_cast(r, a);
		d_px->a = uc_cast(a, w);
	}
public:
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	XY x, y;
	std::int64_t w;
	static void
	invoke_interpolate(ClipResizeAve *p, int i, const int &n_th)
	{
		const int y_start = ( i*(p->y.dest_size) )/n_th;
		const int y_end = ( (i+1)*(p->y.dest_size) )/n_th;
		for (int dy=y_start; dy<y_end; dy++) {
			for (int dx=0; dx<(p->x.dest_size); dx++) {
				p->interpolate(dx, dy);
			}
		}
	}
};
static int
ksa_clip_resize_ave(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<ClipResizeAve> p(new ClipResizeAve());
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
	const int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// パラメータ計算
	p->x.calc_params();
	p->y.calc_params();
	p->w = static_cast<std::int64_t>((p->x.dc)*(p->y.dc));
	
	// 本処理
	parallel_do(ClipResizeAve::invoke_interpolate, p.get(), n_th);
	
	return 0;
}

class DiNN {
public:
	PIXEL_BGRA *dest;
	int w, h;
	bool top;
	void
	doubling()
	{
		if ( top ) {
			for (int y=0; y<h; y+=2) {
				std::memcpy(dest+(y*w), dest+((y+1)*w), sizeof(PIXEL_BGRA)*w);
			}
		} else {
			for (int y=1; y<h; y+=2) {
				std::memcpy(dest+(y*w), dest+((y-1)*w), sizeof(PIXEL_BGRA)*w);
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
	interpolate(const int &x, const int &y)
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
			const float wy = WEIGHTS[(sy-start)>>1];
			const PIXEL_BGRA *s_px = dest+(sy*w+x);
			const float wya = wy*s_px->a;
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
	int w, h;
	bool top;
	static void
	invoke_interpolate(DiSpatial *p, int i, const int &n_th)
	{
		const int x_start = ( i*(p->w) )/n_th;
		const int x_end = ( (i+1)*(p->w) )/n_th;
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
constexpr const float DiSpatial::WEIGHTS[] = {
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
	p->w = lua_tointeger(L, ++i);
	p->h = lua_tointeger(L, ++i);
	p->top = !( lua_tointeger(L, ++i) );
	const int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// 本処理
	parallel_do(DiSpatial::invoke_interpolate, p.get(), n_th);
	
	return 0;
}

class DiTemporal {
private:
	void
	interpolate(const int &x, const int &y)
	{
		int idx = y*w+x;
		PIXEL_BGRA *px_d = dest+idx;
		const PIXEL_BGRA *px_p = past+idx, *px_f = future+idx;
		if ( px_p->a == 255 && px_f->a == 255 ) {
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_p->b)+static_cast<int>(px_f->b))>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_p->g)+static_cast<int>(px_f->g))>>1 );
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_p->r)+static_cast<int>(px_f->r))>>1 );
			px_d->a = static_cast<unsigned char>(255);
		} else {
			const float pa = px_p->a, fa = px_f->a;
			const float pafa = pa+fa;
			px_d->b = uc_cast( ( px_p->b*pa + px_f->b*fa ) / pafa );
			px_d->g = uc_cast( ( px_p->g*pa + px_f->g*fa ) / pafa );
			px_d->r = uc_cast( ( px_p->r*pa + px_f->r*fa ) / pafa );
			px_d->a = static_cast<unsigned char>( (static_cast<int>(px_p->a)+static_cast<int>(px_f->a))>>1 );
		}
	}
public:
	PIXEL_BGRA *dest;
	const PIXEL_BGRA *past, *future;
	int w, h;
	bool top;
	static void
	invoke_interpolate(DiTemporal *p, int i, const int &n_th)
	{
		const int x_start = ( i*(p->w) )/n_th;
		const int x_end = ( (i+1)*(p->w) )/n_th;
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
	const int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// 本処理
	parallel_do(DiTemporal::invoke_interpolate, p.get(), n_th);
	
	return 0;
}

class DiGhost {
private:
	void
	interpolate_spatial(PIXEL_BGRA *d, const bool &t, const int &x, const int &y)
	{
		int start=y-5, end=y+6, skip=0;
		if ( start<0 ) {
			if ( t ) {
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
			const float wy = DiSpatial::WEIGHTS[(sy-start)>>1];
			const PIXEL_BGRA *s_px = d+(sy*w+x);
			const float wya = wy*s_px->a;
			b += s_px->b*wya;
			g += s_px->g*wya;
			r += s_px->r*wya;
			a += wya;
			ww += wy;
		}
		PIXEL_BGRA *d_px = d+(y*w+x);
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/ww);
	}
	void
	interpolate_temporal(const int &x, const int &y)
	{
		const int idx = y*w+x;
		PIXEL_BGRA *px_d = past_temp+idx;
		const PIXEL_BGRA *px_f = future+idx;
		if ( px_d->a == 255 && px_f->a == 255 ) {
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_d->b)+static_cast<int>(px_f->b))>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_d->g)+static_cast<int>(px_f->g))>>1 );
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_d->r)+static_cast<int>(px_f->r))>>1 );
		} else {
			const float pa = px_d->a, fa = px_f->a;
			const float pafa = pa+fa;
			px_d->b = uc_cast( ( px_d->b*pa + px_f->b*fa ) / pafa );
			px_d->g = uc_cast( ( px_d->g*pa + px_f->g*fa ) / pafa );
			px_d->r = uc_cast( ( px_d->r*pa + px_f->r*fa ) / pafa );
			px_d->a = uc_cast( pafa*0.5f );
		}
	}
	constexpr void
	interpolate0(const int &x, const int &y)
	{
		interpolate_spatial(dest, top, x, y);
		interpolate_temporal(x, y);
	}
	constexpr void
	interpolate1(const int &x, const int &y)
	{
		interpolate_spatial(past_temp, !top, x, y);
	}
	void
	mix(const int &x, const int &y)
	{
		const int idx = y*w+x;
		PIXEL_BGRA *px_d=dest+idx, *px_t=past_temp+idx;
		if ( px_d->a == 255 && px_t->a == 255 ) {
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_d->b)+static_cast<int>(px_t->b)+1)>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_d->g)+static_cast<int>(px_t->g)+1)>>1 );
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_d->r)+static_cast<int>(px_t->r)+1)>>1 );
		} else {
			const float da = px_d->a, ta = px_t->a;
			const float data = da+ta;
			px_d->b = uc_cast( ( px_d->b*da + px_t->b*ta ) / data );
			px_d->g = uc_cast( ( px_d->g*da + px_t->g*ta ) / data );
			px_d->r = uc_cast( ( px_d->r*da + px_t->r*ta ) / data );
			px_d->a = uc_cast( data*0.5f );
		}
	}
public:
	PIXEL_BGRA *dest, *past_temp;
	const PIXEL_BGRA *future;
	int w, h;
	bool top;
	static void
	invoke_interpolate0(DiGhost *p, int i, const int &n_th)
	{
		const int x_start = ( i*(p->w) )/n_th;
		const int x_end = ( (i+1)*(p->w) )/n_th;
		if ( p->top ) {
			for (int y=0; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate0(x, y);
				}
			}
		} else {
			for (int y=1; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate0(x, y);
				}
			}
		}
	}
	static void
	invoke_interpolate1(DiGhost *p, int i, const int &n_th)
	{
		const int x_start = ( i*(p->w) )/n_th;
		const int x_end = ( (i+1)*(p->w) )/n_th;
		if ( p->top ) {
			for (int y=1; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate1(x, y);
				}
			}
		} else {
			for (int y=0; y<p->h; y+=2) {
				for (int x=x_start; x<x_end; x++) {
					p->interpolate1(x, y);
				}
			}
		}
	}
	static void
	invoke_mix(DiGhost *p, int i, const int &n_th)
	{
		const int x_start = ( i*(p->w) )/n_th;
		const int x_end = ( (i+1)*(p->w) )/n_th;
		for (int y=0; y<p->h; y++) {
			for (int x=x_start; x<x_end; x++) {
				p->mix(x, y);
			}
		}
	}
};
static int
ksa_deinterlace_ghost(lua_State *L)
{
	// 引数受け取り
	std::unique_ptr<DiGhost> p(new DiGhost());
	int i=0;
	p->dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->past_temp = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->future = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	p->w = lua_tointeger(L, ++i);
	p->h = lua_tointeger(L, ++i);
	p->top = !( lua_tointeger(L, ++i) );
	const int n_th = n_th_correction(lua_tointeger(L, ++i));
	
	// 本処理
	parallel_do(DiGhost::invoke_interpolate0, p.get(), n_th);
	parallel_do(DiGhost::invoke_interpolate1, p.get(), n_th);
	parallel_do(DiGhost::invoke_mix, p.get(), n_th);
	
	return 0;
}
