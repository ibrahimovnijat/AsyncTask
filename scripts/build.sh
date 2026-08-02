#!/bin/bash
clear

echo -e "Configuring tasklib and asynctask_cli ... \n"
cd ../
rm -rf build
cmake -S . -B build
cmake --build build --preset debug

echo -e "Build finished, no errors...\n"