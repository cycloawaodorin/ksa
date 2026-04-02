#!/bin/bash

mkdir KSA
cp ../ksa/* ./KSA
VERSION=$(grep '#define VERSION' version.h | sed -E 's/.*"([^"]+)".*/\1/')
zip -r "../$1.${VERSION}.zip" KSA
rm -rf KSA
