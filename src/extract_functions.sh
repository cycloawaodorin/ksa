#!bash

: > functions.c
cat ksa_ext.cpp | while read line; do
	if [[ ${line} =~ ^ksa_(.+)\(lua_State' '\*L\)$ ]]; then
		echo "	{ \"${BASH_REMATCH[1]}\", ksa_${BASH_REMATCH[1]} }," >> functions.c
	fi
done
