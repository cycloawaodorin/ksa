#!/bin/bash

VERSION=$(grep '#define VERSION' version.h | sed -E 's/.*"([^"]+)".*/\1/')

zip -r "../$1.${VERSION}.zip" ../ksa
