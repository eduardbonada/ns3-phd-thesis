#!/bin/bash


# rm output/outTable*; rm output/n*; ./autorun.sh machineName experimentId topType gridDegree meshType failureType

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

  # execution parameters
  ######################
  totalNumExecutions=1
  computerName=$1
  testId=$2  

  # general simulation parameters
  ###############################
  finalNumExecutions=0								# leave variable set to 0 (it is a counter)
  averageMode=txHoldCount  		        # numNodes - txHoldCount - txHoldPeriod  


  # topology
  ##########
  topologyType=$3 # grid / mesh / two-tier / manual / ring_root_aside / ring_to_full

  topologyGridDegree=$4
  topologyMeshType=$5 # light / heavy / full
  topologyTwoTierRatioCM=2
  topologyTwoTierRatioME=2 
  
  # topology sizes
  ################
  if [ $topologyType = grid ];then
    # grid        ( 4 9 16 25 36 49 64 81 100 )
    numNodesArray=( 4 9 16 25 36 49 64 81 100 )
  fi
  
  if [ $topologyType = mesh ];then
    if [ $topologyMeshType = light ];then
      # mesh-light  ( 12 20 30 40 50 60 72 84 98 )
      numNodesArray=( 8 12 20 30 50 72 84 98 ) 
    fi
    if [ $topologyMeshType = heavy ];then
      # mesh-heavy  ( 8 18 32 50 72 98 )
      numNodesArray=( 50 ) 
    fi
    if [ $topologyMeshType = full ];then
	    # mesh-full   ( 8 18 32 50 72 98 )
      numNodesArray=( 50 ) 
    fi
  fi
  
  if [ $topologyType = two-tier ];then
    if [ $topologyTwoTierRatioCM = 2 ] && [ $topologyTwoTierRatioME = 2 ];then
      # two-tier 2-2  ( 7 14 28 42 56 70 84 98 )
      numNodesArray=( 56 ) 
    fi
    if [ $topologyTwoTierRatioCM = 2 ] && [ $topologyTwoTierRatioME = 4 ];then
      # two-tier 2-4  ( 11 22 33 44 55 66 77 88 99 )
      numNodesArray=( 22 33 44 55 66 ) 
    fi
    if [ $topologyTwoTierRatioCM = 4 ] && [ $topologyTwoTierRatioME = 4 ];then
      # two-tier 4-4  ( 21 42 63 84 105 )
      numNodesArray=( 21 42 63 )
    fi
  fi

  if [ $topologyType = ring_root_aside ];then
    for (( i=4; i <= 40 ; i++)); do
      numNodesArray[$i-4]=$i;
    done;
    #numNodesArray=( 20 )
  fi

  if [ $topologyType = ring_to_full ];then
    numNodesArray=( 20 )
  fi
  
  if [ $topologyType = manual ];then
    numNodesArray=( 6 )
  fi  

  # link/node to failure
  ######################
  #if [ $topologyType = grid ];then
  #  failureType=nodecentral   # for grid topologies: linkcorner / linkcentral / nodecorner / nodecentral
  #fi
  #if [ $topologyType = mesh ];then
  #  failureType=linkcore   # for mesh topologies: linkcore / linkedge / nodecore / node-edge
  #fi
  #if [ $topologyType = two-tier ];then
  #  failureType=nodecore   # for two-tier topologies: nodecore / linkcore
  #fi  
  
  failureType=$6
  
  # links
  #######
  linkDatarateBps=10000000000
  linkDelayUs=100


  # failure within xstp
  #####################
  activateFailure=1
  activatePhyFailDetect=1
  linkFailureTimeUs=3000000
       
                            
  # xstp parameters
  #################
  stpMode=5                         # stp(1) - rstp(2) - spbIsIs(3) - spbDv(4) - rstpConf(5) - spbDvConf (6)
 
  randomBridgeIds=1                 # random distribution of bridge Ids
  activateRandBridgeInit=0          # random startup time of nodes

  xstpActivatePeriodicalBpdus=1     # activate the periodical generation of BPDUs
  xstpActivateExpirationMaxAge=1    # activate the expiration of MessAge (this parameter does not change the discarding after receiving a MessAge>MaxAge)
  xstpActivateIncreaseMessAge=1     # activate increase of MessAge at each hop
  xstpActivateTopologyChange=0      # activate topology change

  spbisisActivateBpduForwarding=0   # activates the exchange of BPDUs to transition ports to forwarding (otherwise the transition is made by simGod)
 
  spbdvActivatePathVectorDuplDetect=1 # activates the detection of duplicated elements in the path vector (in order to avoid looping BPDUs)
  spbdvActivateTreeDeactivationV2=0   # activates the tree deactivation V2: wait until receive '9999' in all R/A ports. Activate again when received 'not-9999' in D ports
  spbdvActivatePeriodicalStpStyle=0   # if true, the root of the tree sends the periodical messages (STP style), if false all nodes send (RSTP style)
  spbdvActivateBpduMerging=0          # activates the merging of BPDUs. otherwise one BPDU per frame. 
  
  xstpMaxAge=20                       # timer values
  xstpForwDelay=15                    # ...
  xstpHelloTime=2                     # ...

  xstpResetTxHoldPeriod=1           # reset to 0 the txHold counters (only effect on RSTP implementation) (IEEE STP always to 0 - IEEE RSTP decreases 1)

  xstpSimulationTimeSecs=50          # after this moment no more BPDUs are generated, and no more timeouts considered


  # traffic parameters
  ####################
  trafficActivate=0
  trafficPktBytesDistr=1      # 1: constant | ...
  trafficPktBytesMean=1000
  trafficPktBytesVariance=0 
  trafficPktBytesUpper=0
  trafficPktBytesLower=0
  
  trafficInterPktTimeDistr=1   # 1: constant | ...
  trafficInterPktTimeMean=0.0001
  trafficInterPktTimeVariance=0 
  trafficInterPktTimeUpper=0  
  trafficInterPktTimeLower=0
  
  trafficDestAddrMode=0  # 0: broadcast | 1: pairsn (i <=> nodes-i) | 2: random
  
			# 1Gbps   => PktLength: 1000 bytes | InterPktTime: 0.000008  s
			# 1Gbps   => PktLength: 500  bytes | InterPktTime: 0.000004  s
			# 1Gbps   => PktLength: 100  bytes | InterPktTimne: 0.0000008 s
		
			# 100Mbps => PktLength: 1000 bytes | InterPktTime: 0.00008  s
			# 100Mbps => PktLength: 500  bytes | InterPktTime: 0.00004  s
			# 100Mbps => PktLength: 100  bytes | InterPktTime: 0.000008 s

			# 10Mbps  => PktLength: 1000 bytes | InterPktTime: 0.0008  s
			# 10Mbps  => PktLength: 500  bytes | InterPktTimne: 0.0004  s
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
  if [ $stpMode = 5 ];then
    stpTextFilename="rstpConf"
  fi
  if [ $stpMode = 3 ];then
    stpTextFilename="spbIsIs"
  fi
  if [ $stpMode = 4 ];then
    stpTextFilename="spbDv"
  fi
  if [ $stpMode = 6 ];then
    stpTextFilename="spbDvConf"
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

  for numNodes in ${numNodesArray[@]}
  do
  if [ $averageMode = numNodes ];then
    colToAverage=$numNodes                # to do the average of several executions - use numNodes as x-axis
  fi
 
  #xstpMaxAge=`echo -e "sqrt($numNodes)\nquit\n" | bc -q -i`
  #xstpMaxAge=`expr $xstpMaxAge + $xstpMaxAge + 1`  

  numExecutions=$totalNumExecutions #loop later on, just before single execution


