BEGIN{
  iC=0;   # stats index during init convergence
  iP=1;   # stats index between init convergence and failure (periodical)
  fC=2;   # stats index during failure convergence 
  fP=3;   # stats index after failure convergence 
  ind=iC;
  treeFailed=-1;
  
}

#  awk '/nodeAToFail/{nodeIdFail=$2;} /Installing SPBDV with bridge Id/{if($2==nodeIdFail){treeFail=$10;}} //{if($4 != "FRW" && $5!="State"){print;}else{if($7!=treeFail && $1>11000000000){print;}else{{print $1 " " $2 " " $3 " FRW-State " $6 " " $7 " " $8}}}}' $outputTracesFilename > tmp.tmp
    
# key input parameters that vary
/numNodes/          		  { num_nodes=$2} 
/forcedRootNode/   			  { forced_root_node=$2}
/nodeAToFail/     			  { node_a_to_fail=$2} 
/nodeBToFail/        		  { node_b_to_fail=$2; if(node_b_to_fail==-1) nodeFailure=1;}
/txHoldPeriod/            { tx_hold_period=$2 }
/xstpMaxBpduTxHoldPeriod/ { max_tx_hold=$2 }

# detecting the tree that dies
/Installing SPBDV with bridge Id/{
  if(nodeFailure==1)
    {
      if($2==node_a_to_fail) treeFailed=$10;
    }
}
  
# general counters
/Link to port/       { num_ports++;}

# check before/after failure
/EthPointToPointChannel\(\): Failed Link/{
  ind=fC;
  time_link_failure=$1;
}

# BPDU counters
###############
/Sends BPDU with/{
  if($15!="(periodical_BPDUs)") num_bpdus_sent[ind]++; 
}

