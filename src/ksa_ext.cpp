// 透明グラデーション
class Trsgrad {
	PIXEL_BGRA *data;
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
	int w, h, type;
	float
	calc_grad(float x, float y)
	const {
		constexpr static const float pi = std::numbers::pi_v<float>;
		float d = std::fma(sx, x-cx, sy*(y-cy));
		if ( d < -0.5f ) {
			return a0;
		} else if ( 0.5f < d ) {
			return a1;
		} else {
			if ( type == 1 ) {
				d = 0.5f*std::sin(pi*d);
			}
			return std::fma(a_cef, d, a_int);
		}
	}
public:
	Trsgrad(lua_State *L)
	{
		// 引数受け取り
		int i=0;
		data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		w = lua_tointeger(L, ++i);
		h = lua_tointeger(L, ++i);
		cx = static_cast<float>(lua_tonumber(L, ++i));
		cy = static_cast<float>(lua_tonumber(L, ++i));
		const auto angle = static_cast<float>(lua_tonumber(L, ++i));
		const auto gwidth = static_cast<float>(lua_tonumber(L, ++i));
		a0 = static_cast<float>(lua_tonumber(L, ++i));
		a1 = static_cast<float>(lua_tonumber(L, ++i));
		type = lua_tointeger(L, ++i);
		
		// パラメータ計算
		sx = -std::sin(angle)/gwidth;
		sy = std::cos(angle)/gwidth;
		a_cef = a1 - a0;
		a_int = (a0+a1)*0.5f;
	}
	int
	get_h()
	{
		return h;
	}
	void
	invoke_calc_grad(int y)
	{
		auto p = &data[y*w];
		auto fy = static_cast<float>(y);
		for (auto x=0; x<w; x++) {
			p->a = static_cast<unsigned char>( p->a * calc_grad(static_cast<float>(x), fy) );
			p++;
		}
	}
};
static int
ksa_trsgrad(lua_State *L)
{
	Trsgrad it(L);
	TP->parallel_do_batched([&it](int i){ it.invoke_calc_grad(i); }, it.get_h());
	return 0;
}

// 縁透明グラデーション
class Edgegrad {
	PIXEL_BGRA *data;
	int w, h, t, b, l, r, type;
	bool round;
	float
	mag(float z)
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
	float
	cw(float cx, float cy)
	const {
		if ( round ) {
			return std::max(1.0f-std::hypot(1.0f-cx, 1.0f-cy), 0.0f);
		} else {
			return std::min(cx, cy);
		}
	}
	void
	set_alpha(int x, int y, float z)
	{
		auto tag = &data[y*w+x];
		tag->a = static_cast<unsigned char>(static_cast<float>(tag->a)*mag(z));
	}
	void
	corner()
	{
		for (auto y=0; y<t; y++) {
			const float cy = (static_cast<float>(y)+0.5f)/static_cast<float>(t);
			for (auto x=0; x<l; x++) {
				const float cx = (static_cast<float>(x)+0.5f)/static_cast<float>(l);
				set_alpha(x, y, cw(cx, cy));
			}
			for (auto x=w-r; x<w; x++) {
				const float cx = (static_cast<float>(w-x)-0.5f)/static_cast<float>(r);
				set_alpha(x, y, cw(cx, cy));
			}
		}
		for (auto y=h-b; y<h; y++) {
			const float cy = (static_cast<float>(h-y)-0.5f)/static_cast<float>(b);
			for (auto x=0; x<l; x++) {
				const float cx = (static_cast<float>(x)+0.5f)/static_cast<float>(l);
				set_alpha(x, y, cw(cx, cy));
			}
			for (auto x=w-r; x<w; x++) {
				const float cx = (static_cast<float>(w-x)-0.5f)/static_cast<float>(r);
				set_alpha(x, y, cw(cx, cy));
			}
		}
	}
	void
	top()
	{
		for (auto y=0; y<t; y++) {
			const float z = (static_cast<float>(y)+0.5f)/static_cast<float>(t);
			for (auto x=l; x<w-r; x++) {
				set_alpha(x, y, z);
			}
		}
	}
	void
	bottom()
	{
		for (auto y=h-b; y<h; y++) {
			const float z = (static_cast<float>(h-y)-0.5f)/static_cast<float>(b);
			for (auto x=l; x<w-r; x++) {
				set_alpha(x, y, z);
			}
		}
	}
	void
	left()
	{
		for (auto x=0; x<l; x++) {
			const float z = (static_cast<float>(x)+0.5f)/static_cast<float>(l);
			for (auto y=t; y<h-b; y++) {
				set_alpha(x, y, z);
			}
		}
	}
	void
	right()
	{
		for (auto x=w-r; x<w; x++) {
			const float z = (static_cast<float>(w-x)-0.5f)/static_cast<float>(r);
			for (auto y=t; y<h-b; y++) {
				set_alpha(x, y, z);
			}
		}
	}
public:
	const std::function<void()> fs[5];
	Edgegrad(lua_State *L) :
		fs{ [this]{corner();}, [this]{top();}, [this]{bottom();}, [this]{left();}, [this]{right();} }
	{
		int i=0;
		data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		w = lua_tointeger(L, ++i);
		h = lua_tointeger(L, ++i);
		t = lua_tointeger(L, ++i);
		b = lua_tointeger(L, ++i);
		l = lua_tointeger(L, ++i);
		r = lua_tointeger(L, ++i);
		round = ( lua_tointeger(L, ++i) != 0 );
		type = lua_tointeger(L, ++i);
	}
};
static int
ksa_edgegrad(lua_State *L)
{
	Edgegrad it(L);
	TP->parallel_do([&it](int i){ it.fs[i](); }, 5);
	return 0;
}