#
# LOOP NUMNODES & NUM OF REPEATED EXECUTIONS
##########################################################################


##########################################################################
# LOOP TOPOLOGIES IN RING_TO_FULL
#
  # N10: 5
  # N10: 35 (1-3-5-...-35)
  # N20: 170 (2-6-10-...-78)
  # N30: 405 (3-9-15-...-117)
  # N50: 1175 (5-15-195)
  #for (( numLinksRing=2; numLinksRing <= 78 ; numLinksRing=numLinksRing+4));
  #do
#
# LOOP NUMNODES & NUM OF REPEATED EXECUTIONS
##########################################################################


##########################################################################
# LOOP TX HOLD COUNT
#
  #let sqrtNumNodes=$numNodes*6
  #sqrtNumNodes=`echo -e "2*sqrt($numNodes)\nquit\n" | bc -q -i`
  for xstpMaxBpduTxHoldPeriod in 1000000 #1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 # 2 4 6 8 10 12 13 14 15 16 17 18 19 20 22 24 18 32 36 40 # $numNodes
  do
  if [ $averageMode = txHoldCount ];then
    colToAverage=$xstpMaxBpduTxHoldPeriod # to do the average of several executions - use xstpMaxBpduTxHoldPeriod as x-axis
  fi

#
# LOOP TX HOLD COUNT
##########################################################################

