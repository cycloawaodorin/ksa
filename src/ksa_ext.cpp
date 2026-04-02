// 透明グラデーション
class Trsgrad {
	float
	calc_grad(float x, float y)
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
public:
	PIXEL_BGRA *data;
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
	int w, h, type;
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
	// 引数受け取り
	Trsgrad it;
	int i=0;
	it.data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.w = lua_tointeger(L, ++i);
	it.h = lua_tointeger(L, ++i);
	it.cx = static_cast<float>(lua_tonumber(L, ++i));
	it.cy = static_cast<float>(lua_tonumber(L, ++i));
	const auto angle = static_cast<float>(lua_tonumber(L, ++i));
	const auto gwidth = static_cast<float>(lua_tonumber(L, ++i));
	it.a0 = static_cast<float>(lua_tonumber(L, ++i));
	it.a1 = static_cast<float>(lua_tonumber(L, ++i));
	it.type = lua_tointeger(L, ++i);
	
	// パラメータ計算
	it.sx = -std::sin(angle)/gwidth;
	it.sy = std::cos(angle)/gwidth;
	it.a_cef = (it.a1)-(it.a0);
	it.a_int = ((it.a0)+(it.a1))*0.5f;
	
	// グラデーション反映
	TP->parallel_do_batched([&it](int j){ it.invoke_calc_grad(j); }, it.h);
	
	return 0;
}

// 縁透明グラデーション
class Edgegrad {
private:
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
	PIXEL_BGRA *data;
	int w, h, t, b, l, r, type;
	bool round;
	void
	invoke(int i)
	{
		if ( i == 0 ) {
			corner();
		} else if ( i == 1 ) {
			top();
		} else if ( i == 2 ) {
			bottom();
		} else if ( i == 3 ) {
			left();
		} else {
			right();
		}
	}
};
static int
ksa_edgegrad(lua_State *L)
{
	int i=0;
	Edgegrad it;
	it.data = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.w = lua_tointeger(L, ++i);
	it.h = lua_tointeger(L, ++i);
	it.t = lua_tointeger(L, ++i);
	it.b = lua_tointeger(L, ++i);
	it.l = lua_tointeger(L, ++i);
	it.r = lua_tointeger(L, ++i);
	it.round = ( lua_tointeger(L, ++i) != 0 );
	it.type = lua_tointeger(L, ++i);
	
	TP->parallel_do([&it](int j){ it.invoke(j); }, 5);
	
	return 0;
}

// クリッピング & Lanczos3 拡大縮小
class ClipResize {
private:
	class XY {
	private:
		struct RANGE {
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
			return sinc(PI*x)*sinc((PI/3.0f)*x);
		}
	public:
		int src_size, dest_size, clip_start, clip_end, var;
		bool extend;
		Rational reversed_scale, correction, weight_scale;
		std::unique_ptr<std::unique_ptr<float[]>[]> weights;
		std::unique_ptr<RANGE[]> ranges;
		void
		alloc_range()
		{
			ranges = std::make_unique<RANGE[]>(static_cast<std::size_t>(dest_size));
		}
		void
		calc_range(int xy)
		{
			auto range = &ranges[static_cast<std::size_t>(xy)];
			range->center = reversed_scale*xy + correction;
			if ( extend ) {
				range->start = static_cast<int>( range->center.ceil_eps() ) - 3;
				range->end = static_cast<int>( range->center.floor_eps() ) + 3;
			} else {
				range->start = static_cast<int>( ( range->center - reversed_scale*3ll ).ceil_eps() );
				range->end = static_cast<int>( ( range->center + reversed_scale*3ll ).floor_eps() );
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
				s = c.ceil_eps() - 3ll;
				e = c.floor_eps() + 3ll;
			} else {
				s = ( c - reversed_scale*3ll ).ceil_eps();
				e = ( c + reversed_scale*3ll ).floor_eps();
			}
			auto j = static_cast<std::size_t>(i);
			weights[j] = std::make_unique<float[]>(static_cast<std::size_t>(e-s+1ll));
			for ( auto sxy = s; sxy <= e; sxy++ ) {
				weights[j][static_cast<std::size_t>(sxy-s)] = lanczos3( ((c-sxy)*weight_scale).to_float() );
			}
		}
	};
	void
	interpolate(int dx, int dy)
	{
		const auto xrange = &(x.ranges[static_cast<std::size_t>(dx)]);
		const auto yrange = &(y.ranges[static_cast<std::size_t>(dy)]);
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, w=0.0f;
		const auto wxs = x.weights[ static_cast<std::size_t>( dx % (x.var) ) ].get();
		const auto wys = y.weights[ static_cast<std::size_t>( dy % (y.var) ) ].get();
		for ( auto sy=(yrange->start); sy<=(yrange->end); sy++ ) {
			const auto wy = wys[sy-(yrange->start)+(yrange->skipped)];
			for ( auto sx=(xrange->start); sx<=(xrange->end); sx++ ) {
				const auto wxy = wy*wxs[sx-(xrange->start)+(xrange->skipped)];
				const auto s_px = &src[sy*(x.src_size)+sx];
				const auto wxya = wxy*s_px->a;
				b += s_px->b*wxya;
				g += s_px->g*wxya;
				r += s_px->r*wxya;
				a += wxya;
				w += wxy;
			}
		}
		auto d_px = &dest[dy*(x.dest_size)+dx];
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/w);
	}
