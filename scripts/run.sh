#!/bin/bash
clear

echo -e "\n----------- Async Task Cli ---------- \n"

cd ../build/asynctask_cli/
./asynctask_cli "$@"

echo -e "\n--------------- End -----------------\n"
