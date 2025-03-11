#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Error: You must provide server's name and port"
    exit 1
fi
if ! [[ $2 =~ ^[0-9]+$ ]]; then
    echo "Error: Port must be a number"
    exit 1
fi
server=$1
port=$2

./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &
./bin/jobCommander $server $port issueJob ./progDelay 15 &