##########################################################################
# LOOP TX HOLD PERIOD (both for xSTP and for SPB-LS)
#
  txHoldPeriodTmp=`echo "0.01*$xstpMaxBpduTxHoldPeriod" | bc`
  if [ $stpMode = 2 ];then
    txHoldPeriodTmp=1
  fi
  if [ $stpMode = 5 ];then
    txHoldPeriodTmp=1
  fi
  if [ $stpMode = 3 ];then
    txHoldPeriodTmp=0
  fi
  if [ $stpMode = 4 ];then
    txHoldPeriodTmp=1
  fi
  if [ $stpMode = 6 ];then
    txHoldPeriodTmp=1
  fi
  for txHoldPeriod in $txHoldPeriodTmp # 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0
  do
  if [ $averageMode = txHoldPeriod ];then
    colToAverage=$txHoldPeriod # to do the average of several executions - use txHoldPeriod as x-axis
  fi

#
# LOOP TX HOLD COUNT
##########################################################################




##########################################################################
# LOOP FORCED ROOT and 2nd ROOT
#
  # all roots
  for (( forcedRootNode=0; forcedRootNode < numNodes ; forcedRootNode=forcedRootNode+1)); do
  #for forcedRootNode in -1; do

  # root center
	if [ `expr $numNodes % 2` == 0 ]; then # parell
	  rootTmp=`echo "$numNodes/2 + sqrt($numNodes)/2 - 1"|bc`
	fi
	if [ `expr $numNodes % 2` == 1 ]; then # imparell
	  rootTmp=`echo "$numNodes/2"|bc`
	fi
  #for forcedRootNode in $rootTmp; do
  
  #tmp2ndRoot=`expr $numNodes - 1` #opposite corner
  #tmp2ndRoot=1
  tmp2ndRoot=-1
  #tmp2ndRoot=10
  for forcedSecondRootNode in $tmp2ndRoot; do
#
# LOOP FORCED BRIDGE IDS
##########################################################################




