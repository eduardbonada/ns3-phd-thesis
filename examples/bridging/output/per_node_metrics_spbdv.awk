# Computation of metrics per node - spbdv
# input parameters
# - sortByNodeIdInParam
# - sortByBridgeIdInParam
# - convTimeAsLastFrwInParam
# - convTimeAsLastBpduInParam
# - coldStartInParam
# - linkFailureInParam
# - nodeFailureInParam

# use from output folder:
# rm spbdv_metrics_per_node.txt; x=0; y=0; for (( n=0; n<64; n++ )); do awk  -v xPos=$x -v yPos=$y -v tree=$n -v sortByNodeIdInParam=1 -v sortByBridgeIdInParam=0 -v convTimeAsLastFrwInParam=1 -v convTimeAsLastBpduInParam=0 -v coldStartInParam=1 -v linkFailureInParam=0 -v nodeFailureInParam=0 -f per_node_metrics_spbdv.awk nXX... >> spbdv_metrics_per_node.txt; y=`expr $y + 1`;  if [ $y = 8 ];then x=`expr $x + 1`; y=0; fi; done;


BEGIN{
  execution=0;
  capture=0;
  readBetterFromDes=0;
    
  sortByNodeId=sortByNodeIdInParam;
  sortByBridgeId=sortByBridgeIdInParam;
  
  convTimeAsLastFrw=convTimeAsLastFrwInParam;
  convTimeAsLastBpdu=convTimeAsLastBpduInParam;
  
  coldStart=coldStartInParam;
  linkFailure=linkFailureInParam;
  nodeFailure=nodeFailureInParam;
  rootFailure=rootFailureInParam;

  if(coldStart==1) capture=1;
}

# key input parameters that vary
/numNodes/          		  { num_nodes=$2} 
/nodeAToFail/     			  { node_a_to_fail=$2; if(coldStart==1) node_a_to_fail=-1;} 
/nodeBToFail/        		  { node_b_to_fail=$2; if(coldStart==1) node_b_to_fail=-1;}

# check before/after failure
/EthPointToPointChannel\(\): Failed Link/{
  if(linkFailure==1 || nodeFailure==1) capture=1;
  if(coldStart==1 ) capture=0; 
  time_link_failure=$1;
}

# Match nodeId<=>bridgeId
#########################
/Installing SPBDV with bridge Id/{
  bridgeId2nodeId[$10]=$2;
  nodeId2bridgeId[$2]=$10;
}  

