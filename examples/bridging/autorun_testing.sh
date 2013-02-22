#!/bin/bash

#set -x

source autorun_lib.sh


##########################################################################
# BUILD c++
#
  # build program
  echo -e "BUILDING"
  ./waf build
#
# BUILD c++
##########################################################################



##########################################################################
# INPUT PARAMETERS
#  
  # topology
  topologyType=grid # grid/manual
  topologyDegree=4
  linkDatarateBps=10000000000
  linkDelayUs=100

  # failure
  activateFailure=1
  activatePhyFailDetect=1
  linkFailureTimeUs=11000000

  stpMode=4                         # stp(1) - rstp(2) - spbIsIs(3) - spbDv(4)

  averageMode=numNodes  		        # numNodes - txHoldCount - txHoldPeriod
  
  randomBridgeIds=1                 # random distribution of bridge Ids
  activateRandBridgeInit=0          # random startup time of nodes

  xstpActivatePeriodicalBpdus=1     # activate the periodical generation of BPDUs
  xstpActivateExpirationMaxAge=1    # activate the expiration of MessAge (this parameter does not change the discarding after receiving a MessAge>MaxAge)
  xstpActivateIncreaseMessAge=1     # activate increase of MessAge at each hop
  xstpActivateTopologyChange=0      # activate topology change
  
  spbdvActivatePathVectorDuplDetect=1 # activates the detection of duplicated elements in the path vector (in order to avoid looping BPDUs)
  
  xstpMaxAge=20                     # timer values
  xstpForwDelay=15                   # ...
  xstpHelloTime=2                   # ...

  xstpResetTxHoldPeriod=1           # reset to 0 the txHold counters (only effect on RSTP implementation) (IEEE STP always to 0 - IEEE RSTP decreases 1)

  xstpSimulationTimeSecs=20        # after this moment no more BPDUs are generated, and no more timeouts considered


  #numNodes=4                     # => there is a loop for this parameter
  #xstpMaxBpduTxHoldPeriod=100    # => there is a loop for this parameter
  #xstpTxHoldPeriod=1             # => there is a loop for this parameter
  # nodeAToFail=0                 # => there is a loop for this parameter
  # nodeBToFail=1                 # => there is a loop for this parameter
  #forcedRootNode=0               # => there is a loop for this parameter
  #forcedSecondRootNode=0         # => there is a loop for this parameter

  # tmp counter
  finalNumExecutions=0
  
  # traffic parameters
  trafficActivate=0
  trafficPktBytesDistr=1      # 0: constant | ...
  trafficPktBytesMean=1000
  trafficPktBytesVariance=0 
  trafficPktBytesUpper=0
  trafficPktBytesLower=0
  
  trafficInterPktTimeDistr=1   # 0: constant | ...
  trafficInterPktTimeMean=1
  trafficInterPktTimeVariance=0 
  trafficInterPktTimeUpper=0  
  trafficInterPktTimeLower=0
  
  trafficDestAddrMode=0  # 0: broadcast | 1: pairs (i <=> nodes-i) | 2: random
  
  # 1Gbps   => PktLength: 1000 bytes | InterPktTime: 0.000008  s
  # 1Gbps   => PktLength: 500  bytes | InterPktTime: 0.000004  s
  # 1Gbps   => PktLength: 100  bytes | InterPktTime: 0.0000008 s
  
  # 100Mbps => PktLength: 1000 bytes | InterPktTime: 0.00008  s
  # 100Mbps => PktLength: 500  bytes | InterPktTime: 0.00004  s
  # 100Mbps => PktLength: 100  bytes | InterPktTime: 0.000008 s

  # 10Mbps  => PktLength: 1000 bytes | InterPktTime: 0.0008  s
  # 10Mbps  => PktLength: 500  bytes | InterPktTime: 0.0004  s
  # 10Mbps  => PktLength: 100  bytes | InterPktTime: 0.00008 s
  
  # 1Mbps   => PktLength: 1000 bytes | InterPktTime: 0.008  s
  # 1Mbps   => PktLength: 500  bytes | InterPktTime: 0.004  s
  # 1Mbps   => PktLength: 100  bytes | InterPktTime: 0.0008 s

  #define DIST_CONSTANT     1
  #define DIST_UNIFORM      2
  #define DIST_EXPONENTIAL  3
  #define DIST_PARETO       4
  #define DIST_NORMAL       5
  #define DIST_LOGNORMAL    6
  
  #startApplicationTime?
  #stopApplicationTime?
    
