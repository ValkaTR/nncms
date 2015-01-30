#!/bin/bash

killall nncms

#cd /home/rodent/nncms/src
cd src
./spawn-fcgi -a 127.0.0.1 -p 9000 ./nncms
cd ..

#./spawn-fcgi -a 127.0.0.1 -p 9002 ./nncms