##########################################################################
# SET PHYMX FILENAME
#
  if [ $topologyType = grid ];then
    phyMxFilename="examples/bridging/phyMx/grid/${numNodes}nodes_${topologyType}${topologyGridDegree}_phyCostMx.txt"
    phyMxFilename2="phyMx/grid/${numNodes}nodes_${topologyType}${topologyGridDegree}_phyCostMx.txt"
  fi

  if [ $topologyType = mesh ];then
    phyMxFilename="examples/bridging/phyMx/mesh/${numNodes}nodes_${topologyType}-${topologyMeshType}_phyCostMx.txt"
    phyMxFilename2="phyMx/mesh/${numNodes}nodes_${topologyType}-${topologyMeshType}_phyCostMx.txt"
  fi

  if [ $topologyType = two-tier ];then
    phyMxFilename="examples/bridging/phyMx/two-tier/${numNodes}nodes_${topologyType}_CM${topologyTwoTierRatioCM}-ME${topologyTwoTierRatioME}_phyCostMx.txt"
    phyMxFilename2="phyMx/two-tier/${numNodes}nodes_${topologyType}_CM${topologyTwoTierRatioCM}-ME${topologyTwoTierRatioME}_phyCostMx.txt"
  fi

  if [ $topologyType = manual ];then
    phyMxFilename="examples/bridging/phyMx/${numNodes}nodes_manual_phyCostMx.txt"
    phyMxFilename2="phyMx/${numNodes}nodes_manual_phyCostMx.txt"
  fi

  if [ $topologyType = ring_root_aside ];then
    phyMxFilename="examples/bridging/phyMx/ring_root_aside/${numNodes}nodes_ring_root-aside_phyCostMx.txt"
    phyMxFilename2="phyMx/ring_root_aside/${numNodes}nodes_ring_root-aside_phyCostMx.txt"
  fi

  if [ $topologyType = ring_to_full ];then
    phyMxFilename="examples/bridging/phyMx/ring_to_full_N${numNodes}/${numNodes}nodes_ring-to-full[${numLinksRing}]_phyCostMx.txt"
    phyMxFilename2="phyMx/ring_to_full_N${numNodes}/${numNodes}nodes_ring-to-full[${numLinksRing}]_phyCostMx.txt"
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
    declare -i pos2
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
          let pos2=$col*1000+$row
          #echo "A-pos:  " $pos;
          #echo "A-conn: " ${connections[$pos]};
          #echo "A-pos2:  " $pos2;
          #echo "A-conn2: " ${connections[$pos2]};
          if [[ ${connections[$pos2]} ]]; then
            connections[$pos2]=$pos2
          else
            connections[$pos]=$pos
          fi
          #echo "B-pos:  " $pos;
          #echo "B-conn: " ${connections[$pos]};
          #echo "B-pos2:  " $pos2;
          #echo "B-conn2: " ${connections[$pos2]};
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
	#connections[1]=1;
	#connections[9]=9;
	#connections[4005]=4005;
	#connections[2]=2;
	#connections[8]=8;
	#connections[4006]=4006;
	#connections[4]=4;
	#connections[6]=6;
	#connections[2006]=2006;
	#connections[10]=10;
	#connections[19]=19;
	#connections[5014]=5014;
		
  #for-loop for link failures (all links, or manually set in connections)
  #for value in ${connections[@]}; do # note this is executed twice each link (one for A->B and another for B->A)
  #  nodeAToFail=$(expr $value/1000)
  #  nodeBToFail=$(expr $value-$nodeAToFail*1000)

  #for-loop for node failures (all nodes)
  #for (( nodeToFail=0; nodeToFail < numNodes ; nodeToFail++)); do
  #  nodeAToFail=$nodeToFail
  #  nodeBToFail=-1  

  #for-loop for manual node failure
  for tmp in 1; do
    nodeAToFail=$forcedRootNode
    #nodeAToFail=$numNodes
    nodeBToFail=-1  
  
  #for-loop for preset failures with grid, mesh and two-tiers
  #for tmp in 1; do
  if [ 1 = 0 ];then
    # grid failures
    if [ $topologyType = grid ];then    
  	  if [ $failureType = linkcentral ];then
			  # grid - link in the center
				if [ `expr $numNodes % 2` == 0 ]; then # parell
				  nodeAToFail=`echo "$numNodes/2 + sqrt($numNodes)/2 - 1"|bc`
				  nodeBToFail=`echo "$numNodes/2 + sqrt($numNodes)/2"|bc`
				fi
				if [ `expr $numNodes % 2` == 1 ]; then # imparell
				  nodeAToFail=`echo "$numNodes/2"|bc`
				  nodeBToFail=`echo "$numNodes/2 + 1"|bc`
				fi
		  fi	
  	  if [ $failureType = nodecentral ];then
			  # grid - node in the center
				if [ `expr $numNodes % 2` == 0 ]; then # parell
				  nodeAToFail=`echo "$numNodes/2 + sqrt($numNodes)/2 - 1"|bc`
				  nodeBToFail=-1
				fi
				if [ `expr $numNodes % 2` == 1 ]; then # imparell
				  nodeAToFail=`echo "$numNodes/2"|bc`
				  nodeBToFail=-1
				fi
		  fi
  	  if [ $failureType = linkcorner ];then
			  # grid - link in the corner
 	  	  nodeAToFail=0
			  nodeBToFail=1
		  fi		  
  	  if [ $failureType = nodecorner ];then
			  # grid - link in the corner
 	  	  nodeAToFail=0
			  nodeBToFail=-1
		  fi			  		  	
		fi

    # mesh failures
    if [ $topologyType = mesh ];then    
  	  if [ $failureType = linkcore ];then
			  # mesh - link in the core (link between nodes 0-1)
  		  nodeAToFail=0
			  nodeBToFail=1
		  fi	
  	  if [ $failureType = nodecore ];then
			  # mesh - node in the core (node 0)
			  nodeAToFail=0
  		  nodeBToFail=-1
		  fi
  	  if [ $failureType = linkedge ];then
			  # grid - link in the edge (link between nodes first node, 0, and last)
 	  	  nodeAToFail=0
			  nodeBToFail=(numNodes-1)
		  fi		  
  	  if [ $failureType = nodeedge ];then
			  # grid - node in the edge (node N-1)
 	  	  nodeAToFail=numNodes-1
			  nodeBToFail=-1
		  fi			  		  	
		fi

    # two-tier failures
    if [ $topologyType = two-tier ];then    
  	  if [ $failureType = linkcore ];then
			  # two-tier - link in the core (link between nodes 0-1)
  		  nodeAToFail=0
			  nodeBToFail=1
		  fi	
  	  if [ $failureType = nodecore ];then
			  # two-tier - node in the core (node 0)
			  nodeAToFail=0
  		  nodeBToFail=-1
		  fi		  		  	
		fi
        		
    # manual
    if [ $topologyType = manual ];then
      nodeAToFail=0
      nodeBToFail=-1
    fi

    # no failure
    if [ $activateFailure = 0 ];then
      nodeAToFail=-1
      nodeBToFail=-1
    fi
  fi
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
  
  # output traces file stp or rstp
  if [ $stpMode = 1 ] || [ $stpMode = 2 ] || [ $stpMode = 5 ];then
    if [ $topologyType = grid ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyGridDegree}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = mesh ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyMeshType}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = two-tier ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyTwoTierRatioCM}-${topologyTwoTierRatioME}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = manual ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = ring_root_aside ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = ring_to_full ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}-${numLinksRing}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_r${forcedRootNodeFilename}_sr${forcedSecondRootNodeFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
  fi
  
  # output traces file spb-isis
  if [ $stpMode = 3 ];then 
    if [ $topologyType = grid ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyGridDegree}_${stpTextFilename}_txHld${txHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = mesh ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyMeshType}_${stpTextFilename}_txHld${txHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = two-tier ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyTwoTierRatioCM}-${topologyTwoTierRatioME}_${stpTextFilename}_txHld${txHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = manual ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}_${stpTextFilename}_txHld${txHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
  fi

  # output traces file spb-dv  
  if [ $stpMode = 4 ] || [ $stpMode = 6 ];then
    if [ $topologyType = grid ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyGridDegree}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = mesh ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyMeshType}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = two-tier ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}${topologyTwoTierRatioCM}-${topologyTwoTierRatioME}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
    if [ $topologyType = manual ];then
      outputTracesFilename="output/n${numNodes}_${topologyType}_${stpTextFilename}_txHld${txHoldPeriod}_${xstpMaxBpduTxHoldPeriod}_${randomBridgeIdsFilename}_f${nodeAToFailFilename}-${nodeBToFailFilename}_${executionId}.txt"
    fi
  fi  
  
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
                  --spbisisActivateBpduForwarding=$spbisisActivateBpduForwarding\
                  --spbdvActivatePathVectorDuplDetect=$spbdvActivatePathVectorDuplDetect\
                  --spbdvActivatePeriodicalStpStyle=$spbdvActivatePeriodicalStpStyle\
                  --spbdvActivateBpduMerging=$spbdvActivateBpduMerging\                                    
                  --xstpMaxAge=$xstpMaxAge\
                  --xstpHelloTime=$xstpHelloTime\
                  --xstpForwDelay=$xstpForwDelay\
                  --txHoldPeriod=$txHoldPeriod\
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

  echo -e "RUNNING DONE\n"

