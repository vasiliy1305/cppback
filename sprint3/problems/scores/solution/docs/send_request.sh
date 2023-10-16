#!/bin/bash

while true; do
    curl -X POST -H "Content-Type: application/json" -d '{"timeDelta":100}' http://127.0.0.1:8080/api/v1/game/tick
    sleep 0.1
done
