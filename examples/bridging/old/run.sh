#!/bin/sh

numNodes=$1
topologyType=grid
topologyDegree=4

outputMode=no

linkDatarateBps=10000000
linkDelayMs=1
stpSimulationTimeSecs=10
stpActivatePeriodicalBpdus=0
stpActivateExpirationMaxAge=0
stpMaxAge=20
stpHelloTime=2
stpForwDelay=3
stpTxHoldPeriod=1
stpMaxBpduTxHoldPeriod=1

phyMxFilename="examples/bridging/phyMx/${numNodes}nodes_${topologyType}${topologyDegree}_phyCostMx.txt"
outputFilename="output/${numNodes}nodes_${topologyType}${topologyDegree}.txt"

echo $phyFilename

echo "BUILDING"

./waf build

echo "RUNNING"

./waf --run "ptp-morebridges-test
                --numNodes=$numNodes
                --phyMxFilename=$phyMxFilename
                --linkDatarateBps=$linkDatarateBps
                --linkDelayMs=$linkDelayMs
                --stpSimulationTimeSecs=$stpSimulationTimeSecs
                --stpActivatePeriodicalBpdus=$stpActivatePeriodicalBpdus
                --stpActivateExpirationMaxAge=$stpActivateExpirationMaxAge
                --stpMaxAge=$stpMaxAge
                --stpHelloTime=$stpHelloTime
                --stpForwDelay=$stpForwDelay
                --stpTxHoldPeriod=$stpTxHoldPeriod
                --stpMaxBpduTxHoldPeriod=$stpMaxBpduTxHoldPeriod
                " > $outputFilename 2>&1

echo "RUNNING DONE (check $outputFilename)"

awk -f output/parse_output.awk $outputFilename
