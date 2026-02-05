#!/bin/bash

mkdir Script
mkdir ./Script/KSA
cp ../ksa2/* ./Script/KSA
VERSION=$(grep '#define VERSION' version.hpp | sed -E 's/.*"([^"]+)".*/\1/')
zip -r "../$1.${VERSION}.au2pkg.zip" Script
rm -rf Script