/Recvs BPDU \[/{
  num_bpdus_rcvd[ind]++;
}

/Recvs BPDU with/{
  if($15!="(periodical_BPDUs)") num_merged_bpdus_rcvd[ind]++;
  if($15!="(periodical_BPDUs)") num_bytes_rcvd[ind] = num_bytes_rcvd[ind] + $11;
}

/Recvs CONF/{
  num_confs_rcvd[ind]++;
}

/periodical_BPDU/{
#  num_bpdus_sent_periodical[ind]++;

  #update final counters at this point 
  num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
  num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
  num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_bytes_rcvd_final[ind]=num_bytes_rcvd[ind];
  num_merged_bpdus_rcvd_final[ind]=  num_merged_bpdus_rcvd[ind];
  num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
  num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
  num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
  num_bpdus_rcvd_worse_agreement_final[ind]=num_bpdus_rcvd_worse_agreement[ind];
  num_bpdus_rcvd_equal_agreement_final[ind]=num_bpdus_rcvd_equal_agreement[ind];
  num_confs_rcvd_final[ind]=num_confs_rcvd[ind];
}

/Holds BPDU/{
  num_bpdus_hold[ind]++;
}

#/pending txHold/          {
#  num_bpdus_sent_txhold[ind]++;
#}

#/maxAge expiration/{
#  num_bpdus_sent_maxage[ind]++;
#}

# RSTP counters (received detail)
#################
/Better from Designated/{
  num_bpdus_rcvd_better[ind]++;
  time_last_better_bpdu[ind]=$1;
  
  #update final counters at this point of potential last better bpdu received 
  num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
  num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
  num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_bytes_rcvd_final[ind]=num_bytes_rcvd[ind];
  num_merged_bpdus_rcvd_final[ind]=  num_merged_bpdus_rcvd[ind];
  num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
  num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
  num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
  num_bpdus_rcvd_worse_agreement_final[ind]=num_bpdus_rcvd_worse_agreement[ind];
  num_bpdus_rcvd_equal_agreement_final[ind]=num_bpdus_rcvd_equal_agreement[ind];
  num_confs_rcvd_final[ind]=num_confs_rcvd[ind];
}

/Worse from Designated/{
  num_bpdus_rcvd_worse[ind]++;
}
  
/Equal from Designated/{
  num_bpdus_rcvd_equal[ind]++;
}

/Worse from Root-Alternate with Agreement/{
  num_bpdus_rcvd_worse_agreement[ind]++;
  time_last_agreement[ind]=$1;
}

/Equal from Root-Alternate with Agreement/{
  num_bpdus_rcvd_equal_agreement[ind]++;
  time_last_agreement[ind]=$1;
}

/Path Vector with duplicated element detected/{
  num_bpdus_discarded_duplicatedPV[ind]++;
}

/BPDU with lower sequence received/{
  num_bpdus_discarded_lowerSeq[ind]++;  
}

/Infinite cost received/{
  num_bpdus_rcvd_infinite_cost[ind]++;
  treeAffectedByFlooding[$8]=1;
}


# COMMON Special events
#######################
/FRW State/{

#printf("ind:%d - $7:%d - nodeA: %d - nodeB: %d\n", ind, $7, node_a_to_fail, node_b_to_fail);

  #print "counted";
  time_last_transition_forwarding[ind]=$1;
  if(treeFailed!=-1 && $7!=treeFailed) time_last_transition_forwarding_not_dead_tree=$1;
  
	#update final counters at this point 
	num_bpdus_sent_final[ind]=num_bpdus_sent[ind];
	num_bpdus_hold_final[ind]=num_bpdus_hold[ind];
	num_bpdus_rcvd_final[ind]=num_bpdus_rcvd[ind];
  num_merged_bpdus_rcvd_final[ind]=  num_merged_bpdus_rcvd[ind];
	num_bytes_rcvd_final[ind]=num_bytes_rcvd[ind];
	num_bpdus_rcvd_better_final[ind]=num_bpdus_rcvd_better[ind];
	num_bpdus_rcvd_worse_final[ind]=num_bpdus_rcvd_worse[ind];
	num_bpdus_rcvd_equal_final[ind]=num_bpdus_rcvd_equal[ind];
	num_bpdus_rcvd_worse_agreement_final[ind]=num_bpdus_rcvd_worse_agreement[ind];
	num_bpdus_rcvd_equal_agreement_final[ind]=num_bpdus_rcvd_equal_agreement[ind];
  num_confs_rcvd_final[ind]=num_confs_rcvd[ind];
}

/Reconfiguring Tree/{
  time_last_algorihtm_reconf[ind] = $1;
}t

/MessAgeExp/{
  if(time_first_mess_age_exp[ind]==0) time_first_mess_age_exp[ind]=$1;
  time_last_mess_age_exp[ind]=$1;
}


END{

# select the latest among last_better_bpdu and last_mess_age_exp as last algorithm step after failure (in some cases there are no better BPDUs received...)
#time_last_algorihtm_reconf[fC]=time_last_better_bpdu[fC];
#if(time_last_algorihtm_reconf[fC]<time_last_mess_age_exp[fC]) time_last_algorihtm_reconf[fC]=time_last_mess_age_exp[fC];

#check if there were no MessAge expirations after failure (so direct link failure detection was activated)
if(time_first_mess_age_exp[fC]==0)
  {
    act2 = 0.0;
    pct2 = 0.0;
  }
else 
  {
    act2 = (time_last_algorihtm_reconf[fC] - time_first_mess_age_exp[fC])/1000000000;
    pct2 = (time_last_transition_forwarding[fC] - time_first_mess_age_exp[fC])/1000000000;
  }

for (i=0;i<num_nodes;i++)
  {
    numTreesAffectedByFlooding = numTreesAffectedByFlooding + treeAffectedByFlooding[i];
  }
  



#         "Id \tNod\tFlA\tFlB\tFrR\tPrt\tHlC\tRMg\tRcv\tCnf\tByt\tHld\tRBD\tRWD\tRED\tRWA\tREA\tACT1 \tPCT1 \tFai  \tSnd\tRcv\tCnf\tByt\tHld\tRBD\tRWD\tRED\tRWA\tREA\tMAf  \tMAl  \tACT2 \tPCT2 \tACT3 \tPCT3\n"
  printf ("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.4f\t%1.4f\t%1.1f\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%2d\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%1.4f\t%2d\t%2d\t%2d\t%2d\n", exId, num_nodes, node_a_to_fail, node_b_to_fail, forced_root_node, num_ports, max_tx_hold, num_merged_bpdus_rcvd_final[iC], num_bpdus_rcvd_final[iC], num_confs_rcvd_final[iC], num_bytes_rcvd_final[iC]/1024, num_bpdus_hold_final[iC], num_bpdus_rcvd_better_final[iC], num_bpdus_rcvd_worse_final[iC], num_bpdus_rcvd_equal_final[iC], num_bpdus_rcvd_worse_agreement_final[iC], num_bpdus_rcvd_equal_agreement_final[iC], time_last_better_bpdu[iC]/1000000000, time_last_transition_forwarding[iC]/1000000000, time_link_failure/1000000000, num_bpdus_sent_final[fC], num_bpdus_rcvd_final[fC], num_confs_rcvd_final[fC], num_bytes_rcvd_final[fC]/1024, num_bpdus_hold_final[fC], num_bpdus_rcvd_better_final[fC], num_bpdus_rcvd_worse_final[fC], num_bpdus_rcvd_equal_final[fC], num_bpdus_rcvd_worse_agreement_final[fC], num_bpdus_rcvd_equal_agreement_final[fC], time_first_mess_age_exp[fC]/1000000000, time_last_mess_age_exp[fC]/1000000000, act2, pct2, (time_last_better_bpdu[fC] - time_link_failure)/1000000000, (time_last_transition_forwarding[fC] - time_link_failure)/1000000000, (time_last_transition_forwarding_not_dead_tree - time_link_failure)/1000000000,numTreesAffectedByFlooding, num_bpdus_rcvd_infinite_cost[fC], num_bpdus_discarded_lowerSeq[fC], num_bpdus_discarded_duplicatedPV[fC]);
}