if [ 1 = 0 ];then
  # some 'grep' checkings
  grep WARNING $outputTracesFilename
  grep ERROR $outputTracesFilename
  
  echo -e "Correctly finished:                         $(grep -c Done. $outputTracesFilename)"
  echo -e "Initial convergence with spanning trees:   " `awk 'BEGIN{tree=-1} /It is a spanning tree \(initial convergence\)/ {tree=1} /Not a spanning tree yet \(initial convergence\)/ {tree=0} END{print tree}' $outputTracesFilename`
  echo -e "Initial convergence with symmetric trees:  " `awk 'BEGIN{sym=-1} /Trees are symmetric \(initial convergence\)/ {sym=1} /Trees are not symmetric \(initial convergence\)/ {sym=0} END{print sym}' $outputTracesFilename`

  echo -e "Failure convergence with spanning trees:   " `awk 'BEGIN{tree=-1} /It is a spanning tree \(failure convergence\)/ {tree=1} /Not a spanning tree yet \(failure convergence\)/ {tree=0} END{print tree}' $outputTracesFilename`
  echo -e "Failure convergence with symmetric trees:  " `awk 'BEGIN{sym=-1} /Trees are symmetric \(failure convergence\)/ {sym=1} /Trees are not symmetric \(failure convergence\)/ {sym=0} END{print sym}' $outputTracesFilename`

  echo -e "Droppings due to full queues:              " `awk 'BEGIN{full=0} /Queue full/ {full=1} END{print full}' $outputTracesFilename`
    
