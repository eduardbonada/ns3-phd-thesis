

clear all;

##################
Nstart=4;                            # Nstart: starting number of nodes in the network
Nend=100;                              # Nend: ending number of nodes in the network

for N=Nstart:Nend

  costMatrix=ones(N,N)*999999;

  #################

  costMatrix(0+1,1+1)=1;
  costMatrix(1+1,2+1)=1;

  for r=2:N-2
    costMatrix(r+1,r+1+1)=1;
  endfor

  costMatrix(1+1,N-1+1)=1;


  #######################################################
  for i=1:N
    for j=1:N
      if(i==j)
        costMatrix(i,j)=0;
      endif
      if(costMatrix(i,j)==1)
        costMatrix(j,i)=1;
      endif
      #if(costMatrix(i,j)==999999)
      #endif
    endfor
  endfor

  save (sprintf ('/home/eduard/Desktop/phyMx/ring_root/%dnodes_ring_root-aside_phyCostMx.txt', N), "costMatrix")


endfor