#
# INPUT PARAMETERS
##########################################################################



##########################################################################
# SET OUTPUT TABLE FILENAME
#
  if [ $stpMode = 1 ];then
    stpTextFilename="stp"
  fi
  if [ $stpMode = 2 ];then
    stpTextFilename="rstp"
  fi
  if [ $stpMode = 3 ];then
    stpTextFilename="spbIsIs"
  fi
  if [ $stpMode = 4 ];then
    stpTextFilename="spbDv"
  fi

  outputTableStpFilenameRaw="output/outTableRaw_${stpTextFilename}.txt"
  WriteHeaderOutputTableStp $stpMode $outputTableStpFilenameRaw 

  outputTableStpFilename="output/outTable_${stpTextFilename}.txt"
  WriteHeaderOutputTableStp $stpMode $outputTableStpFilename

  outputTableTrafficFilenameRaw="output/outTableRaw_traffic.txt"
  WriteHeaderOutputTableTraffic $outputTableTrafficFilenameRaw 

  outputTableTrafficFilename="output/outTable_traffic.txt"
  WriteHeaderOutputTableTraffic $outputTableTrafficFilename

#
# SET OUTPUT TABLE FILENAME
##########################################################################



##########################################################################
# LOOP NUMNODES & NUM OF REPEATED EXECUTIONS
#

  for numNodes in 4 9 16 25 36 49 64 81 100
  do
  if [ $averageMode = numNodes ];then
    colToAverage=$numNodes                # to do the average of several executions - use numNodes as x-axis
  fi
 
  #xstpMaxAge=`echo -e "sqrt($numNodes)\nquit\n" | bc -q -i`
  #xstpMaxAge=`expr $xstpMaxAge + $xstpMaxAge + 1`  

  numExecutions=0 #loop later on, just before single execution


#
# LOOP NUMNODES & NUM OF REPEATED EXECUTIONS
##########################################################################



##########################################################################
# LOOP TX HOLD COUNT
#
  for xstpMaxBpduTxHoldPeriod in 1000 # 1 2 3 4 5 6 7 8 9 10 # 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 # 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 # 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 # 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100
  do
  if [ $averageMode = txHoldCount ];then
    colToAverage=$xstpMaxBpduTxHoldPeriod # to do the average of several executions - use xstpMaxBpduTxHoldPeriod as x-axis
  fi

#
# LOOP TX HOLD COUNT
##########################################################################

##########################################################################
# LOOP TX HOLD PERIOD
#
  for xstpTxHoldPeriod in 1 # 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
  do
  if [ $averageMode = txHoldPeriod ];then
    colToAverage=$xstpTxHoldPeriod # to do the average of several executions - use xstpTxHoldPeriod as x-axis
  fi

#
# LOOP TX HOLD COUNT
##########################################################################




##########################################################################
# LOOP FORCED ROOT and 2nd ROOT
#
  #for (( forcedRootNode=0; forcedRootNode < numNodes ; forcedRootNode++)); do
  #for forcedRootNode in 0; do
  for forcedRootNode in -1; do
  
  #tmp2ndRoot=`expr $numNodes - 1` #opposite corner
  #tmp2ndRoot=1
  tmp2ndRoot=-1
  for forcedSecondRootNode in $tmp2ndRoot; do
