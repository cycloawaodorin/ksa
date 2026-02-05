local ksa = obj.module("ksa_ext")

ksa.vert_rect = function(vert, x0, y0, x1, y1, u0, v0, u1, v1)
	if ( not u0 ) then
		u0, v0, u1, v1 = 0, 0, obj.w, obj.h
	end
	table.insert(vert, {x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0, u0,v0, u1,v0, u1,v1, u0,v1})
end

ksa.vert_tri = function(vert, x0, y0, x1, y1, x2, y2)
	local mid_x, mid_y = (x1+x2)/2, (y1+y2)/2
	local u0, v0, u1, v1 = 0, 0, obj.w, obj.h
	table.insert(vert, {x0,y0,0, x1,y1,0, mid_x,mid_y,0, x2,y2,0, u0,v0, u1,v0, u1,v1, u0,v1})
end

ksa.drawrect = function(x0, y0, x1, y1, alpha, u0, v0, u1, v1)
	if ( u0 ) then
		obj.drawpoly(x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0, u0,v0, u1,v0, u1,v1, u0,v1, alpha)
	elseif ( alpha ) then
		local u0, v0, u1, v1 = 0, 0, obj.w, obj.h
		obj.drawpoly(x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0, u0,v0, u1,v0, u1,v1, u0,v1, alpha)
	else
		obj.drawpoly(x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0)
	end
end

ksa.drawtri = function(x0, y0, x1, y1, x2, y2, alpha)
	local mid_x, mid_y = (x1+x2)/2, (y1+y2)/2
	if ( alpha ) then
		local u0, v0, u1, v1 = 0, 0, obj.w, obj.h
		obj.drawpoly(x0,y0,0, x1,y1,0, mid_x,mid_y,0, x2,y2,0, u0,v0, u1,v0, u1,v1, u0,v1, alpha)
	else
		obj.drawpoly(x0,y0,0, x1,y1,0, mid_x,mid_y,0, x2,y2,0)
	end
end

ksa.drawvert = function(v, alpha)
	if next(v) ~= nil then
		if alpha then
			obj.drawpoly(v, alpha)
		else
			obj.drawpoly(v)
		end
	end
end

ksa.round = function(x)
	return math.floor(x+0.5)
end

ksa.hypot = function(x, y)
	if x < 0 then
		x = -x
	end
	if y < 0 then
		y = -y
	end
	if x < y then
		x, y = y, x
	end
	if y == 0 then
		return x
	else
		y = y/x
		return x*math.sqrt(1+y*y)
	end
end

ksa.xor = function(b0, b1)
	if b0 then
		return b1
	else
		return (not b1)
	end
end

ksa.log = function(fmt,...)
	debug_print(string.format(tostring(fmt).."",...))
end

return ksa
