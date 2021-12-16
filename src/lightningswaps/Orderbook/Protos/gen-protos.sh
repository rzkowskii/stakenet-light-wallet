#!/bin/bash

protoc -I$PWD --cpp_out=$PWD stakenet/orderbook/*.proto