fi

  echo -e "\nPARSING $outputTracesFilename"
    
  # parse output and add line to output table file
  if [ $stpMode = 1 ];then
    awk -v exId=$executionId -f output/parse_traces_stp.awk $outputTracesFilename >> $outputTableStpFilenameRaw
  fi
  if [ $stpMode = 2 ];then
    awk -v exId=$executionId -f output/parse_traces_rstp.awk $outputTracesFilename >> $outputTableStpFilenameRaw
    #rm output/n*
    #mv output/n* /media/DADES/tmp_runs_manlleuA
  fi  
  if [ $stpMode = 5 ];then
    awk -v exId=$executionId -f output/parse_traces_rstp_conf.awk $outputTracesFilename >> $outputTableStpFilenameRaw
    #rm output/n*
    #mv output/n* /media/DADES/tmp_runs_manlleuA
  fi  
  if [ $stpMode = 3 ];then
    awk -v exId=$executionId -f output/parse_traces_spbls.awk $outputTracesFilename >> $outputTableStpFilenameRaw
    gnuplot plot_spb_isis.gp
  fi  
  if [ $stpMode = 4 ];then
    # this 'if' avoids accounting for 'MessAgeExp'
    #if [ $failureType = nodecentral ] || [ $failureType = node-corner ] || [ $failureType = nodecore ] || [ $failureType = node-edge ];then
    #  awk '//{if($4 != "MessAgeExp"){print;}else{print $1 " " $2 " " $3 " mess-age-exp " $5}}' $outputTracesFilename > tmp.tmp
    #  cp tmp.tmp $outputTracesFilename
      #rm tmp.tmp
    #fi

    # this 'if' avoids accounting for 'FRW State' 
    #if [ $failureType = nodecentral ] || [ $failureType = node-corner ] || [ $failureType = nodecore ] || [ $failureType = node-edge ];then
    #  awk '/nodeAToFail/{nodeIdFail=$2;} /Installing SPBDV with bridge Id/{if($2==nodeIdFail){treeFail=$10;}} //{if($4 != "FRW" && $5!="State"){print;}else{if($7!=treeFail && $1>11000000000){print;}else{{print $1 " " $2 " " $3 " FRW-State " $6 " " $7 " " $8}}}}' $outputTracesFilename > tmp.tmp
    #  cp tmp.tmp $outputTracesFilename
      #rm tmp.tmp
    #fi

    awk -v exId=$executionId -f output/parse_traces_spbdv.awk $outputTracesFilename >> $outputTableStpFilenameRaw
    gnuplot plot_spb_dv.gp
    echo ""			  
  fi  
  if [ $stpMode = 6 ];then
    awk -v exId=$executionId -f output/parse_traces_spbdv_conf.awk $outputTracesFilename >> $outputTableStpFilenameRaw
    gnuplot plot_spb_dv_conf.gp
    echo ""			  
  fi 
  # print timeline of BPDUs about root 0 (whch becomes old root after root failure)
  #awk '/RSTP: Recvs BPDU \[R:0/{print $1," 1"}' output/n25_grid4_rstp_txHld1_100_rnd_r0_srX_f0-all_0.txt | sed 's/ns/ /' > rootInfoTimeline.txt
  
  # print timeline of BPDUs
  #awk '/RSTP: Recvs BPDU /{print $1," 1"}' output/n25_grid4_rstp_txHld1_100_rnd_r0_srX_f0-all_0.txt | sed 's/ns/ /' > bpdusTimeline.txt
  
  # parse output and add line to output table traffic file
  #if [ $trafficActivate = 1 ];then
  #  awk -f output/traffic_output_single_line.awk $outputTracesFilename >> $outputTableTrafficFilenameRaw
  #fi

  echo -e "PARSING DONE\n"
  echo -e "##########################################################################"

  # copy to dropbox
  #cp $outputTableStpFilenameRaw /home/eduard/Desktop/runs/tmp/output/
  #mv /home/eduard/Desktop/runs/tmp/$outputTableStpFilenameRaw /home/eduard/Desktop/runs/tmp/output/outTableRaw_$computerName-run$testId.txt
  #cp output/spb_dv.png /home/eduard/Desktop/runs/tmp/output/
  #mv /home/eduard/Desktop/runs/tmp/output/spb_dv.png /home/eduard/Desktop/runs/tmp/output/plot_$computerName-run$testId.png
  
  # upload traces to manlleu
  #if [ $computerName = manlleuA ] || [ $computerName = manlleuB ];then
  #  cp output/outTableRaw_spbDv.txt /home/eduard/Desktop/remotes/$computerName\_$testId\_outTableRaw_spbDv.txt;
  #  cp output/spb_dv.png /home/eduard/Desktop/remotes/$computerName\_$testId\_spb_dv.png;    
  #else
  #  scp output/outTableRaw_spbDv.txt eduard@10.80.4.105:/home/eduard/Desktop/remotes/$computerName\_$testId\_outTableRaw_spbDv.txt;
  #  scp output/spb_dv.png eduard@10.80.4.105:/home/eduard/Desktop/remotes/$computerName\_$testId\_spb_dv.png;
  #fi



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

		##########################################################################
		# PROCESS RAW OUTPUT TABLE
		#
			# parse output and add line to output table file
			if [ $stpMode = 1 ];then
        awk -v valueCol=$colToAverage -f output/process_output_table_raw_stp.awk $outputTableStpFilenameRaw >> $outputTableStpFilename
        gnuplot plot_stp.gp
			fi
			#if [ $stpMode = 2 ];then
        #awk -v valueCol=$colToAverage -f output/process_output_table_raw_rstp.awk $outputTableStpFilenameRaw >> $outputTableStpFilename
        #gnuplot plot_rstp.gp			
			#fi  
			if [ $stpMode = 5 ];then
        awk -v valueCol=$colToAverage -f output/process_output_table_raw_rstp.awk $outputTableStpFilenameRaw >> $outputTableStpFilename
        gnuplot plot_rstp.gp			
			fi  
			if [ $stpMode = 3 ];then
        gnuplot plot_spb_isis.gp			  
			fi
			if [ $stpMode = 4 ];then
        awk -v valueCol=$colToAverage -f output/process_output_table_raw_spbdv.awk $outputTableStpFilenameRaw >> $outputTableStpFilename			
        gnuplot plot_spb_dv.gp			  
			fi
			#if [ $stpMode = 6 ];then
        #awk -v valueCol=$colToAverage -f output/process_output_table_raw_spbdv.awk $outputTableStpFilenameRaw >> $outputTableStpFilename			
        #gnuplot plot_spb_dv.gp			  
			#fi
						
			# parse output traffic raw and add line to output table file
      #if [ $trafficActivate = 1 ];then
      #  awk -v valueCol=$colToAverage -f output/process_output_table_raw_traffic.awk $outputTableTrafficFilenameRaw >> $outputTableTrafficFilename
      #  gnuplot plot_traffic.gp			
      #fi
 			
		#  
		# PROCESS RAW OUTPUT TABLE
		##########################################################################

  done #for tcHoldPeriod
  done #for txHoldCount
  #done #for ring_to_full
  done #for nodes
