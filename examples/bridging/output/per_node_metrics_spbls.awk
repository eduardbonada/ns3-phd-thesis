# Computation of metrics per node - spbdv
# input parameters
# - convTimeAsLastFrwInParam
# - convTimeAsLastLspInParam
# - coldStartInParam
# - linkFailureInParam
# - nodeFailureInParam

# use from output folder:
# rm spbls_metrics_per_nodeId.txt; 	awk -v convTimeAsLastFrwInParam=1 -v convTimeAsLastLspInParam=0 -v coldStartInParam=$coldStart -v linkFailureInParam=$linkFailure -v nodeFailureInParam=$nodeFailure -f per_node_metrics_spbls.awk $f >> spbls_metrics_per_nodeId.txt;


BEGIN{
  execution=0;
  capture=0;
  readBetterFromDes=0;
    
  sortByNodeId=sortByNodeIdInParam;
  sortByBridgeId=sortByBridgeIdInParam;
  
  convTimeAsLastFrw=convTimeAsLastFrwInParam;
  convTimeAsLastLsp=convTimeAsLastLspInParam;
  
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
  time_link_failure=$1;
}

# Match nodeId<=>bridgeId
#########################
/Installing SPB-ISIS with bridge Id/{
  split($1,a,"[");
  split(a[2],b,"]");

  bridgeId2nodeId[$8]=b[1];
  nodeId2bridgeId[b[1]]=$8;
}  

# LSP counters
###############
/SPBISIS: Recvs LSP/{
  if(capture==1)
    {
			split($2,a,"[");
			split(a[2],b,"]");
			num_lsps_rcvd[bridgeId2nodeId[b[1]]]++;
			total_lsps_rcvd++;
    }
}

/SPBISIS: Higher Sequence Number/{
  if(capture==1)
    {
			split($2,a,"[");
			split(a[2],b,"]");
			num_lsps_rcvd_better[bridgeId2nodeId[b[1]]]++;
			total_lsps_rcvd_better++;

			time_last_better_lsp[bridgeId2nodeId[b[1]]]=$1;
			total_time_last_better_lsp=$1;  
		
			#update final counters at this point of potential last better lsp received 
			for( n = 0; n < num_nodes; n++ )
				{
					num_lsps_rcvd_final[n]=num_lsps_rcvd[n];
					total_lsps_rcvd_final=total_lsps_rcvd;      
					num_lsps_rcvd_better_final[n]=num_lsps_rcvd_better[n];
					total_lsps_rcvd_better_final=total_lsps_rcvd_better;      
				}
		}
}


# COMMON Special events
#######################
/SPBISIS: FRW State/{
  if(capture==1)
    {  
      split($2,a,"[");
  		split(a[2],b,"]");
  		time_last_transition_forwarding[bridgeId2nodeId[b[1]]]=$1;
  		total_time_last_transition_forwarding=$1;
		}
}


END{
  ###########################
  # select last event as CT #
  ###########################
  ct_net=0; mo_net=0; tr_net=0;
  if(coldStart==1)
    {
			for( n=0; n < num_nodes; n++ )
				{
				  #CT as last forwarding    
				  if(convTimeAsLastFrw==1) ct[n]=time_last_transition_forwarding[n];
				  #CT as last LSP        
				  if(convTimeAsLastLsp==1) ct[n]=time_last_better_lsp[n];
				  
				  # total network values
				  if(ct_net<ct[n]) ct_net=ct[n];
				  mo_net=mo_net+num_lsps_rcvd_final[n];
				  tr_net=tr_net+num_lsps_rcvd_better_final[n];
				}
    }
  if(linkFailure==1 || nodeFailure==1)
		{
			for( n=0; n < num_nodes; n++ )
				{
				  #CT as last forwarding    
				  if(convTimeAsLastFrw==1) ct[n]=time_last_transition_forwarding[n]-time_link_failure;
				  #CT as last LSP        
				  if(convTimeAsLastLsp==1) ct[n]=time_last_better_lsp[n]-time_link_failure;
				  
				  if(ct[n]<0) ct[n]=0;

				  # total network values
				  if(ct_net<ct[n]) ct_net=ct[n];
				  mo_net=mo_net+num_lsps_rcvd_final[n];
				  tr_net=tr_net+num_lsps_rcvd_better_final[n];
				}
    }  


  #################
  # print columns #
  #################  
  # printf("#xPos\tyPos\tnodeID\tCT\tMO\tTR\n");

  xPos=0;
  yPos=0;

  # print info per node (N lines)
  if(0)
    {
		  for( n=0; n < num_nodes; n++ )  
				{
					# print first columns
					printf("%2d\t",xPos);
					printf("%2d\t",yPos);
					printf("%2d\t",n);
	
				  # print conv time
				  printf("%0.4f\t",ct[n]/100000);
				    
				  # print messages
				  printf("%2d\t",num_lsps_rcvd_final[n]);

				  # print triggers
				  printf("%2d\t",num_lsps_rcvd_better_final[n]);

				  printf("\n");
				  
				  yPos=yPos+1; if(yPos==8){xPos=xPos+1; yPos=0; printf("\n");}
				}
		}

  # print info per net (1 line)
  if(1)
    {
			# print first columns
			printf("%2d\t",xPos);
			printf("%2d\t",yPos);
			printf("%2d\t",n);

			# print conv time
			printf("%0.4f\t",ct_net/100000);
			  
			# print messages
			printf("%2d\t",mo_net);

			# print triggers
			printf("%2d\t",tr_net);

			printf("\n");
			
			yPos=yPos+1; if(yPos==8){xPos=xPos+1; yPos=0; printf("\n");}      
    }  

}