#
# LOOP FORCED BRIDGE IDS
##########################################################################




##########################################################################
# SET PHYMX FILENAME
#
  if [ $topologyType = grid ];then
    phyMxFilename="examples/bridging/phyMx/${numNodes}nodes_${topologyType}${topologyDegree}_phyCostMx.txt"
    phyMxFilename2="phyMx/${numNodes}nodes_${topologyType}${topologyDegree}_phyCostMx.txt"
  fi

  if [ $topologyType = manual ];then
    phyMxFilename="examples/bridging/phyMx/6nodes_manual_phyCostMx.txt"
    phyMxFilename2="phyMx/6nodes_manual_phyCostMx.txt"
  fi

#
# SET PHYMX FILENAME
##########################################################################



##########################################################################
# COLLECT LINKS FOR LOOP of LINK FAILURES
#
  # read the existing links from the physical matrix and create a for loop for each one of the links  
    declare -a connections
    declare -i pos
    declare -i row
    declare -i col
    declare -i nodeAToFail
    declare -i nodeBToFail

    unset connections 

    sed 's/[ \t]/\n/g' < $phyMxFilename2 > "tmp.tmp"
    sed '/^$/d' < "tmp.tmp" > "tmp2.tmp"
    rm "tmp.tmp"
    {
      let row=0
      let col=0
      while read line; do
        #echo $line
        if [ $line == "1" ]; then
          let pos=$row*1000+$col
          connections[$pos]=$pos
          #echo $pos
          #echo ${connections[$pos]}
        fi
        col=$(expr $col + 1)
        if [ $col == $numNodes ]; then
          col=0
          row=$(expr $row + 1)
        fi
      done
    } < "tmp2.tmp"
    rm "tmp2.tmp"
#
# COLLECT LINKS FOR LOOP LINK FAILURES
##########################################################################



##########################################################################
# LOOP LINK FAILURES - set for loops
#

  #manual several link failures
  #unset connections;
  #connections[0001]=0001;
  #connections[1009]=1009;
  #connections[9010]=9010;
  #connections[10018]=10018;
  #connections[18019]=18019;
  #connections[19027]=19027;
  #connections[27028]=27028;
  #connections[28036]=28036;
  #connections[36037]=36037;
  #connections[37045]=37045;
  #connections[45046]=45046;
  #connections[46054]=46054;
  #connections[54055]=54055;
  #connections[55063]=55063;


  #for value in ${connections[@]}; do # note this is executed twice each link (one for A->B and another for B->A)
  #  nodeAToFail=$(expr $value/1000)
  #  nodeBToFail=$(expr $value-$nodeAToFail*1000)

  #for loop for single manual link failure (nodeBToFail=-1 to fail all nodeAToFail connections)
  for tmp in 1; do
    # root ndoe or its neighbors
    # nodeAToFail=0 # forcedRootNode
    # nodeBToFail=1 # 1-3

    # other manual
    # nodeAToFail=`expr $numNodes - 1`
    # nodeBToFail=`expr $numNodes - 2`

    # grid - link in the center
    if [ `expr $numNodes % 2` == 0 ]; then # parell
      echo -e "numNodes:    $numNodes (parell)"
      nodeAToFail=`echo "$numNodes/2 + sqrt($numNodes)/2 - 1"|bc`
      nodeBToFail=`echo "$numNodes/2 + sqrt($numNodes)/2"|bc`
    fi
    if [ `expr $numNodes % 2` == 1 ]; then # imparell
      echo -e "numNodes:    $numNodes (imparell)"
      nodeAToFail=`echo "$numNodes/2"|bc`
      nodeBToFail=`echo "$numNodes/2 + 1"|bc` 
    fi
    
    echo -e "nodeAToFail: $nodeAToFail"
    echo -e "nodeBToFail: $nodeBToFail"

    # grid - link in the very corner
    #nodeAToFail=0
    #nodeBToFail=1