public:
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	XY x, y;
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
	ClipResize it;
	int i=0;
	it.src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.x.src_size = lua_tointeger(L, ++i);
	it.y.src_size = lua_tointeger(L, ++i);
	it.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.x.dest_size = lua_tointeger(L, ++i);
	it.x.alloc_range();
	it.y.dest_size = lua_tointeger(L, ++i);
	it.y.alloc_range();
	it.y.clip_start = lua_tointeger(L, ++i);
	it.y.clip_end = lua_tointeger(L, ++i);
	it.x.clip_start = lua_tointeger(L, ++i);
	it.x.clip_end = lua_tointeger(L, ++i);
	
	// パラメータ計算
	it.x.calc_params();
	it.y.calc_params();
	TP->parallel_do([&it](int j){ it.invoke_set_weights(j); }, it.x.var + it.y.var);
	TP->parallel_do_batched([&it](int j){ it.invoke_calc_range(j); }, it.x.dest_size + it.y.dest_size);
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate(j); }, it.y.dest_size);
	
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
		void
		calc_range(int xy, RANGE *range)
		const {
			range->start = xy*dc;
			range->end = (xy+1)*dc;
		}
		void
		calc_params()
		{
			const int ss = src_size-clip_start-clip_end;
			const int c = std::gcd(dest_size, ss);
			sc = dest_size/c;
			dc = ss/c;
		}
	};
	void
	interpolate(int dx, int dy)
	{
		XY::RANGE xrange, yrange;
		x.calc_range(dx, &xrange);
		y.calc_range(dy, &yrange);
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
	const PIXEL_BGRA *src;
	PIXEL_BGRA *dest;
	XY x, y;
	std::uint32_t w;
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
	// 引数受け取り
	ClipResizeAve it;
	int i=0;
	it.src = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.x.src_size = lua_tointeger(L, ++i);
	it.y.src_size = lua_tointeger(L, ++i);
	it.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.x.dest_size = lua_tointeger(L, ++i);
	it.y.dest_size = lua_tointeger(L, ++i);
	it.y.clip_start = lua_tointeger(L, ++i);
	it.y.clip_end = lua_tointeger(L, ++i);
	it.x.clip_start = lua_tointeger(L, ++i);
	it.x.clip_end = lua_tointeger(L, ++i);
	
	// パラメータ計算
	it.x.calc_params();
	it.y.calc_params();
	it.w = static_cast<std::uint32_t>( (it.x.dc)*(it.y.dc) );
	
	// 本処理
	TP->parallel_do_batched([&it](int j){ it.invoke_interpolate(j); }, it.y.dest_size);
	
	return 0;
}