#  
# END OF FOR LOOPS
##########################################################################



##########################################################################
# FINAL SUMMARY REPORT
#
  # some final checkings as a quick summary report
  echo -e "\n#######################################\n#"
  echo -e "\nFINAL REPORT\n"
  
  echo -e "Total number of executions:                 $finalNumExecutions"

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=$(grep -c Done. $f)
    counter=`expr $counter + $tmp` 
  done;
  echo -e "Correctly finished:                         $counter"

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=`awk 'BEGIN{tree=-1}/It is a spanning tree \(initial convergence\)/ {tree=1} /Not a spanning tree yet \(initial convergence\)/ {tree=0} END{print tree}' $f`
    counter=`expr $counter + $tmp` 
  done;
  echo -e "Initial convergence with spanning trees:    $counter"

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=`awk 'BEGIN{sym=-1}/Trees are symmetric \(initial convergence\)/ {sym=1} /Trees are not symmetric \(initial convergence\)/ {sym=0} END{print sym}' $f`
    counter=`expr $counter + $tmp` 
  done;
  echo -e "Initial convergence with symmetric trees:   $counter"

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=`awk 'BEGIN{tree=-1}/It is a spanning tree \(failure convergence\)/ {tree=1} /Not a spanning tree yet \(failure convergence\)/ {tree=0} END{print tree}' $f`
    counter=`expr $counter + $tmp` 
  done;
  echo -e "Failure convergence with spanning trees:    $counter"

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=`awk 'BEGIN{sym=-1}/Trees are symmetric \(failure convergence\)/ {sym=1} /Trees are not symmetric \(failure convergence\)/ {sym=0} END{print sym}' $f`
    counter=`expr $counter + $tmp` 
  done;
  echo -e "Failure convergence with symmetric trees:   $counter"

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=`awk 'BEGIN{full=0}/Queue full/ {full=1} END{print full}' $f`
    counter=`expr $counter + $tmp` 
  done;
  echo -e "\nDroppings due to full queues:               $counter"
      
  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=$(grep -c ERROR $f)
    counter=`expr $counter + $tmp` 
  done;

  if [ $counter != 0 ]
  then
    echo -e "\nERRORS:\n"
    grep ERROR output/*
  else
    echo -e "\nNo ERRORS\n"
  fi

  counter=0
  for f in `ls -tr output/n*`
  do
    tmp=$(grep -c WARNING $f)
    counter=`expr $counter + $tmp` 
  done;

  if [ $counter != 0 ]
  then
    echo -e "WARNINGS:\n"
    grep WARNING output/*
  else
    echo -e "No WARNINGS\n"
  fi



  echo -e "\n#\n#######################################"

  echo -e "\nEND of program\n"

#
# FINAL SUMMARY REPORT
##########################################################################

##########################################################################
# COMPRESSION OF TRACES, END NOTIFICATION
#

  # compress all n* files
  tar -cvf output/$testId.tar output/n*
  gzip output/$testId.tar
    
  # notify the test has finished and send compressed traces to manlleu
  #echo "" > output/finished\_$computerName\_$testId.txt
  if [ $computerName = manlleuA ] || [ $computerName = manlleuB ];then
    #cp output/finished\_$computerName\_$testId.txt /home/eduard/Desktop/remotes/finished\_$computerName\_$testId.txt    
    cp output/$testId.tar.gz ~/Desktop
  else
    #scp output/finished\_$computerName\_$testId.txt eduard@10.80.4.105:/home/eduard/Desktop/remotes/finished\_$computerName\_$testId.txt
    scp output/$testId.tar.gz eduard@10.80.4.105:~/Desktop
  fi
  #rm output/finished\_$computerName\_$testId.txt
  
#
# COMPRESSION OF TRACES, END NOTIFICATION
##########################################################################

