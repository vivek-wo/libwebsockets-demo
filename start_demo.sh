#! /bin/bash

export LD_LIBRARY_PATH=deps/lib/:$LD_LIBRARY_PATH

if [ $1 = "server" ];then
./websocket-server
else
./websocket-client
fi