class DiNN {
public:
	PIXEL_BGRA *dest;
	int w, h;
	bool top;
	void
	doubling(int i)
	{
		auto e=static_cast<std::size_t>(i*2*w);
		auto o=static_cast<std::size_t>((i*2+1)*w);
		auto len=static_cast<std::size_t>(w)*sizeof(PIXEL_BGRA);
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
	// 引数受け取り
	DiNN it;
	int i=0;
	it.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.w = lua_tointeger(L, ++i);
	it.h = lua_tointeger(L, ++i);
	it.top = !( lua_tointeger(L, ++i) );
	
	// 本処理
	TP->parallel_do_batched([&it](int j){ it.doubling(j); }, it.h/2);
	
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
		for (auto sy=start+skip; sy<end; sy+=2) {
			const auto wy = WEIGHTS[(sy-start)>>1];
			const auto s_px = &dest[sy*w+x];
			const auto wya = wy*s_px->a;
			b += s_px->b*wya;
			g += s_px->g*wya;
			r += s_px->r*wya;
			a += wya;
			ww += wy;
		}
		auto d_px = &dest[y*w+x];
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/ww);
	}
public:
	constexpr static const float WEIGHTS[] = {
		0.024456521739130432f, -0.1358695652173913f, 0.6114130434782609f,
		0.6114130434782609f, -0.1358695652173913f, 0.024456521739130432f
	};
	PIXEL_BGRA *dest;
	int w, h;
	bool top;
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
	// 引数受け取り
	DiSpatial it;
	int i=0;
	it.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.w = lua_tointeger(L, ++i);
	it.h = lua_tointeger(L, ++i);
	it.top = !( lua_tointeger(L, ++i) );
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate(j); }, it.w);
	
	return 0;
}

class DiTemporal {
private:
	void
	interpolate(int x, int y)
	{
		int idx = y*w+x;
		auto px_d = &dest[idx];
		const auto px_p = &past[idx], px_f = &future[idx];
		if ( px_p->a == 255u && px_f->a == 255u ) {
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_p->b)+static_cast<int>(px_f->b))>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_p->g)+static_cast<int>(px_f->g))>>1 );
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_p->r)+static_cast<int>(px_f->r))>>1 );
			px_d->a = static_cast<unsigned char>(255u);
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
	// 引数受け取り
	DiTemporal it;
	int i=0;
	it.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.past = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.future = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.w = lua_tointeger(L, ++i);
	it.h = lua_tointeger(L, ++i);
	it.top = ( lua_tointeger(L, ++i) != 0 );
	
	// 本処理
	TP->parallel_do_batched([&it](int j){ it.invoke_interpolate(j); }, it.w);
	
	return 0;
}

class DiGhost {
private:
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
		float b=0.0f, g=0.0f, r=0.0f, a=0.0f, ww=0.0f;
		for (auto sy=start+skip; sy<end; sy+=2) {
			const auto wy = DiSpatial::WEIGHTS[(sy-start)>>1];
			const auto s_px = &d[sy*w+x];
			const auto wya = wy*s_px->a;
			b += s_px->b*wya;
			g += s_px->g*wya;
			r += s_px->r*wya;
			a += wya;
			ww += wy;
		}
		auto d_px = &d[y*w+x];
		d_px->b = uc_cast(b/a);
		d_px->g = uc_cast(g/a);
		d_px->r = uc_cast(r/a);
		d_px->a = uc_cast(a/ww);
	}
	void
	interpolate_temporal(int x, int y)
	{
		const int idx = y*w+x;
		PIXEL_BGRA *px_d = past_temp+idx;
		const PIXEL_BGRA *px_f = future+idx;
		if ( px_d->a == 255u && px_f->a == 255u ) {
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
	interpolate0(int x, int y)
	{
		interpolate_spatial(dest, top, x, y);
		interpolate_temporal(x, y);
	}
	constexpr void
	interpolate1(int x, int y)
	{
		interpolate_spatial(past_temp, !top, x, y);
	}
	void
	mix(int x, int y)
	{
		const int idx = y*w+x;
		PIXEL_BGRA *px_d=dest+idx, *px_t=past_temp+idx;
		if ( px_d->a == 255u && px_t->a == 255u ) {
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
	// 引数受け取り
	DiGhost it;
	int i=0;
	it.dest = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.past_temp = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.future = static_cast<PIXEL_BGRA *>(lua_touserdata(L, ++i));
	it.w = lua_tointeger(L, ++i);
	it.h = lua_tointeger(L, ++i);
	it.top = ( lua_tointeger(L, ++i) != 0 );
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate0(j); }, it.w);
	TP->parallel_do([&it](int j){ it.invoke_interpolate1(j); }, it.w);
	TP->parallel_do_batched([&it](int j){ it.invoke_mix(j); }, it.w);
	
	return 0;
}
