// 透明グラデーション
class Trsgrad {
private:
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
public:
	PIXEL_RGBA *data;
	float sx, sy, cx, cy, a_cef, a_int, a0, a1;
	int w, h, type;
	void
	invoke_calc_grad(int y)
	{
		auto p=&data[y*w];
		float fy = static_cast<float>(y);
		for (int x=0; x<w; x++) {
			p->a = static_cast<unsigned char>( p->a * calc_grad(static_cast<float>(x), fy) );
			p++;
		}
	}
};
static void
ksa_trsgrad(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 10) ) { return; }
	int i=0;
	Trsgrad it;
	it.data = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.w = param->get_param_int(i++);
	it.h = param->get_param_int(i++);
	it.cx = static_cast<float>(param->get_param_double(i++));
	it.cy = static_cast<float>(param->get_param_double(i++));
	const float angle = static_cast<float>(param->get_param_double(i++));
	const float gwidth = static_cast<float>(param->get_param_double(i++));
	it.a0 = static_cast<float>(param->get_param_double(i++));
	it.a1 = static_cast<float>(param->get_param_double(i++));
	it.type = param->get_param_int(i++);
	
	// パラメータ計算
	it.sx = -std::sin(angle)/gwidth;
	it.sy = std::cos(angle)/gwidth;
	it.a_cef = (it.a1)-(it.a0);
	it.a_int = ((it.a0)+(it.a1))*0.5f;
	
	// グラデーション反映
	TP->parallel_do([&it](int j){ it.invoke_calc_grad(j); }, it.h);
}

