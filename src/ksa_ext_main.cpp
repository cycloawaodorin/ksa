#include "lua/lua.hpp"

#include "ksa_ext.cpp"

static constexpr luaL_Reg ksa_ext[] = {
#include "functions.c"
	{ nullptr, nullptr }
};

extern "C" {
int
luaopen_ksa_ext(lua_State *L)
{
	luaL_register(L, "ksa_ext", ksa_ext);
	return 1;
}
}