// クリッピング & Lanczos3 拡大縮小
class ClipResize {
	class XY {
		struct Range {
			int start, end, skipped;
			Rational center;
		};
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
			constexpr static const float pi = std::numbers::pi_v<float>;
			constexpr static const float pi_third = pi/3.0f;
			return sinc(pi*x)*sinc(pi_third*x);
		}
	public:
		int src_size, dest_size, clip_start, clip_end, var;
		bool extend;
		Rational reversed_scale, correction, weight_scale;
		std::unique_ptr<std::unique_ptr<float[]>[]> weights;
		std::unique_ptr<Range[]> ranges;
		void
		calc_range(int xy)
		{
			auto range = &ranges[static_cast<std::size_t>(xy)];
			range->center = reversed_scale*xy + correction;
			if ( extend ) {
				range->start = static_cast<int>( range->center.floorp1() ) - 3;
				range->end = static_cast<int>( range->center.ceilm1() ) + 3;
			} else {
				range->start = static_cast<int>( ( range->center - reversed_scale*3ll ).floorp1() );
				range->end = static_cast<int>( ( range->center + reversed_scale*3ll ).ceilm1() );
			}
			range->skipped = 0;
			if ( range->start < clip_start ) {
				range->skipped = clip_start - (range->start);
				range->start = clip_start;
			}
			if ( src_size - clip_end - 1 < range->end ) {
				range->end = src_size - clip_end - 1;
			}
		}
		void
		calc_params()
		{
			ranges = std::make_unique<Range[]>(static_cast<std::size_t>(dest_size));
			reversed_scale = Rational(src_size-clip_start-clip_end, dest_size);
			extend = ( reversed_scale.get_numerator() <= reversed_scale.get_denominator() );
			correction = (reversed_scale-1ll)/2ll + clip_start;
			weight_scale = extend ? Rational(1ll) : reversed_scale.reciprocal();
			var = (dest_size)/std::gcd(dest_size, src_size-clip_start-clip_end);
			weights = std::make_unique<std::unique_ptr<float[]>[]>(var);
		}
		void
		set_weights(int i)
		{
			const Rational c = reversed_scale*i + correction;
			std::intmax_t s, e;
			if ( extend ) {
				s = c.floorp1() - 3ll;
				e = c.ceilm1() + 3ll;
			} else {
				s = ( c - reversed_scale*3ll ).floorp1();
				e = ( c + reversed_scale*3ll ).ceilm1();
			}
			auto j = static_cast<std::size_t>(i);
			weights[j] = std::make_unique<float[]>(static_cast<std::size_t>(e-s+1ll));
			for ( auto sxy = s; sxy <= e; sxy++ ) {
				weights[j][static_cast<std::size_t>(sxy-s)] = lanczos3( ((c-sxy)*weight_scale).to_float() );
			}
		}
	};
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	XY x, y;
	void
	interpolate(int dx, int dy)
	{
		const auto xrange = &(x.ranges[static_cast<std::size_t>(dx)]);
		const auto yrange = &(y.ranges[static_cast<std::size_t>(dy)]);
		FloatBGRAW bgraw;
		const auto wxs = x.weights[ static_cast<std::size_t>( dx % (x.var) ) ].get();
		const auto wys = y.weights[ static_cast<std::size_t>( dy % (y.var) ) ].get();
		for ( auto sy=(yrange->start); sy<=(yrange->end); sy++ ) {
			const auto wy = wys[sy-(yrange->start)+(yrange->skipped)];
			for ( auto sx=(xrange->start); sx<=(xrange->end); sx++ ) {
				const auto wxy = wy*wxs[sx-(xrange->start)+(xrange->skipped)];
				bgraw.fma(&src[sy*(x.src_size)+sx], wxy);
			}
		}
		bgraw.put_pixel(&dest[dy*(x.dest_size)+dx]);
	}