// 縁透明グラデーション
class Edgegrad {
private:
	float
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
	float
	cw(const float &cx, const float &cy)
	const {
		if ( round ) {
			return std::max(1.0f-std::hypot(1.0f-cx, 1.0f-cy), 0.0f);
		} else {
			return std::min(cx, cy);
		}
	}
	void
	set_alpha(const int &x, const int &y, const float &z)
	{
		auto tag = &data[y*w+x];
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
	PIXEL_RGBA *data;
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
static void
ksa_edgegrad(SCRIPT_MODULE_PARAM *param)
{
	if ( check_arg_num(param, 9) ) { return; }
	int i=0;
	Edgegrad it;
	it.data = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.w = param->get_param_int(i++);
	it.h = param->get_param_int(i++);
	it.t = param->get_param_int(i++);
	it.b = param->get_param_int(i++);
	it.l = param->get_param_int(i++);
	it.r = param->get_param_int(i++);
	it.round = param->get_param_boolean(i++);
	it.type = param->get_param_int(i++);
	
	TP->parallel_do([&it](int j){ it.invoke(j); }, 5);
}

// クリッピング & Lanczos3 拡大縮小
class ClipResize {
private:
	class XY {
	private:
		static float
		sinc(const float &x)
		{
			if ( x == 0.0f ) {
				return 1.0f;
			} else {
				return std::sin(x)/x;
			}
		}
		static float
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
		calc_range(const int &_dest, RANGE *range)
		const {
			range->center = reversed_scale*_dest+correction;
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
			weights = std::make_unique<std::unique_ptr<float[]>[]>(static_cast<std::size_t>(var));
		}
		void
		set_weights(const int i)
		{
			const Rational c = reversed_scale*i + correction;
			std::intmax_t s, e;
			if ( extend ) {
				s = c.ceil_eps() - 3;
				e = c.floor_eps() + 3;
			} else {
				s = ( c - reversed_scale*3 ).ceil_eps();
				e = ( c + reversed_scale*3 ).floor_eps();
			}
			auto j = static_cast<std::size_t>(i);
			weights[j] = std::make_unique<float[]>(static_cast<std::size_t>(e-s+1));
			for ( auto sxy = s; sxy <= e; sxy++ ) {
				weights[j][static_cast<std::size_t>(sxy-s)] = lanczos3( ((c-sxy)*weight_scale).to_float() );
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
		const auto *wxs = x.weights[ static_cast<std::size_t>( dx % (x.var) ) ].get();
		const auto *wys = y.weights[ static_cast<std::size_t>( dy % (y.var) ) ].get();
		for ( auto sy=(yrange.start); sy<=(yrange.end); sy++ ) {
			const auto wy = wys[sy-(yrange.start)+(yrange.skipped)];
			for ( auto sx=(xrange.start); sx<=(xrange.end); sx++ ) {
				const auto wxy = wy*wxs[sx-(xrange.start)+(xrange.skipped)];
				const PIXEL_RGBA *s_px = &src[ sy*(x.src_size)+sx ];
				const auto wxya = wxy*s_px->a;
				r += s_px->r*wxya;
				g += s_px->g*wxya;
				b += s_px->b*wxya;
				a += wxya;
				w += wxy;
			}
		}
		auto d_px = &dest[dy*(x.dest_size)+dx];
		d_px->r = uc_cast(r/a);
		d_px->g = uc_cast(g/a);
		d_px->b = uc_cast(b/a);
		d_px->a = uc_cast(a/w);
	}
public:
	const PIXEL_RGBA *src;
	PIXEL_RGBA *dest;
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
	invoke_interpolate(int dy)
	{
		for (int dx=0; dx<(x.dest_size); dx++) {
			interpolate(dx, dy);
		}
	}
};
static void
ksa_clip_resize(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 10) ) { return; }
	ClipResize it;
	int i=0;
	it.src = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.x.src_size = param->get_param_int(i++);
	it.y.src_size = param->get_param_int(i++);
	it.dest = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.x.dest_size = param->get_param_int(i++);
	it.y.dest_size = param->get_param_int(i++);
	it.y.clip_start = param->get_param_int(i++);
	it.y.clip_end = param->get_param_int(i++);
	it.x.clip_start = param->get_param_int(i++);
	it.x.clip_end = param->get_param_int(i++);
	
	// パラメータ計算
	it.x.calc_params();
	it.y.calc_params();
	TP->parallel_do([&it](int j){ it.invoke_set_weights(j); }, it.x.var + it.y.var);
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate(j); }, it.y.dest_size);
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
		calc_range(const int &_dest, RANGE *range)
		const {
			range->start = _dest*dc;
			range->end = (_dest+1)*dc;
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
	interpolate(const int &dx, const int &dy)
	{
		XY::RANGE xrange, yrange;
		x.calc_range(dx, &xrange);
		y.calc_range(dy, &yrange);
		int b=0, g=0, r=0, a=0;
		for ( auto sy=(yrange.start); sy<(yrange.end); sy++ ) {
			const auto xs = (sy/y.sc+y.clip_start)*(x.src_size) + x.clip_start;
			for ( auto sx=(xrange.start); sx<(xrange.end); sx++ ) {
				const auto s_px = &src[xs+(sx/x.sc)];
				const auto wa=static_cast<std::intmax_t>(s_px->a);
				r += s_px->r*wa;
				g += s_px->g*wa;
				b += s_px->b*wa;
				a += wa;
			}
		}
		auto d_px = &dest[dy*(x.dest_size)+dx];
		d_px->r = uc_cast(r, a);
		d_px->g = uc_cast(g, a);
		d_px->b = uc_cast(b, a);
		d_px->a = uc_cast(a, w);
	}
public:
	const PIXEL_RGBA *src;
	PIXEL_RGBA *dest;
	XY x, y;
	int w;
	void
	invoke_interpolate(int i, const int &n_th)
	{
		const int y_start = ( i*(y.dest_size) )/n_th;
		const int y_end = ( (i+1)*(y.dest_size) )/n_th;
		for (auto dy=y_start; dy<y_end; dy++) {
			for (auto dx=0; dx<(x.dest_size); dx++) {
				interpolate(dx, dy);
			}
		}
	}
};
static void
ksa_clip_resize_ave(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 10) ) { return; }
	ClipResizeAve it;
	int i=0;
	it.src = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.x.src_size = param->get_param_int(i++);
	it.y.src_size = param->get_param_int(i++);
	it.dest = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.x.dest_size = param->get_param_int(i++);
	it.y.dest_size = param->get_param_int(i++);
	it.y.clip_start = param->get_param_int(i++);
	it.y.clip_end = param->get_param_int(i++);
	it.x.clip_start = param->get_param_int(i++);
	it.x.clip_end = param->get_param_int(i++);
	
	// パラメータ計算
	it.x.calc_params();
	it.y.calc_params();
	it.w = (it.x.dc)*(it.y.dc);
	
	// 本処理
	int n = static_cast<int>(TP->get_size());
	TP->parallel_do([&it, n](int i){it.invoke_interpolate(i, n);}, n);
}

