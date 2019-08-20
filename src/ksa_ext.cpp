#include "lua/lua.hpp"

static const luaL_Reg ksa_ext[] = {
	{ nullptr, nullptr }
};

extern "C" {
__declspec(dllexport) int
luaopen_ksa_ext(lua_State *L)
{
	luaL_register(L, "ksa_ext", ksa_ext);
	return 1;
}
}
