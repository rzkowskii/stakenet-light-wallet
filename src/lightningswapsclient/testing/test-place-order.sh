#!/bin/bash
PRICE=$1
AMOUNT=$2
BUY=$3
PORT=$4

if [ -z "$PORT" ]
then 
    PORT=50051
fi

./lightningswapsclient --port=$PORT addcurrency BTC /tmp/lnd/exchange-b/tls.cert localhost:20001 && 
./lightningswapsclient --port=$PORT addcurrency XSN /tmp/lnd/exchange-a/tls.cert localhost:20000 && 
./lightningswapsclient --port=$PORT placeorder XSN/BTC $PRICE $AMOUNT $BUY