public:
	ClipResize(lua_State *L)
	{
		int i=0;
		src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		x.src_size = lua_tointeger(L, ++i);
		y.src_size = lua_tointeger(L, ++i);
		dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		x.dest_size = lua_tointeger(L, ++i);
		y.dest_size = lua_tointeger(L, ++i);
		y.clip_start = lua_tointeger(L, ++i);
		y.clip_end = lua_tointeger(L, ++i);
		x.clip_start = lua_tointeger(L, ++i);
		x.clip_end = lua_tointeger(L, ++i);
	}
	int get_var()
	{
		return x.var + y.var;
	}
	int get_rsize()
	{
		return x.dest_size + y.dest_size;
	}
	int get_dh()
	{
		return y.dest_size;
	}
	void
	invoke_calc_params(int i)
	{
		if ( i == 0 ) {
			x.calc_params();
		} else {
			y.calc_params();
		}
	}
	void
	invoke_set_weights(int i)
	{
		if ( i < x.var ) {
			x.set_weights(i);
		} else {
			y.set_weights(i-x.var);
		}
	}
	void
	invoke_calc_range(int i)
	{
		if ( i < x.dest_size ) {
			x.calc_range(i);
		} else {
			y.calc_range(i-x.dest_size);
		}
	}
	void
	invoke_interpolate(int dy)
	{
		for (int dx=0; dx<(x.dest_size); dx++) {
			interpolate(dx, dy);
		}
	}
};
static int
ksa_clip_resize(lua_State *L)
{
	// 引数受け取り
	ClipResize it(L);
	
	// パラメータ計算
	TP->parallel_do([&it](int i){ it.invoke_calc_params(i); }, 2);
	TP->parallel_do([&it](int i){ it.invoke_set_weights(i); }, it.get_var());
	TP->parallel_do_batched([&it](int i){ it.invoke_calc_range(i); }, it.get_rsize());
	
	// 本処理
	TP->parallel_do([&it](int i){ it.invoke_interpolate(i); }, it.get_dh());
	
	return 0;
}

// クリッピング & 画素平均法 拡大縮小
class ClipResizeAve {
	struct XY {
		int src_size, dest_size, clip_start, clip_end, sc, dc;
		void
		calc_params()
		{
			const auto ss = src_size-clip_start-clip_end;
			const auto c = std::gcd(dest_size, ss);
			sc = dest_size/c;
			dc = ss/c;
		}
	};
	struct Range {
		int start, end;
		Range(int i, int dc) : start(i*dc), end((i+1)*dc) {}
	};
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	XY x, y;
	std::uint32_t w;
	void
	interpolate(int dx, int dy)
	{
		Range xrange(dx, x.dc), yrange(dy, y.dc);
		std::uint32_t b=0u, g=0u, r=0u, a=0u;
		for ( auto sy=(yrange.start); sy<(yrange.end); sy++ ) {
			const auto xs = (sy/y.sc+y.clip_start)*(x.src_size) + x.clip_start;
			for ( auto sx=(xrange.start); sx<(xrange.end); sx++ ) {
				const auto s_px = &src[xs+(sx/x.sc)];
				const auto wa = static_cast<std::uint32_t>(s_px->a);
				b += s_px->b*wa;
				g += s_px->g*wa;
				r += s_px->r*wa;
				a += wa;
			}
		}
		auto d_px = &dest[dy*(x.dest_size)+dx];
		d_px->b = uc_cast(b, a);
		d_px->g = uc_cast(g, a);
		d_px->r = uc_cast(r, a);
		d_px->a = uc_cast(a, w);
	}
public:
	ClipResizeAve(lua_State *L)
	{
		// 引数受け取り
		int i=0;
		src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		x.src_size = lua_tointeger(L, ++i);
		y.src_size = lua_tointeger(L, ++i);
		dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		x.dest_size = lua_tointeger(L, ++i);
		y.dest_size = lua_tointeger(L, ++i);
		y.clip_start = lua_tointeger(L, ++i);
		y.clip_end = lua_tointeger(L, ++i);
		x.clip_start = lua_tointeger(L, ++i);
		x.clip_end = lua_tointeger(L, ++i);
		
		// パラメータ計算
		x.calc_params();
		y.calc_params();
		w = static_cast<std::uint32_t>( (x.dc)*(y.dc) );
	}
	int
	get_dh()
	{
		return y.dest_size;
	}
	void
	invoke_interpolate(int dy)
	{
		for (auto dx=0; dx<(x.dest_size); dx++) {
			interpolate(dx, dy);
		}
	}
};
static int
ksa_clip_resize_ave(lua_State *L)
{
	ClipResizeAve it(L);
	TP->parallel_do_batched([&it](int i){ it.invoke_interpolate(i); }, it.get_dh());
	return 0;
}

