local ksa = {}

ksa.drawrect = function(obj, x0, y0, x1, y1, alpha, u0, v0, u1, v1)
	if ( u0 ) then
		obj.drawpoly(x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0, u0,v0, u1,v0, u1,v1, u0,v1, alpha)
	elseif ( alpha ) then
		u0, v0, u1, v1 = 0, 0, obj.w, obj.h
		obj.drawpoly(x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0, u0,v0, u1,v0, u1,v1, u0,v1, alpha)
	else
		obj.drawpoly(x0,y0,0, x1,y0,0, x1,y1,0, x0,y1,0)
	end
end

return ksa