class DiNN {
public:
	PIXEL_RGBA *dest;
	int w, h;
	bool top;
	void
	doubling(int i)
	{
		auto e=static_cast<std::size_t>(i*2*w);
		auto o=static_cast<std::size_t>((i*2+1)*w);
		auto len=static_cast<std::size_t>(w)*sizeof(PIXEL_RGBA);
		if ( top ) {
			std::memcpy(&dest[e], &dest[o], len);
		} else {
			std::memcpy(&dest[o], &dest[e], len);
		}
	}
};
static void
ksa_deinterlace_nn(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 4) ) { return; }
	DiNN it;
	int i=0;
	it.dest = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.w = param->get_param_int(i++);
	it.h = param->get_param_int(i++);
	it.top = param->get_param_boolean(i++);
	
	// 本処理
	TP->parallel_do([&it](int j){ it.doubling(j); }, it.h/2);
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
			const auto s_px = &dest[sy*w+x];
			const float wya = wy*s_px->a;
			r += s_px->r*wya;
			g += s_px->g*wya;
			b += s_px->b*wya;
			a += wya;
			ww += wy;
		}
		auto d_px = &dest[y*w+x];
		d_px->r = uc_cast(r/a);
		d_px->g = uc_cast(g/a);
		d_px->b = uc_cast(b/a);
		d_px->a = uc_cast(a/ww);
	}
public:
	constexpr static const float WEIGHTS[] = {
		0.024456521739130432f, -0.1358695652173913f, 0.6114130434782609f,
		0.6114130434782609f, -0.1358695652173913f, 0.024456521739130432f
	};
	PIXEL_RGBA *dest;
	int w, h;
	bool top;
	void
	invoke_interpolate(int x)
	{
		if ( top ) {
			for (int y=0; y<h; y+=2) {
				interpolate(x, y);
			}
		} else {
			for (int y=1; y<h; y+=2) {
				interpolate(x, y);
			}
		}
	}
};

static void
ksa_deinterlace_spatial(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 4) ) { return; }
	DiSpatial it;
	int i=0;
	it.dest = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.w = param->get_param_int(i++);
	it.h = param->get_param_int(i++);
	it.top = param->get_param_boolean(i++);
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate(j); }, it.w);
}

class DiTemporal {
private:
	void
	interpolate(const int &x, const int &y)
	{
		int idx = y*w+x;
		auto px_d = &dest[idx];
		const auto px_p = &past[idx], px_f = &future[idx];
		if ( px_p->a == 255 && px_f->a == 255 ) {
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_p->r)+static_cast<int>(px_f->r))>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_p->g)+static_cast<int>(px_f->g))>>1 );
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_p->b)+static_cast<int>(px_f->b))>>1 );
			px_d->a = static_cast<unsigned char>(255);
		} else {
			const float pa = px_p->a, fa = px_f->a;
			const float pafa = pa+fa;
			px_d->r = uc_cast( ( px_p->r*pa + px_f->r*fa ) / pafa );
			px_d->g = uc_cast( ( px_p->g*pa + px_f->g*fa ) / pafa );
			px_d->b = uc_cast( ( px_p->b*pa + px_f->b*fa ) / pafa );
			px_d->a = static_cast<unsigned char>( (static_cast<int>(px_p->a)+static_cast<int>(px_f->a))>>1 );
		}
	}
public:
	PIXEL_RGBA *dest;
	const PIXEL_RGBA *past, *future;
	int w, h;
	bool top;
	void
	invoke_interpolate(int x)
	{
		if ( top ) {
			for (int y=0; y<h; y+=2) {
				interpolate(x, y);
			}
		} else {
			for (int y=1; y<h; y+=2) {
				interpolate(x, y);
			}
		}
	}
};
static void
ksa_deinterlace_temporal(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 6) ) { return; }
	DiTemporal it;
	int i=0;
	it.dest = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.past = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.future = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.w = param->get_param_int(i++);
	it.h = param->get_param_int(i++);
	it.top = param->get_param_boolean(i++);
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate(j); }, it.w);
}

