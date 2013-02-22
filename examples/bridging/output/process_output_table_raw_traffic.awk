BEGIN{

  interval1=1 # interval 1: from cold start to first port going to forwarding
  interval2=2 # interval 2: from first port going to forwarding to last port going to forwarding
  interval3=3 # interval 3: from last port going to forwarding to failure
  
  interval4=4 # interval 4: from failure to first better BPDU
  interval5=5 # interval 5: from first better BPDU to last port going to forwarding
  interval6=6 # interval 6: from last port going to forwarding to end of simulation  
  interval7=7 # interval 7: from failure to first port going to forwarding
  
  num_samples=0
}

//{

  #$1 == valueCol => numNodes
  # ...
  if($1 == valueCol)
    {     
      num_samples = num_samples + 1
            
      num_nodes = $1
      
      total_gen_packets[interval1] = total_gen_packets[interval1]*( (num_samples-1)/num_samples ) + $2/num_samples;
      total_rcv_packets[interval1] = total_rcv_packets[interval1]*( (num_samples-1)/num_samples ) + $3/num_samples;
      interval1_length             = interval1_length*( (num_samples-1)/num_samples ) + $4/num_samples;

      total_gen_packets[interval2] = total_gen_packets[interval2]*( (num_samples-1)/num_samples ) + $5/num_samples;
      total_rcv_packets[interval2] = total_rcv_packets[interval2]*( (num_samples-1)/num_samples ) + $6/num_samples;
      interval2_length             = interval2_length*( (num_samples-1)/num_samples ) + $7/num_samples;

      total_gen_packets[interval3] = total_gen_packets[interval3]*( (num_samples-1)/num_samples ) + $8/num_samples;
      total_rcv_packets[interval3] = total_rcv_packets[interval3]*( (num_samples-1)/num_samples ) + $9/num_samples;
      interval3_length             = interval3_length*( (num_samples-1)/num_samples ) + $10/num_samples;

      total_gen_packets[interval4] = total_gen_packets[interval4]*( (num_samples-1)/num_samples ) + $11/num_samples;
      total_rcv_packets[interval4] = total_rcv_packets[interval4]*( (num_samples-1)/num_samples ) + $12/num_samples;
      interval4_length             = interval4_length*( (num_samples-1)/num_samples ) + $13/num_samples;

      total_gen_packets[interval5] = total_gen_packets[interval5]*( (num_samples-1)/num_samples ) + $14/num_samples;
      total_rcv_packets[interval5] = total_rcv_packets[interval5]*( (num_samples-1)/num_samples ) + $15/num_samples;
      interval5_length             = interval5_length*( (num_samples-1)/num_samples ) + $16/num_samples;

      total_gen_packets[interval6] = total_gen_packets[interval6]*( (num_samples-1)/num_samples ) + $17/num_samples;
      total_rcv_packets[interval6] = total_rcv_packets[interval6]*( (num_samples-1)/num_samples ) + $18/num_samples;
      interval6_length             = interval6_length*( (num_samples-1)/num_samples ) + $19/num_samples;

      total_gen_packets[interval7] = total_gen_packets[interval7]*( (num_samples-1)/num_samples ) + $20/num_samples;
      total_rcv_packets[interval7] = total_rcv_packets[interval7]*( (num_samples-1)/num_samples ) + $21/num_samples;
      interval7_length             = interval7_length*( (num_samples-1)/num_samples ) + $22/num_samples;
      
      ratio_43 = ratio_43*( (num_samples-1)/num_samples ) + $23/num_samples;
      ratio_53 = ratio_53*( (num_samples-1)/num_samples ) + $24/num_samples;

    }    
}

END{

  #printf("Nod  \tNG1  \tNR1  \tL1   \tNG2  \tNR2  \tL2   \tNG3  \tNR3  \tL3   \tNG4  \tNR4  \tL4   \tNG5  \tNR5  \tL5   \t  NG6\t  NR6\tL6   \t  NG7\t  NR7\tL7   \tR4/R3\tR5/R3\n");
  printf ("%0.0f\t%0.0f\t%0.0f\t%1.2f\t%0.0f\t%0.0f\t%1.2f\t%0.0f\t%0.0f\t%1.2f\t%0.0f\t%0.0f\t%1.2f\t%0.0f\t%0.0f\t%1.2f\t%0.0f\t%0.0f\t%1.2f\t%0.0f\t%0.0f\t%1.2f\t%1.2f\t%1.2f\n",\
  num_nodes,\
  total_gen_packets[interval1],\
  total_rcv_packets[interval1],\
  interval1_length,\
  total_gen_packets[interval2],\
  total_rcv_packets[interval2],\
  interval2_length,\
  total_gen_packets[interval3],\
  total_rcv_packets[interval3],\
  interval3_length,\
  total_gen_packets[interval4],\
  total_rcv_packets[interval4],\
  interval4_length,\
  total_gen_packets[interval5],\
  total_rcv_packets[interval5],\
  interval5_length,\
  total_gen_packets[interval6],\
  total_rcv_packets[interval6],\
  interval6_length,\
  total_gen_packets[interval7],\
  total_rcv_packets[interval7],\
  interval7_length,\
  ratio_43,\
  ratio_53);

}