class DiNN {
	PIXEL_BGRA *dest;
	int w, h;
	bool top;
public:
	DiNN(lua_State *L)
	{
		int i=0;
		dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		w = lua_tointeger(L, ++i);
		h = lua_tointeger(L, ++i);
		top = ( lua_tointeger(L, ++i) != 0 );
	}
	int
	get_hh()
	{
		return h/2;
	}
	void
	doubling(int i)
	{
		const auto e=static_cast<std::size_t>(i*2*w);
		const auto o=static_cast<std::size_t>((i*2+1)*w);
		const auto len=static_cast<std::size_t>(w)*sizeof(PIXEL_BGRA);
		if ( top ) {
			std::memcpy(&dest[e], &dest[o], len);
		} else {
			std::memcpy(&dest[o], &dest[e], len);
		}
	}
};
static int
ksa_deinterlace_nn(lua_State *L)
{
	DiNN it(L);
	TP->parallel_do_batched([&it](int i){ it.doubling(i); }, it.get_hh());
	return 0;
}

class DiSpatial {
	PIXEL_BGRA *dest;
	int w, h;
	bool top;
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
		FloatBGRAW bgraw;
		for (auto sy=start+skip; sy<end; sy+=2) {
			const auto wy = WEIGHTS[(sy-start)>>1];
			bgraw.fma(&dest[sy*w+x], wy);
		}
		bgraw.put_pixel(&dest[y*w+x]);
	}
public:
	constexpr static const float WEIGHTS[] = {
		0.024456521739130432f, -0.1358695652173913f, 0.6114130434782609f,
		0.6114130434782609f, -0.1358695652173913f, 0.024456521739130432f
	};
	DiSpatial(lua_State *L)
	{
		int i=0;
		dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		w = lua_tointeger(L, ++i);
		h = lua_tointeger(L, ++i);
		top = ( lua_tointeger(L, ++i) != 0 );
	}
	int
	get_w()
	{
		return w;
	}
	void
	invoke_interpolate(int x)
	{
		if ( top ) {
			for (auto y=0; y<h; y+=2) {
				interpolate(x, y);
			}
		} else {
			for (auto y=1; y<h; y+=2) {
				interpolate(x, y);
			}
		}
	}
};
static int
ksa_deinterlace_spatial(lua_State *L)
{
	DiSpatial it(L);
	TP->parallel_do([&it](int i){ it.invoke_interpolate(i); }, it.get_w());
	return 0;
}

class DiTemporal {
	PIXEL_BGRA *dest;
	const PIXEL_BGRA *past, *future;
	int w, h;
	bool top;
	void
	interpolate(int x, int y)
	{
		int idx = y*w+x;
		auto px_d = &dest[idx];
		const auto px_p = &past[idx], px_f = &future[idx];
		if ( px_p->a == px_f->a ) {
			px_d->b = std::midpoint(px_p->b, px_f->b);
			px_d->g = std::midpoint(px_p->g, px_f->g);
			px_d->r = std::midpoint(px_p->r, px_f->r);
			px_d->a = px_p->a;
		} else {
			const float pa = px_p->a, fa = px_f->a;
			const float pafa = pa+fa;
			px_d->b = uc_cast( std::fmaf(px_p->b, pa, px_f->b*fa) / pafa );
			px_d->g = uc_cast( std::fmaf(px_p->g, pa, px_f->g*fa) / pafa );
			px_d->r = uc_cast( std::fmaf(px_p->r, pa, px_f->r*fa) / pafa );
			px_d->a = std::midpoint(px_p->a, px_f->a);
		}
	}
public:
	DiTemporal(lua_State *L)
	{
		int i=0;
		dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		past = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		future = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		w = lua_tointeger(L, ++i);
		h = lua_tointeger(L, ++i);
		top = ( lua_tointeger(L, ++i) != 0 );
	}
	int
	get_w()
	{
		return w;
	}
	void
	invoke_interpolate(int x)
	{
		if ( top ) {
			for (auto y=0; y<h; y+=2) {
				interpolate(x, y);
			}
		} else {
			for (auto y=1; y<h; y+=2) {
				interpolate(x, y);
			}
		}
	}
};
static int
ksa_deinterlace_temporal(lua_State *L)
{
	DiTemporal it(L);
	TP->parallel_do_batched([&it](int i){ it.invoke_interpolate(i); }, it.get_w());
	return 0;
}