class DiGhost {
private:
	void
	interpolate_spatial(PIXEL_RGBA *d, const bool &t, const int &x, const int &y)
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
			const auto s_px = &d[sy*w+x];
			const float wya = wy*s_px->a;
			r += s_px->r*wya;
			g += s_px->g*wya;
			b += s_px->b*wya;
			a += wya;
			ww += wy;
		}
		auto d_px = &d[y*w+x];
		d_px->r = uc_cast(r/a);
		d_px->g = uc_cast(g/a);
		d_px->b = uc_cast(b/a);
		d_px->a = uc_cast(a/ww);
	}
	void
	interpolate_temporal(const int &x, const int &y)
	{
		const int idx = y*w+x;
		auto px_d = &past_temp[idx];
		const auto px_f = &future[idx];
		if ( px_d->a == 255 && px_f->a == 255 ) {
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_d->r)+static_cast<int>(px_f->r))>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_d->g)+static_cast<int>(px_f->g))>>1 );
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_d->b)+static_cast<int>(px_f->b))>>1 );
		} else {
			const float pa = px_d->a, fa = px_f->a;
			const float pafa = pa+fa;
			px_d->r = uc_cast( ( px_d->r*pa + px_f->r*fa ) / pafa );
			px_d->g = uc_cast( ( px_d->g*pa + px_f->g*fa ) / pafa );
			px_d->b = uc_cast( ( px_d->b*pa + px_f->b*fa ) / pafa );
			px_d->a = uc_cast( pafa*0.5f );
		}
	}
	void
	interpolate0(const int &x, const int &y)
	{
		interpolate_spatial(dest, top, x, y);
		interpolate_temporal(x, y);
	}
	void
	interpolate1(const int &x, const int &y)
	{
		interpolate_spatial(past_temp, !top, x, y);
	}
	void
	mix(const int &x, const int &y)
	{
		const int idx = y*w+x;
		auto px_d=&dest[idx], px_t=&past_temp[idx];
		if ( px_d->a == 255 && px_t->a == 255 ) {
			px_d->r = static_cast<unsigned char>( (static_cast<int>(px_d->r)+static_cast<int>(px_t->r)+1)>>1 );
			px_d->g = static_cast<unsigned char>( (static_cast<int>(px_d->g)+static_cast<int>(px_t->g)+1)>>1 );
			px_d->b = static_cast<unsigned char>( (static_cast<int>(px_d->b)+static_cast<int>(px_t->b)+1)>>1 );
		} else {
			const float da = px_d->a, ta = px_t->a;
			const float data = da+ta;
			px_d->r = uc_cast( ( px_d->r*da + px_t->r*ta ) / data );
			px_d->g = uc_cast( ( px_d->g*da + px_t->g*ta ) / data );
			px_d->b = uc_cast( ( px_d->b*da + px_t->b*ta ) / data );
			px_d->a = uc_cast( data*0.5f );
		}
	}
public:
	PIXEL_RGBA *dest, *past_temp;
	const PIXEL_RGBA *future;
	int w, h;
	bool top;
	void
	invoke_interpolate0(int x)
	{
		if ( top ) {
			for (int y=0; y<h; y+=2) {
				interpolate0(x, y);
			}
		} else {
			for (int y=1; y<h; y+=2) {
				interpolate0(x, y);
			}
		}
	}
	void
	invoke_interpolate1(int x)
	{
		if ( top ) {
			for (int y=1; y<h; y+=2) {
				interpolate1(x, y);
			}
		} else {
			for (int y=0; y<h; y+=2) {
				interpolate1(x, y);
			}
		}
	}
	void
	invoke_mix(int x)
	{
		for (int y=0; y<h; y++) {
			mix(x, y);
		}
	}
};
static void
ksa_deinterlace_ghost(SCRIPT_MODULE_PARAM *param)
{
	// 引数受け取り
	if ( check_arg_num(param, 6) ) { return; }
	DiGhost it;
	int i=0;
	it.dest = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.past_temp = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.future = static_cast<PIXEL_RGBA *>(param->get_param_data(i++));
	it.w = param->get_param_int(i++);
	it.h = param->get_param_int(i++);
	it.top = param->get_param_boolean(i++);
	
	// 本処理
	TP->parallel_do([&it](int j){ it.invoke_interpolate0(j); }, it.w);
	TP->parallel_do([&it](int j){ it.invoke_interpolate1(j); }, it.w);
	TP->parallel_do([&it](int j){ it.invoke_mix(j); }, it.w);
}
