#!/bin/bash

dest=../release
mkdir -p ${dest}
name=${dest}/`git describe --tags`.zip
rm -f ${name}
zip -j ${name} src/*.cpp include/*.h
