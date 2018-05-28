#!/bin/bash
./stress/cpu_stress &
sudo cpulimit -fvz -e cpu_stress -l $1;
pkill cpu_stress;