class DiGhost {
	PIXEL_BGRA *dest, *past_temp;
	const PIXEL_BGRA *future;
	int w, h;
	bool top;
	void
	interpolate_spatial(PIXEL_BGRA *d, bool t, int x, int y)
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
		FloatBGRAW bgraw;
		for (auto sy=start+skip; sy<end; sy+=2) {
			const auto wy = DiSpatial::WEIGHTS[(sy-start)>>1];
			bgraw.fma(&d[sy*w+x], wy);
		}
		bgraw.put_pixel(&d[y*w+x]);
	}
	void
	interpolate_temporal(int x, int y)
	{
		const int idx = y*w+x;
		PIXEL_BGRA *px_d = past_temp+idx;
		const PIXEL_BGRA *px_f = future+idx;
		if ( px_d->a == px_f->a ) {
			px_d->b = std::midpoint(px_d->b, px_f->b);
			px_d->g = std::midpoint(px_d->g, px_f->g);
			px_d->r = std::midpoint(px_d->r, px_f->r);
		} else {
			const float pa = px_d->a, fa = px_f->a;
			const float pafa = pa+fa;
			px_d->b = uc_cast( std::fmaf(px_d->b, pa, px_f->b*fa) / pafa );
			px_d->g = uc_cast( std::fmaf(px_d->g, pa, px_f->g*fa) / pafa );
			px_d->r = uc_cast( std::fmaf(px_d->r, pa, px_f->r*fa) / pafa );
			px_d->a = std::midpoint(px_d->a, px_f->a);
		}
	}
	void
	interpolate0(int x, int y)
	{
		interpolate_spatial(dest, top, x, y);
		interpolate_temporal(x, y);
	}
	void
	interpolate1(int x, int y)
	{
		interpolate_spatial(past_temp, !top, x, y);
	}
	void
	mix(int x, int y)
	{
		const int idx = y*w+x;
		PIXEL_BGRA *px_d=dest+idx, *px_t=past_temp+idx;
		if ( px_d->a == px_t->a ) {
			px_d->b = std::midpoint(px_d->b, px_t->b);
			px_d->g = std::midpoint(px_d->g, px_t->g);
			px_d->r = std::midpoint(px_d->r, px_t->r);
		} else {
			const float da = px_d->a, ta = px_t->a;
			const float data = da+ta;
			px_d->b = uc_cast( std::fmaf(px_d->b, da, px_t->b*ta) / data );
			px_d->g = uc_cast( std::fmaf(px_d->g, da, px_t->g*ta) / data );
			px_d->r = uc_cast( std::fmaf(px_d->r, da, px_t->r*ta) / data );
			px_d->a = std::midpoint(px_d->a, px_t->a);
		}
	}
public:
	DiGhost(lua_State *L)
	{
		int i=0;
		dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		past_temp = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		future = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
		w = lua_tointeger(L, ++i);
		h = lua_tointeger(L, ++i);
		top = ( lua_tointeger(L, ++i) != 0 );
	}
	int
	get_w()
	{
		return w;
	}
	void
	invoke_interpolate0(int x)
	{
		if ( top ) {
			for (auto y=0; y<h; y+=2) {
				interpolate0(x, y);
			}
		} else {
			for (auto y=1; y<h; y+=2) {
				interpolate0(x, y);
			}
		}
	}
	void
	invoke_interpolate1(int x)
	{
		if ( top ) {
			for (auto y=1; y<h; y+=2) {
				interpolate1(x, y);
			}
		} else {
			for (auto y=0; y<h; y+=2) {
				interpolate1(x, y);
			}
		}
	}
	void
	invoke_mix(int x)
	{
		for (auto y=0; y<h; y++) {
			mix(x, y);
		}
	}
};
static int
ksa_deinterlace_ghost(lua_State *L)
{
	DiGhost it(L);
	
	const int w = it.get_w();
	TP->parallel_do([&it](int i){ it.invoke_interpolate0(i); }, w);
	TP->parallel_do([&it](int i){ it.invoke_interpolate1(i); }, w);
	TP->parallel_do_batched([&it](int i){ it.invoke_mix(i); }, w);
	
	return 0;
}
