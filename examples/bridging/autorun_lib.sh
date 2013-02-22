#!/bin/bash



##########################################################################
# FUNCTION WriteHeaderOutputTableFilename
#
function WriteHeaderOutputTableStp () {

  local stpMode=$1
  local outputTableFilename=$2

  if [ -f $outputTableFilename ]
  then
    echo -e "ERROR: file '$outputTableFilename' already exists! Overwrite? (y/n)"
    read answer
    if [ $answer != "y" ]
    then exit
    fi
  fi

  #print input parameters in the output table file
  #echo -e "# numNodes=                      -" > $outputTableFilename
  echo -e "# topologyType=                  $topologyType" >> $outputTableFilename
  echo -e "# topologyDegree=                $topologyDegree" >> $outputTableFilename
  echo -e "# linkDatarateBps=               $linkDatarateBps" >> $outputTableFilename
  echo -e "# linkDelayUs=                   $linkDelayUs" >> $outputTableFilename
  echo -e "# activateFailure=               $activateFailure" >> $outputTableFilename
  echo -e "# activatePhyFailDetect=         $activatePhyFailDetect" >> $outputTableFilename
  echo -e "# linkFailureTimeUs=             $linkFailureTimeUs" >> $outputTableFilename
  #echo -e "# nodeAToFail=                   $nodeAToFail" >> $outputTableFilename
  #echo -e "# nodeBToFail=                   $nodeBToFail" >> $outputTableFilename
  echo -e "# stpMode=                       $stpMode" >> $outputTableFilename
  echo -e "# randomBridgeIds=               $randomBridgeIds" >> $outputTableFilename
  #echo -e "# forcedRootNode=                $forcedRootNode" >> $outputTableFilename
  echo -e "# xstpSimulationTimeSecs=        $xstpSimulationTimeSecs" >> $outputTableFilename
  echo -e "# xstpActivatePeriodicalBpdus=   $xstpActivatePeriodicalBpdus" >> $outputTableFilename
  echo -e "# xstpActivateExpirationMaxAge=  $xstpActivateExpirationMaxAge" >> $outputTableFilename
  echo -e "# xstpActivateIncreaseMessAge=   $xstpActivateIncreaseMessAge" >> $outputTableFilename
  echo -e "# xstpMaxAge=                    $xstpMaxAge" >> $outputTableFilename
  echo -e "# xstpHelloTime=                 $xstpHelloTime" >> $outputTableFilename
  echo -e "# xstpForwDelay=                 $xstpForwDelay" >> $outputTableFilename
  #echo -e "# xstpTxHoldPeriod=              $xstpTxHoldPeriod" >> $outputTableFilename
  #echo -e "# xstpMaxBpduTxHoldPeriod=       $xstpMaxBpduTxHoldPeriod"  >> $outputTableFilename
  echo -e "# xstpResetTxHoldPeriod=         $xstpResetTxHoldPeriod"  >> $outputTableFilename
  echo -e "# activateRandBridgeInit=        $activateRandBridgeInit"  >> $outputTableFilename
  echo -e "" >> $outputTableFilename

  #print header info of the output table file (mode:STP)
  if [ $stpMode = 1 ];then
    echo -e "# Id:  Execution Id" >> $outputTableFilename
    echo -e "# Nod: Number of nodes" >> $outputTableFilename
    echo -e "# FlA: Node A to fail" >> $outputTableFilename
    echo -e "# FlB: Node B to fail" >> $outputTableFilename
    echo -e "# FrR: Node forced as root (bridge id set to 0)" >> $outputTableFilename
    echo -e "# Prt: Number of ports (square of the number of links)" >> $outputTableFilename
    echo -e "# HlP: TxHold period" >> $outputTableFilename
    echo -e "# HlC: Maximum Number of BPDUs to be send per TxHold period" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs" >> $outputTableFilename
    #echo -e "# SBe: Send after disseminate better BPDU" >> $outputTableFilename
    #echo -e "# SWo: Send after reply worse BPDU" >> $outputTableFilename
    #echo -e "# SEq: Send after disseminate equal BPDU" >> $outputTableFilename
    #echo -e "# SPr: Send because of periodical bpdu" >> $outputTableFilename
    #echo -e "# SHl: Send after clearing txHold and there were pending" >> $outputTableFilename
    #echo -e "# SEx: Send because of max age expiration" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure" >> $outputTableFilename
    echo -e "# PCT1: Protocol convergence time (last port transition to forwarding) before failure" >> $outputTableFilename
    echo -e "# Fai: Time when link fails" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs (after failure)" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent (after failure)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs (after failure)" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received (after failure)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received (after failure)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received (after failure)" >> $outputTableFilename   
    echo -e "# MAf: First expiration of Message Age" >> $outputTableFilename
    echo -e "# MAl: Last expiration of Message Age" >> $outputTableFilename
    echo -e "# ACT2: Algorithm convergence Time (STP: last better BPDU) after last mess age expiration" >> $outputTableFilename
    echo -e "# PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration" >> $outputTableFilename
    echo -e "# ACT3: Algorithm convergence Time (STP: last better BPDU) after failure" >> $outputTableFilename
    echo -e "# PCT3: Protocol convergence time (last port transition to forwarding) after failure" >> $outputTableFilename

    echo -e "\n#Id\tNod\tFlA\tFlB\tFrR\tPrt\tHlP\tHlC\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tACT1\tPCT1\tFai\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tMAf\tMAl\tACT2\tPCT2\tACT3\tPCT3" >> $outputTableFilename  
    #echo -e "\nNod\tFlA\tFlB\tFrR\tPrt\tTxH\tSnd\tSBe\tSWo\tSEq\tSPr\tSHl\tSEx\tHld\tRcv\tRBD\tRWD\tRED\tRWA\tREA\tACT1\tPCT1\tFai\tMAf\tMAl\tACT2\tPCT2\tACT3\tPCT3" >> $outputTableFilename  
  fi

  #print header info of the output table file (mode:RSTP)
  if [ $stpMode = 2 ];then
    echo -e "# Id:  Execution Id" >> $outputTableFilename
    echo -e "# Nod: Number of nodes" >> $outputTableFilename
    echo -e "# FlA: Node A to fail" >> $outputTableFilename
    echo -e "# FlB: Node B to fail" >> $outputTableFilename
    echo -e "# FrR: Node forced as root (bridge id set to 0)" >> $outputTableFilename
    echo -e "# Prt: Number of ports (square of the number of links)" >> $outputTableFilename
    echo -e "# HlP: TxHold period" >> $outputTableFilename
    echo -e "# HlC: Maximum Number of BPDUs to be send per TxHold period" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs" >> $outputTableFilename
    #echo -e "# SBe: Send after disseminate better BPDU" >> $outputTableFilename
    #echo -e "# SWo: Send after reply worse BPDU" >> $outputTableFilename
    #echo -e "# SEq: Send after disseminate equal BPDU" >> $outputTableFilename
    #echo -e "# SPr: Send because of periodical bpdu" >> $outputTableFilename
    #echo -e "# SHl: Send after clearing txHold and there were pending" >> $outputTableFilename
    #echo -e "# SEx: Send because of max age expiration" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RDA: Discrded agreement because it is about a different Root" >> $outputTableFilename
    #echo -e "# REA: Worse BPDU with Agreement received (only RSTP, from Root/Alternate)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received (only RSTP, from Root/Alternate)" >> $outputTableFilename
    echo -e "# ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure" >> $outputTableFilename
    echo -e "# PCT1: Protocol convergence time (last port transition to forwarding) before failure" >> $outputTableFilename
    echo -e "# Fai: Time when link fails" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs (after failure)" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent (after failure)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs (after failure)" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received from Designated (after failure)" >> $outputTableFilename
    #echo -e "# REA: Worse BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# RDA: Discrded agreement because it is about a different Root (after failure)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# MAf: First expiration of Message Age" >> $outputTableFilename
    echo -e "# MAl: Last expiration of Message Age" >> $outputTableFilename
    echo -e "# ACT2: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after last mess age expiration" >> $outputTableFilename
    echo -e "# PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration" >> $outputTableFilename
    echo -e "# ACT3: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after failure" >> $outputTableFilename
    echo -e "# PCT3: Protocol convergence time (last port transition to forwarding) after failure" >> $outputTableFilename

    echo -e "\n#Id\tNod\tFlA\tFlB\tFrR\tPrt\tHlP\tHlC\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tRWA\tRDA\tACT1\tPCT1\tFai\tSnd\tHld\tRcv\tRBD\tRWD\tRED\tRWA\tRDA\tMAf\tMAl\tACT2\tPCT2\tACT3\tPCT3" >> $outputTableFilename  
  fi

  #print header info of the output table file (mode:RSTPconf)
  if [ $stpMode = 5 ];then
    echo -e "# Id:  Execution Id" >> $outputTableFilename
    echo -e "# Nod: Number of nodes" >> $outputTableFilename
    echo -e "# FlA: Node A to fail" >> $outputTableFilename
    echo -e "# FlB: Node B to fail" >> $outputTableFilename
    echo -e "# FrR: Node forced as root (bridge id set to 0)" >> $outputTableFilename
    echo -e "# Prt: Number of ports (square of the number of links)" >> $outputTableFilename
    echo -e "# HlP: TxHold period" >> $outputTableFilename
    echo -e "# HlC: Maximum Number of BPDUs to be send per TxHold period" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs" >> $outputTableFilename
    #echo -e "# SBe: Send after disseminate better BPDU" >> $outputTableFilename
    #echo -e "# SWo: Send after reply worse BPDU" >> $outputTableFilename
    #echo -e "# SEq: Send after disseminate equal BPDU" >> $outputTableFilename
    #echo -e "# SPr: Send because of periodical bpdu" >> $outputTableFilename
    #echo -e "# SHl: Send after clearing txHold and there were pending" >> $outputTableFilename
    #echo -e "# SEx: Send because of max age expiration" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs" >> $outputTableFilename
    echo -e "# Cnf: Total Received BPDUconfs" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received (from Designated in RSTP)" >> $outputTableFilename
    echo -e "# RDA: Discrded agreement because it is about a different Root" >> $outputTableFilename
    #echo -e "# REA: Worse BPDU with Agreement received (only RSTP, from Root/Alternate)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received (only RSTP, from Root/Alternate)" >> $outputTableFilename
    echo -e "# ACT1: Algorithm convergence Time (STP: last better BPDU) (RSTP: last better BPDU or last agreement) before failure" >> $outputTableFilename
    echo -e "# PCT1: Protocol convergence time (last port transition to forwarding) before failure" >> $outputTableFilename
    echo -e "# Fai: Time when link fails" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs (after failure)" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent (after failure)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs (after failure)" >> $outputTableFilename
    echo -e "# Cnf: Total Received BPDUconfs (after failure)" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received from Designated (after failure)" >> $outputTableFilename
    #echo -e "# REA: Worse BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# RDA: Discrded agreement because it is about a different Root (after failure)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# MAf: First expiration of Message Age" >> $outputTableFilename
    echo -e "# MAl: Last expiration of Message Age" >> $outputTableFilename
    echo -e "# ACT2: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after last mess age expiration" >> $outputTableFilename
    echo -e "# PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration" >> $outputTableFilename
    echo -e "# ACT3: Algorithm convergence Time (RSTP: last better BPDU or last agreement) after failure" >> $outputTableFilename
    echo -e "# PCT3: Protocol convergence time (last port transition to forwarding) after failure" >> $outputTableFilename

    echo -e "\n#Id\tNod\tFlA\tFlB\tFrR\tPrt\tHlP\tHlC\tSnd\tHld\tRcv\tCnf\tRBD\tRWD\tRED\tRWA\tRDA\tACT1\tPCT1\tFai\tSnd\tHld\tRcv\tCnf\tRBD\tRWD\tRED\tRWA\tRDA\tMAf\tMAl\tACT2\tPCT2\tACT3\tPCT3" >> $outputTableFilename  
  fi
  #print header info of the output table file (mode:SPB)
  if [ $stpMode = 3 ];then
    echo -e "# Id:  Execution Id" >> $outputTableFilename
    echo -e "# Nod: Number of nodes" >> $outputTableFilename
    echo -e "# FlA: Node A to fail" >> $outputTableFilename
    echo -e "# FlB: Node B to fail" >> $outputTableFilename
    echo -e "# FrR: Node forced as root (bridge id set to 0)" >> $outputTableFilename
    echo -e "# Prt: Number of ports (square of the number of links)" >> $outputTableFilename
    echo -e "# TxH: TxHold period (seconds)" >> $outputTableFilename
    #echo -e "# SdH: Total Send HELLOs (before failure)" >> $outputTableFilename
    #echo -e "# RvH: Total Received HELLOs (before failure)" >> $outputTableFilename
    echo -e "# SdL: Total Send LSPs (before failure)" >> $outputTableFilename
    echo -e "# RvL: Total Received LSPs (before failure)" >> $outputTableFilename
    echo -e "# HdL: Total Held LSPs (before failure)" >> $outputTableFilename
    echo -e "# Lbe: Received LSPs carrying a higher sequence number (before failure)" >> $outputTableFilename
    echo -e "# Lwo: Received LSPs carrying a lower sequence number (before failure)" >> $outputTableFilename
    echo -e "# Leq: Received LSPs carrying an equal sequence number (before failure)" >> $outputTableFilename
    echo -e "# SdB: Total Send BPDUs (before failure)" >> $outputTableFilename
    echo -e "# RvB: Total Received BPDUs (before failure)" >> $outputTableFilename
    echo -e "# KB : Total Received KBs (before failure)" >> $outputTableFilename
    #echo -e "# BCo: Received BPDUs carrying a contract (before failure)" >> $outputTableFilename
    #echo -e "# BAg: Received BPDUs carrying an agreement (before failure)" >> $outputTableFilename    
    echo -e "# Cmp: Number of topology computations (same as RvL) (before failure)" >> $outputTableFilename
    echo -e "# ACT1: Time of last topology computation" >> $outputTableFilename
    echo -e "# PCT1: Time of last transition to FRW" >> $outputTableFilename
    echo -e "# Fai: Time when link fails" >> $outputTableFilename
    echo -e "# SdL: Total Send LSPs (after failure)" >> $outputTableFilename
    echo -e "# RvL: Total Received LSPs (after failure)" >> $outputTableFilename
    echo -e "# HdL: Total Held LSPs (after failure)" >> $outputTableFilename
    echo -e "# Lbe: Received LSPs carrying a higher sequence number (after failure)" >> $outputTableFilename
    echo -e "# Lwo: Received LSPs carrying a lower sequence number (after failure)" >> $outputTableFilename
    echo -e "# Leq: Received LSPs carrying an equal sequence number (after failure)" >> $outputTableFilename
    echo -e "# SdB: Total Send BPDUs (after failure)" >> $outputTableFilename
    echo -e "# RvB: Total Received BPDUs (after failure)" >> $outputTableFilename
    echo -e "# KB : Total Received KBs (after failure)" >> $outputTableFilename
    #echo -e "# BCo: Received BPDUs carrying a contract (after failure)" >> $outputTableFilename
    #echo -e "# BAg: Received BPDUs carrying an agreement (after failure)" >> $outputTableFilename 
    echo -e "# ACT2: Time of last topology computation (after failure)" >> $outputTableFilename
    echo -e "# PCT2: Time of last transition to FRW (after failure)" >> $outputTableFilename
    echo -e "\n#Id\tNod\tFlA\tFlB\tFrR\tPrt\tTxH\tSdL\tRvL\tHdL\tLbe\tLwo\tLeq\tSdB\tRvB\tKB \tCmp\tACT1\tPCT1\tFai\tSdL\tRvL\tHdL\tLbe\tLwo\tLeq\tSdB\tRvB\tKB \tACT2\tPCT2" >> $outputTableFilename  
  fi

  #print header info of the output table file (mode:SPBDV)
  if [ $stpMode = 4 ];then
    echo -e "# Id:  Execution Id" >> $outputTableFilename
    echo -e "# Nod: Number of nodes" >> $outputTableFilename
    echo -e "# FlA: Node A to fail" >> $outputTableFilename
    echo -e "# FlB: Node B to fail" >> $outputTableFilename
    echo -e "# FrR: Node forced as root (bridge id set to 0)" >> $outputTableFilename
    echo -e "# Prt: Number of ports (square of the number of links)" >> $outputTableFilename
    echo -e "# HlP: TxHold period" >> $outputTableFilename
    #echo -e "# HlC: Maximum Number of BPDUs to be send per TxHold period" >> $outputTableFilename
    #echo -e "# Snd: Total Send BPDUs" >> $outputTableFilename
    echo -e "# RMg: Total Received Merged BPDUs (merged BPDUs)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs" >> $outputTableFilename
    echo -e "# KB : Total Received BPDUs (Kbytes)" >> $outputTableFilename
    #echo -e "# SBe: Send after disseminate better BPDU" >> $outputTableFilename
    #echo -e "# SWo: Send after reply worse BPDU" >> $outputTableFilename
    #echo -e "# SEq: Send after disseminate equal BPDU" >> $outputTableFilename
    #echo -e "# SPr: Send because of periodical bpdu" >> $outputTableFilename
    #echo -e "# SHl: Send after clearing txHold and there were pending" >> $outputTableFilename
    #echo -e "# SEx: Send because of max age expiration" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received (from Designated in SPBDV)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received (from Designated in SPBDV)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received (from Designated in SPBDV)" >> $outputTableFilename
    echo -e "# REA: Worse BPDU with Agreement received (only SPBDV, from Root/Alternate)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received (only SPBDV, from Root/Alternate)" >> $outputTableFilename
    echo -e "# ACT1: Algorithm convergence Time (STP: last better BPDU) (SPBDV: last better BPDU or last agreement) before failure" >> $outputTableFilename
    echo -e "# PCT1: Protocol convergence time (last port transition to forwarding) before failure" >> $outputTableFilename
    echo -e "# Fai: Time when link fails" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs without counting periodicals (after failure)" >> $outputTableFilename
    #echo -e "# RMg: Total Received Merged BPDUs (merged BPDUs)(after failure)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs (after failure)" >> $outputTableFilename
    echo -e "# KB : Total Received BPDUs (Kbytes)" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent (after failure)" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# REA: Worse BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# MAf: First expiration of Message Age" >> $outputTableFilename
    echo -e "# MAl: Last expiration of Message Age" >> $outputTableFilename
    echo -e "# ACT2: Algorithm convergence Time (SPBDV: last better BPDU or last agreement) after last mess age expiration" >> $outputTableFilename
    echo -e "# PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration" >> $outputTableFilename
    echo -e "# ACT3: Algorithm convergence Time (SPBDV: last better BPDU or last agreement) after failure" >> $outputTableFilename
    echo -e "# PCT3: Protocol convergence time (last port transition to forwarding) after failure" >> $outputTableFilename
    echo -e "# PCT4: Protocol convergence time (last port transition to forwarding) after failure only accounting not dead trees" >> $outputTableFilename
    echo -e "# AffT: Number of affected trees (issueing a flooding)" >> $outputTableFilename
    echo -e "# RInf: Received with infinite cost" >> $outputTableFilename
    echo -e "# LwSq: Discarded BPDUs due to lower sequence number" >> $outputTableFilename
    echo -e "# Dupl: Discarded BPDUs due to duplicated path vector" >> $outputTableFilename


    echo -e "\n#Id\tNod\tFlA\tFlB\tFrR\tPrt\tHlC\tRMg\tRcv\tKB \tHld\tRBD\tRWD\tRED\tRWA\tREA\tACT1\tPCT1\tFai\tSnd\tRcv\tKB \tHld\tRBD\tRWD\tRED\tRWA\tREA\tMAf\tMAl\tACT2\tPCT2\tACT3\tPCT3\tPCT4\tAffT\tRinf\tLwSq\tDupl" >> $outputTableFilename  
  fi

  if [ $stpMode = 6 ];then
    echo -e "# Id:  Execution Id" >> $outputTableFilename
    echo -e "# Nod: Number of nodes" >> $outputTableFilename
    echo -e "# FlA: Node A to fail" >> $outputTableFilename
    echo -e "# FlB: Node B to fail" >> $outputTableFilename
    echo -e "# FrR: Node forced as root (bridge id set to 0)" >> $outputTableFilename
    echo -e "# Prt: Number of ports (square of the number of links)" >> $outputTableFilename
    echo -e "# HlP: TxHold period" >> $outputTableFilename
    #echo -e "# HlC: Maximum Number of BPDUs to be send per TxHold period" >> $outputTableFilename
    #echo -e "# Snd: Total Send BPDUs" >> $outputTableFilename
    echo -e "# RMg: Total Received Merged BPDUs (merged BPDUs)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs" >> $outputTableFilename
    echo -e "# Cnf: Total Received BPDUconfs" >> $outputTableFilename
    echo -e "# KB : Total Received BPDUs (Kbytes)" >> $outputTableFilename
    #echo -e "# SBe: Send after disseminate better BPDU" >> $outputTableFilename
    #echo -e "# SWo: Send after reply worse BPDU" >> $outputTableFilename
    #echo -e "# SEq: Send after disseminate equal BPDU" >> $outputTableFilename
    #echo -e "# SPr: Send because of periodical bpdu" >> $outputTableFilename
    #echo -e "# SHl: Send after clearing txHold and there were pending" >> $outputTableFilename
    #echo -e "# SEx: Send because of max age expiration" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received (from Designated in SPBDV)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received (from Designated in SPBDV)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received (from Designated in SPBDV)" >> $outputTableFilename
    echo -e "# REA: Worse BPDU with Agreement received (only SPBDV, from Root/Alternate)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received (only SPBDV, from Root/Alternate)" >> $outputTableFilename
    echo -e "# ACT1: Algorithm convergence Time (STP: last better BPDU) (SPBDV: last better BPDU or last agreement) before failure" >> $outputTableFilename
    echo -e "# PCT1: Protocol convergence time (last port transition to forwarding) before failure" >> $outputTableFilename
    echo -e "# Fai: Time when link fails" >> $outputTableFilename
    echo -e "# Snd: Total Send BPDUs without counting periodicals (after failure)" >> $outputTableFilename
    #echo -e "# RMg: Total Received Merged BPDUs (merged BPDUs)(after failure)" >> $outputTableFilename
    echo -e "# Rcv: Total Received BPDUs (after failure)" >> $outputTableFilename
    echo -e "# Cnf: Total Received BPDUconfs (after failure)" >> $outputTableFilename
    echo -e "# KB : Total Received BPDUs (Kbytes)" >> $outputTableFilename
    echo -e "# Hld: Hold BPDUs that had to be sent (after failure)" >> $outputTableFilename
    echo -e "# RBD: Better BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RWD: Worse BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# RED: Equal BPDU received from Designated (after failure)" >> $outputTableFilename
    echo -e "# REA: Worse BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# RWA: Equal BPDU with Agreement received from Root/Alternate (after failure)" >> $outputTableFilename
    echo -e "# MAf: First expiration of Message Age" >> $outputTableFilename
    echo -e "# MAl: Last expiration of Message Age" >> $outputTableFilename
    echo -e "# ACT2: Algorithm convergence Time (SPBDV: last better BPDU or last agreement) after last mess age expiration" >> $outputTableFilename
    echo -e "# PCT2: Protocol convergence time (last port transition to forwarding) after last mess age expiration" >> $outputTableFilename
    echo -e "# ACT3: Algorithm convergence Time (SPBDV: last better BPDU or last agreement) after failure" >> $outputTableFilename
    echo -e "# PCT3: Protocol convergence time (last port transition to forwarding) after failure" >> $outputTableFilename
    echo -e "# PCT4: Protocol convergence time (last port transition to forwarding) after failure only accounting not dead trees" >> $outputTableFilename
    echo -e "# AffT: Number of affected trees (issueing a flooding)" >> $outputTableFilename
    echo -e "# RInf: Received with infinite cost" >> $outputTableFilename
    echo -e "# LwSq: Discarded BPDUs due to lower sequence number" >> $outputTableFilename
    echo -e "# Dupl: Discarded BPDUs due to duplicated path vector" >> $outputTableFilename


    echo -e "\n#Id\tNod\tFlA\tFlB\tFrR\tPrt\tHlC\tRMg\tRcv\tCnf\tKB \tHld\tRBD\tRWD\tRED\tRWA\tREA\tACT1\tPCT1\tFai\tSnd\tRcv\tCnf\tKB \tHld\tRBD\tRWD\tRED\tRWA\tREA\tMAf\tMAl\tACT2\tPCT2\tACT3\tPCT3\tPCT4\tAffT\tRinf\tLwSq\tDupl" >> $outputTableFilename  
  fi
}
 
#
# FUNCTION WriteHeaderOutputTableFilename
##########################################################################



##########################################################################
# FUNCTION WriteHeaderOutputTableTraffic
#
function WriteHeaderOutputTableTraffic() {

  local outputTableFilename=$1
  
  echo -e "# Nod:   Number of nodes" >> $outputTableFilename  
  echo -e "# NGi:   Number of generated packets per second during interval i" >> $outputTableFilename  
  echo -e "# NRi:   Number of received packets per second during interval i" >> $outputTableFilename  
  echo -e "# Li:    Length in seconds of interval i" >> $outputTableFilename   
  echo -e "# R4/R3: Ratio between received packets in intervals 3 and 4 (measure of network partition)" >> $outputTableFilename    
  echo -e "# R5/R3: Ratio between received packets in intervals 3 and 5 (measure of old/new topologies similarity)" >> $outputTableFilename      
  echo -e "\n#Nod\tNG1\tNR1\tL1\tNG2\tNR2\tL2\tNG3\tNR3\tL3\tNG4\tNR4\tL4\tNG5\tNR5\tL5\tNG6\tNR6\tL6\tNG7\tNR7\tL7\tR4/R3\tR5/R3" >> $outputTableFilename  
}



