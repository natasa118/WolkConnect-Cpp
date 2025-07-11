#!/usr/bin/env bash

cd ./WolkConnect-Cpp || exit

debuild -us -uc -b -j$(nproc)