# BPDU counters
###############
/Recvs BPDU \[R:/{
  if(capture==1)
    {
      split($6,c,"|");
			split(c[1],d,":"); 
			if(bridgeId2nodeId[d[2]]==tree || tree==-1)
				{
					split($2,a,"[");
					split(a[2],b,"]");
					num_bpdus_rcvd[bridgeId2nodeId[b[1]]]++;
					total_bpdus_rcvd++;
					readBetterFromDes=1;
				}
			else
				{
				  readBetterFromDes=0;
				}
    }
}

/Better from Designated/{
  if(capture==1)
    {
			if(readBetterFromDes==1)
				{
					split($2,a,"[");
					split(a[2],b,"]");
					num_bpdus_rcvd_better[bridgeId2nodeId[b[1]]]++;
					total_bpdus_rcvd_better++;

					time_last_better_bpdu[bridgeId2nodeId[b[1]]]=$1;
					total_time_last_better_bpdu=$1;  
		
					#update final counters at this point of potential last better bpdu received 
					for( n = 0; n < num_nodes; n++ )
						{
							num_bpdus_rcvd_final[n]=num_bpdus_rcvd[n];
							total_bpdus_rcvd_final=total_bpdus_rcvd;      
							num_bpdus_rcvd_better_final[n]=num_bpdus_rcvd_better[n];
							total_bpdus_rcvd_better_final=total_bpdus_rcvd_better;      
						}
				}
    }
}

/Periodical BPDUs/{ 
  if(coldStart==1 && $1!="0ns" )
    {
      capture=0; 
			#update final counters at this point of potential last better bpdu received 
			for( n = 0; n < num_nodes; n++ )
				{
					num_bpdus_rcvd_final[n]=num_bpdus_rcvd[n];
					total_bpdus_rcvd_final=total_bpdus_rcvd;      
					num_bpdus_rcvd_better_final[n]=num_bpdus_rcvd_better[n];
					total_bpdus_rcvd_better_final=total_bpdus_rcvd_better;         
				}
    }
    
  # update final variables only once right after the failure recovery
  if(linkFailure==1 || nodeFailure==1)
    {
			if($1=="4000000000ns" && $2=="Brdg[0]:Port[-]")
				{
					#update final counters at this point of potential last better bpdu received 
					for( n = 0; n < num_nodes; n++ )
						{
							num_bpdus_rcvd_final[n]=num_bpdus_rcvd[n];
							total_bpdus_rcvd_final=total_bpdus_rcvd;      
							num_bpdus_rcvd_better_final[n]=num_bpdus_rcvd_better[n];
							total_bpdus_rcvd_better_final=total_bpdus_rcvd_better;      
						}      
				}
	  }
} 

# COMMON Special events
#######################
/FRW State/{
  if(capture==1)
    {
			if(bridgeId2nodeId[$7]==tree || tree==-1)
				{
				  split($2,a,"[");
				  split(a[2],b,"]");
				  time_last_transition_forwarding[bridgeId2nodeId[b[1]]]=$1;
				  total_time_last_transition_forwarding=$1;
				}
    }
}


END{
  ###########################
  # select last event as CT #
  ###########################
  if(coldStart==1)
    {
			for( n=0; n < num_nodes; n++ )
				{
				  #CT as last forwarding    
				  if(convTimeAsLastFrw==1) ct[n]=time_last_transition_forwarding[n];
				  #CT as last BPDU        
				  if(convTimeAsLastBpdu==1) ct[n]=time_last_better_bpdu[n];
				}
			#CT as last forwarding    
			if(convTimeAsLastFrw==1) total_ct=total_time_last_transition_forwarding;
			#CT as last BPDU        
			if(convTimeAsLastBpdu==1) total_ct=total_time_last_better_bpdu;  
    }
  if(linkFailure==1 || nodeFailure==1)
		{
			for( n=0; n < num_nodes; n++ )
				{ 
				  #CT as last forwarding    
				  if(convTimeAsLastFrw==1) ct[n]=time_last_transition_forwarding[n]-time_link_failure;
				  #CT as last BPDU    
				  if(convTimeAsLastBpdu==1) ct[n]=time_last_better_bpdu[n]-time_link_failure;
				  if(ct[n]<0) ct[n]=0;
				}

			#CT as last forwarding    
			if(convTimeAsLastFrw==1) total_ct=total_time_last_transition_forwarding-time_link_failure;
			#CT as last BPDU    
			if(convTimeAsLastBpdu==1) total_ct=total_time_last_better_bpdu-time_link_failure;
			if(total_ct<0) total_ct=0;
    }  

  #######################
  # print first columns #
  #######################  
  #printf("# Xposition Yposition | tree   executionId | nodeAFail   nodeBFail | Total_CT [-- %d CTs per nodeId --] | Total_Mess [-- %d message overheads per nodeId --] | Total_Trigg [-- %d triggers per nodeId --]\n", num_nodes, num_nodes, num_nodes);
  #printf("# FailA-Xposition FailB-Yposition | forcedRoot   executionId | nodeAFail   nodeBFail | Total_CT [-- %d CTs per nodeId --] | Total_Mess [-- %d message overheads per nodeId --] | Total_Trigg [-- %d triggers per nodeId --]\n", num_nodes, num_nodes, num_nodes);

  printf("%2d\t",xPos);
  printf("%2d\t",yPos);
  
  printf("%2d\t",tree);
  printf("%2d\t",execution);

  printf("%2d\t",node_a_to_fail);
  printf("%2d\t",node_b_to_fail);


  ###################
  # print conv time #
  ################### 
  printf("%0.4f\t",total_ct/100000);
  for( n=0; n < num_nodes; n++ )
    {
      # sort by nodeId
      if(sortByNodeId==1) printf("%0.4f\t",ct[n]/100000);

      # sort by brigdeId
      if(sortByBridgeId==1) printf("%0.4f\t",ct[bridgeId2nodeId[n]]/100000);
    }
    
  ##################
  # print messages #
  ################## 
  printf("%2d\t",total_bpdus_rcvd_final);
  for( n=0; n < num_nodes; n++ )
    {
      # sort by nodeId
      if(sortByNodeId==1) printf("%2d\t",num_bpdus_rcvd_final[n]);

      # sort by brigdeId      
      if(sortByBridgeId==1) printf("%2d\t",num_bpdus_rcvd_final[bridgeId2nodeId[n]]);
    }


  ##################
  # print triggers #
  ##################  
  printf("%2d\t",total_bpdus_rcvd_better_final);
  for( n=0; n < num_nodes; n++ )
    {
      # sort by nodeId
      if(sortByNodeId==1) printf("%2d\t",num_bpdus_rcvd_better_final[n]);

      # sort by brigdeId
      if(sortByBridgeId==1) printf("%2d\t",num_bpdus_rcvd_better_final[bridgeId2nodeId[n]]);
    }
  printf("\n");
}