#
# LOOP LINK FAILURES - set for loops
##########################################################################



##########################################################################
# LOOP NUMBER OF EXECUTIONS WITH SAME CONFIGURATION
#
  for (( executionId=0; executionId<$numExecutions; executionId++ ))
  do
#
# LOOP NUMBER OF EXECUTIONS WITH SAME CONFIGURATION
##########################################################################



##########################################################################
# SINGLE SIMULATION EXECUTION
#
  finalNumExecutions=`expr $finalNumExecutions + 1`
  
  if [ $forcedRootNode = -1 ];then
    forcedRootNodeFilename="X"
  else
    forcedRootNodeFilename=$forcedRootNode
  fi
  if [ $forcedSecondRootNode = -1 ];then
    forcedSecondRootNodeFilename="X"
  else
    forcedSecondRootNodeFilename=$forcedSecondRootNode
  fi
    
  if [ $nodeAToFail = -1 ];then
    nodeAToFailFilename="X"
  else
    nodeAToFailFilename=$nodeAToFail
  fi
  if [ $nodeBToFail = -1 ];then
    nodeBToFailFilename="all"
  else
    nodeBToFailFilename=$nodeBToFail
  fi
  if [ $randomBridgeIds = 1 ];then
    randomBridgeIdsFilename="rnd"
  else
    randomBridgeIdsFilename="seq"
  fi


  outputTracesFilename="output/n${numNodes}_${topologyType}${topologyDegree}_${stpTextFilename}_txHld${xstpTxHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"

  echo -e "\nRUNNING"

  ./waf --run "ptp-grid-bridges-traff\
                  --numNodes=$numNodes\
                  --phyMxFilename=$phyMxFilename\
                  --linkDatarateBps=$linkDatarateBps\
                  --linkDelayUs=$linkDelayUs\
                  --activateFailure=$activateFailure\                  
                  --activatePhyFailDetect=$activatePhyFailDetect\                  
                  --linkFailureTimeUs=$linkFailureTimeUs\
                  --nodeAToFail=$nodeAToFail\
                  --nodeBToFail=$nodeBToFail\
                  --stpMode=$stpMode\
                  --randomBridgeIds=$randomBridgeIds\
                  --forcedRootNode=$forcedRootNode\
                  --forcedSecondRootNode=$forcedSecondRootNode\
                  --activateRandBridgeInit=$activateRandBridgeInit\
                  --xstpSimulationTimeSecs=$xstpSimulationTimeSecs\
                  --xstpActivatePeriodicalBpdus=$xstpActivatePeriodicalBpdus\
                  --xstpActivateExpirationMaxAge=$xstpActivateExpirationMaxAge\
                  --xstpActivateIncreaseMessAge=$xstpActivateIncreaseMessAge\                  
                  --xstpActivateTopologyChange=$xstpActivateTopologyChange\
                  --spbdvActivatePathVectorDuplDetect=$spbdvActivatePathVectorDuplDetect\
                  --xstpMaxAge=$xstpMaxAge\
                  --xstpHelloTime=$xstpHelloTime\
                  --xstpForwDelay=$xstpForwDelay\
                  --xstpTxHoldPeriod=$xstpTxHoldPeriod\
                  --xstpMaxBpduTxHoldPeriod=$xstpMaxBpduTxHoldPeriod\
                  --xstpResetTxHoldPeriod=$xstpResetTxHoldPeriod\
                  --trafficActivate=$trafficActivate\
                  --trafficPktBytesDistr=$trafficPktBytesDistr\
                  --trafficPktBytesMean=$trafficPktBytesMean\
                  --trafficPktBytesVariance=$trafficPktBytesVariance\                  
                  --trafficPktBytesUpper=$trafficPktBytesUpper\                  
                  --trafficPktBytesLower=$trafficPktBytesLower\ 
                  --trafficInterPktTimeDistr=$trafficInterPktTimeDistr\
                  --trafficInterPktTimeMean=$trafficInterPktTimeMean\
                  --trafficInterPktTimeVariance=$trafficInterPktTimeVariance\                  
                  --trafficInterPktTimeUpper=$trafficInterPktTimeUpper\                  
                  --trafficInterPktTimeLower=$trafficInterPktTimeLower\ 
                  --trafficDestAddrMode=$trafficDestAddrMode\                                      
                  "   > $outputTracesFilename 2>&1

  echo -e "RUNNING DONE (check $outputTracesFilename)"

  # some 'grep' checkings
  grep WARNING $outputTracesFilename
  grep ERROR $outputTracesFilename
  
  echo -e "Correctly finished:                         $(grep -c Done. $outputTracesFilename)"
  echo -e "Initial convergence with spanning trees:   " `awk 'BEGIN{tree=-1} /It is a spanning tree \(initial convergence\)/ {tree=1} /Not a spanning tree yet \(initial convergence\)/ {tree=0} END{print tree}' $outputTracesFilename`
  echo -e "Initial convergence with symmetric trees:  " `awk 'BEGIN{sym=-1} /Trees are symmetric \(initial convergence\)/ {sym=1} /Trees are not symmetric \(initial convergence\)/ {sym=0} END{print sym}' $outputTracesFilename`

  echo -e "Failure convergence with spanning trees:   " `awk 'BEGIN{tree=-1} /It is a spanning tree \(failure convergence\)/ {tree=1} /Not a spanning tree yet \(failure convergence\)/ {tree=0} END{print tree}' $outputTracesFilename`
  echo -e "Failure convergence with symmetric trees:  " `awk 'BEGIN{sym=-1} /Trees are symmetric \(failure convergence\)/ {sym=1} /Trees are not symmetric \(failure convergence\)/ {sym=0} END{print sym}' $outputTracesFilename`
  
  echo -e "\n"
  
  # parse output and add line to output table file
  if [ $stpMode = 1 ];then
    awk -v exId=$executionId -f output/stp_output_single_line.awk $outputTracesFilename >> $outputTableStpFilenameRaw
  fi
  if [ $stpMode = 2 ];then
    awk -v exId=$executionId -f output/rstp_output_single_line.awk $outputTracesFilename >> $outputTableStpFilenameRaw
  fi  
  if [ $stpMode = 3 ];then
    awk -v exId=$executionId -f output/spbisis_output_single_line.awk $outputTracesFilename >> $outputTableStpFilenameRaw
  fi  
  if [ $stpMode = 4 ];then
    awk -v exId=$executionId -f output/spbdv_output_single_line.awk $outputTracesFilename >> $outputTableStpFilenameRaw
  fi  
  # print timeline of BPDUs about root 0 (whch becomes old root after root failure)
  #awk '/RSTP: Recvs BPDU \[R:0/{print $1," 1"}' output/n25_grid4_rstp_txHld1_100_rnd_r0_srX_f0-all_0.txt | sed 's/ns/ /' > rootInfoTimeline.txt
  
  # print timeline of BPDUs
  #awk '/RSTP: Recvs BPDU /{print $1," 1"}' output/n25_grid4_rstp_txHld1_100_rnd_r0_srX_f0-all_0.txt | sed 's/ns/ /' > bpdusTimeline.txt
  
  # parse output and add line to output table traffic file
  if [ $trafficActivate = 1 ];then
    awk -f output/traffic_output_single_line.awk $outputTracesFilename >> $outputTableTrafficFilenameRaw
  fi
 
#
# SINGLE SIMULATION EXECUTION
##########################################################################



##########################################################################
# END OF FOR LOOPS
#
  done #number of repeated executions
  done #for failed links
  done #for fixed second root
  done #for fixed root



  done #for tcHoldPeriod
  done #for txHoldCount
  done #for nodes





