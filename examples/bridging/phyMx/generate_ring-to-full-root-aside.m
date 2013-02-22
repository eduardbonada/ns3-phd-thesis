

clear all;

##################
N=10;                            # N: number of nodes in the network

costMatrix=ones(N,N)*999999;



######################

costMatrix(0+1,1+1)=1;
costMatrix(1+1,2+1)=1;

for r=2:N-2
  costMatrix(r+1,r+1+1)=1;
endfor

costMatrix(1+1,N-1+1)=1;

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



#####################################

tempMatrix=zeros(N,N);
allConnectionsToAdd=[];
for n=2:N
  for m=2:N
    if(costMatrix(n,m)!=1 && n!=m && tempMatrix(n,m)==0)
      nextEntry=size(allConnectionsToAdd,1)+1;
      allConnectionsToAdd(nextEntry,1)=n;
      allConnectionsToAdd(nextEntry,2)=m;
      tempMatrix(n,m)=1;
      tempMatrix(m,n)=1;
    endif
  endfor
endfor

numAddedLinks=0
while(size(allConnectionsToAdd,1)>0)
  elem=floor(rand*(size(allConnectionsToAdd,1))-1)+2;
  
  costMatrix(allConnectionsToAdd(elem,1),allConnectionsToAdd(elem,2))=1;
  costMatrix(allConnectionsToAdd(elem,2),allConnectionsToAdd(elem,1))=1;
        
  allConnectionsToAdd(elem,:)=[];
  numAddedLinks=numAddedLinks+1;
  
  save (sprintf ('/home/eduard/Desktop/phyMx/%dnodes_ring-to-full[%d]_root-aside_phyCostMx.txt', N, numAddedLinks), "costMatrix")
endwhile

  
  

