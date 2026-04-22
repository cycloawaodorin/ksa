#!bash

: > functions.cpp
cat ksa_ext.cpp | while read line; do
	if [[ ${line} =~ ^ksa_(.+)\(SCRIPT_MODULE_PARAM' '\*param\)$ ]]; then
		echo "	{ L\"${BASH_REMATCH[1]}\", KSA::ksa_${BASH_REMATCH[1]} }," >> functions.cpp
	fi
done
