/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Eduard Bonada<eduard.bonada@upf.edu>
 */

#include "stp.h"
#include "rstp.h"
#include "spb-isis.h"
#include "simulation-god.h"
#include "bridge-net-device.h"
#include "bpdu-header.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include <stdlib.h>
#include <cmath>
#include <string>
#include <sstream>

NS_LOG_COMPONENT_DEFINE ("SimulationGod");

namespace ns3 {

TypeId
SimulationGod::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationGod")
    .SetParent<Object> ()
    .AddConstructor<SimulationGod> ()
    ;
  return tid;
}

SimulationGod::SimulationGod()
{
  m_numNodes=0;
  m_cumulatedLinkFailures=0;
}
  
SimulationGod::~SimulationGod()
{
}

void
SimulationGod::SetStpMode(uint16_t mode)
{
  m_stpMode=mode;
}

void
SimulationGod::SetNumNodes(uint64_t numNodes)
{
  m_numNodes=numNodes;
 
  // init matrices
  std::vector < uint16_t > rowTmpPorts;
  std::vector < int > rowTmpTree;
  std::vector < uint32_t > rowTmpSpb;
  std::vector < std::vector < uint32_t > > mxTmpSpb;
  
  m_lastTopologyWithLoop.resize(numNodes);
  m_treeIsSpanning.resize(numNodes);
  
  m_nodeId2bridgeId.resize(numNodes);
  m_bridgeId2nodeId.resize(numNodes);
  
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      rowTmpPorts.clear();
      rowTmpTree.clear();
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          rowTmpPorts.push_back(999);
          rowTmpTree.push_back(0);
        }
      m_portMx.push_back(rowTmpPorts);
      m_failedMx.push_back(rowTmpTree);      // not an error, initialize with 0s
      m_failedNodes.push_back(false);
    }

  // initialize matrix to store SPB LS database and array of tree matrices
  for (uint64_t j=0 ; j < m_numNodes ; j++ )
    {
      rowTmpSpb.clear();
      for (uint64_t k=0 ; k < m_numNodes ; k++ )
        {
          rowTmpSpb.push_back(0);
        }
      mxTmpSpb.push_back(rowTmpSpb);
    }
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      m_spbIsisTopVertAdjMx.push_back(mxTmpSpb); // use the already created empty matrix to create the array of matrices
      m_vertexAdjTree.push_back(m_failedMx); // use the already created empty matrix to create the array of matrices
      m_lastTopologyWithLoop.push_back(false);
      m_treeIsSpanning.push_back(false);
    } 
}

void
SimulationGod::SetNodeIdBridgeIdMap(uint64_t n, uint64_t b)
{
  m_nodeId2bridgeId.at(n) = b;
  m_bridgeId2nodeId.at(b) = n;  
}

void
SimulationGod::SetPortConnection(uint64_t nodeA, uint64_t nodeB, uint16_t port)
{
   m_portMx.at(nodeA).at(nodeB) = port;
}

void
SimulationGod::SetActivePort(uint64_t tree, uint64_t node, uint16_t port, bool forwarding)
{
  NS_LOG_LOGIC("Setting Node " << node << " - Port " << port << " to state " << forwarding);
  
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      //NS_LOG_LOGIC("Node " << node << " - node " << i << " => port " <<  m_portMx.at(node).at(i));
      if( m_portMx.at(node).at(i)==port )
        {
          NS_LOG_LOGIC("Found node " << i << " connected to node " << node << " - port " << port);
          if(forwarding && m_failedMx.at(node).at(i)==0 ) //if set to forwarding and not failed
            {
              m_vertexAdjTree.at(tree).at(node).at(i)=1;
            }
          else // if set to discarding or failed
            {
              m_vertexAdjTree.at(tree).at(node).at(i)=0;
            }
          break;
        }
    }

  if(forwarding)
    NS_LOG_LOGIC("Node " << node << " - port " << port << " just went forwarding");
  else
    NS_LOG_LOGIC("Node " << node << " - port " << port << " just went discarding");
  
  //PrintTreeMx(tree);
  
  // check for loops in tree
  m_treeIsSpanning.at(tree)=true;
  //for (uint64_t i=0 ; i < m_numNodes ; i++ )
    //{
      NS_LOG_LOGIC("Checking Loops for tree " << tree);
      int16_t loopTree=CheckLoopsTreeMx(tree); // '1' if loop, '0' id spanning, '-1' if not spanning
      if(loopTree==1)
        {
          NS_LOG_INFO(Simulator::Now() << " SimGod => WARNING: Loop found in tree of bridge " << tree << "!");
          m_lastTopologyWithLoop.at(tree)=true;
        }
      else
        {
          if(m_lastTopologyWithLoop.at(tree)==true)
            {
              NS_LOG_INFO(Simulator::Now() << " SimGod => WARNING: No Loops Any More in tree of bridge " << tree << "!");
              m_lastTopologyWithLoop.at(tree)=false;
            }
        }
      if(loopTree==-1 || loopTree==1) //if it is not spanning, or there is a loop
        {
          m_treeIsSpanning.at(tree)=false;
        }
    //}

  // print wether 'all' trees are spanning or not
  bool allSpanningTree=true;
  if(m_stpMode==1 || m_stpMode==2) // single tree (STP-RSTP)
    {
      if(m_treeIsSpanning.at(0)!=true && m_failedNodes.at(m_bridgeId2nodeId.at(0))==false) allSpanningTree=false;
    }
  if(m_stpMode==3 || m_stpMode==4) // one tree per node (SPBISIS - SPBDV)  
    {
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          NS_LOG_LOGIC("Failed bridge " << i << "? " << m_failedNodes.at(m_bridgeId2nodeId.at(i)));
          NS_LOG_LOGIC("Spanning tree " << i << "? " << m_treeIsSpanning.at(i));          
          if(m_treeIsSpanning.at(i)!=true && m_failedNodes.at(m_bridgeId2nodeId.at(i))==false) allSpanningTree=false;
        }
    }
    
  if(m_cumulatedLinkFailures==0)
    {
      if(allSpanningTree==true)
        {
          NS_LOG_INFO(Simulator::Now() << " SimGod => It is a spanning tree (initial convergence).");
          if(m_stpMode==3 || m_stpMode==4) // one tree per node (SPBISIS - SPBDV) 
            {
              if(CheckSymmetry()==true) NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are symmetric (initial convergence).");
              else                      NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are not symmetric (initial convergence).");
            }  
        } 
      else
        NS_LOG_INFO(Simulator::Now() << " SimGod => Not a spanning tree yet (initial convergence).");
    }
  else
    {
      if(allSpanningTree==true)
        {
          NS_LOG_INFO(Simulator::Now() << " SimGod => It is a spanning tree (failure convergence).");
          if(m_stpMode==3 || m_stpMode==4) // one tree per node (SPBISIS - SPBDV) 
            {
              if(CheckSymmetry()==true) NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are symmetric (failure convergence).");
              else                      NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are not symmetric (failure convergence).");
            }
        }
      else
        NS_LOG_INFO(Simulator::Now() << " SimGod => Not a spanning tree yet (failure convergence).");    
    }
}
      
void
SimulationGod::SetFailedLink(uint64_t nodeA,uint64_t nodeB)
{
  m_cumulatedLinkFailures++;
  
  // update failed matrix (DB)
  m_failedMx.at(nodeA).at(nodeB) = 1;
  m_failedMx.at(nodeB).at(nodeA) = 1;
  
  //check if this link failure isolates a node (node failure)
  bool nodeAFailed=false;
  NS_LOG_LOGIC("Checking if node " << nodeA << " has failed");
  for(uint64_t n=0 ; n < m_numNodes ; n++)
    {
      if(nodeA!=n)
        {
          NS_LOG_LOGIC("  Checking connection with node " << n );
          
          //check nodeA
          NS_LOG_LOGIC("    nodeA: " << nodeA << " => connected-" << m_vertexAdjPhyMx.at(nodeA).at(n) << " | failedMx-" << m_failedMx.at(nodeA).at(n));    
          if( m_vertexAdjPhyMx.at(nodeA).at(n)==1)
            {
              if(m_failedMx.at(nodeA).at(n) == 1 )
                {
                  // this connection has failed
                  nodeAFailed = true;
                }
              else
                {
                  //this connection has not failed
                  nodeAFailed = false;
                  break;
                }
            }
        }
    }
    
  NS_LOG_LOGIC("Checking if node " << nodeB << " has failed");
  bool nodeBFailed=false;
  for(uint64_t n=0 ; n < m_numNodes ; n++)
    {      
      if(nodeB!=n)
        {
          NS_LOG_LOGIC("  Checking connection with node " << n );
          
          //check nodeB
          NS_LOG_LOGIC("    nodeB: " << nodeB << " => connected-" << m_vertexAdjPhyMx.at(nodeB).at(n) << " | failedMx-" << m_failedMx.at(nodeB).at(n));    
          if( m_vertexAdjPhyMx.at(nodeB).at(n)==1)
            {
              if(m_failedMx.at(nodeB).at(n) == 1 )
                {
                  // this connection has failed
                  nodeBFailed = true;
                }
              else
                {
                  //this connection has not failed
                  nodeBFailed = false;
                  break;
                }
            }
        }
    }
  if(nodeAFailed==true)
    {
      m_failedNodes.at(nodeA)=true;
      NS_LOG_LOGIC("Node" << nodeA << " has failed");
    }
  if(nodeBFailed==true)
    {
      m_failedNodes.at(nodeB)=true;
      NS_LOG_LOGIC("Node" << nodeB << " has failed");
    }
     
  // set to 'blocking' in tree mx
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      m_vertexAdjTree.at(i).at(nodeA).at(nodeB)=0;
      m_vertexAdjTree.at(i).at(nodeB).at(nodeA)=0;
      //PrintTreeMx(i);
    }
    
  // check for loops in each tree
  bool allSpanningTree=true;
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      if(m_failedNodes.at(i)==false) //avoid checking a failed node
        {
          NS_LOG_LOGIC("Checking Loops for tree " << i);
          int16_t loopTree=CheckLoopsTreeMx(i);
          if(loopTree==1)
            {
              NS_LOG_INFO(Simulator::Now() << " SimGod => WARNING: Loop found in tree of bridgeId " << i << "!");
              m_lastTopologyWithLoop.at(i)=true;
            }
          else
            {
              if(m_lastTopologyWithLoop.at(i)==true)
                {
                  NS_LOG_INFO(Simulator::Now() << " SimGod => WARNING: No Loops Any More in tree of bridgeId " << i << "!");
                  m_lastTopologyWithLoop.at(i)=false;
                }
            }
          if(loopTree==-1 || loopTree==1) //if it is not spanning, or there is a loop
            {
              allSpanningTree=false;
            }    
        }
    }

  // print whether 'all' trees are spanning or not
  if(m_cumulatedLinkFailures==0)
    {
      if(allSpanningTree==true)
        {
          NS_LOG_INFO(Simulator::Now() << " SimGod => It is a spanning tree (initial convergence).");
          if(m_stpMode==3 || m_stpMode==4) // one tree per node (SPBISIS - SPBDV) 
            {
              if(CheckSymmetry()==true) NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are symmetric (initial convergence).");
              else                      NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are not symmetric (initial convergence).");
            }  
        } 
      else
        NS_LOG_INFO(Simulator::Now() << " SimGod => Not a spanning tree yet (initial convergence).");
    }
  else
    {
      if(allSpanningTree==true)
        {
          NS_LOG_INFO(Simulator::Now() << " SimGod => It is a spanning tree (failure convergence).");
          if(m_stpMode==3 || m_stpMode==4) // one tree per node (SPBISIS - SPBDV) 
            {
              if(CheckSymmetry()==true) NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are symmetric (failure convergence).");
              else                      NS_LOG_INFO(Simulator::Now() << " SimGod => Trees are not symmetric (failure convergence).");
            }
        }
      else
        NS_LOG_INFO(Simulator::Now() << " SimGod => Not a spanning tree yet (failure convergence).");    
    }
}

int16_t
SimulationGod::CheckLoopsTreeMx(uint64_t tree)
{
  // returns '1' if loop exist, '0' if it is a spannign tree, and '-1' if no loops but not a spanning tree yet

  /*
    - it is similar to check wether a broadcast message, one started at each node, is received in a node that already forwarded it
    - this previous idea is taken from the graph perspecttive, and the calculations are done at matrix level.
    - basically, the algorithm starts at one node, goes down to all possible branches from that node, and marks nodes as visited, whena visited node is 
      visited again, loop found!
    - this is repeated fro each node, and for each bifurcation found.
    - if no nodes are re-visited, no loops detected.
  */
  
  NS_LOG_FUNCTION_NOARGS ();
    
  bool connected[m_numNodes];
  bool visited[m_numNodes];
  std::vector < std::vector < int > > treeTmpMx;
  std::vector < uint64_t > pendingToVisit;
  std::vector < uint64_t > lastBifurcation;
  uint64_t prev_n;
  uint64_t countOnes=0;
  bool isSpanningTree=false; //if all nodes are visited in a single for-loop, then all nodes are connected. so if no loop, amd connected, it is a spanning tree
  
  treeTmpMx = m_vertexAdjPhyMx;
  
  for(uint64_t i=0 ; i < m_numNodes ; i++)
    {
      visited[i]=false;
      
      //check whether nodes are connected to someone (to avoid accounting for a not connected node, specially when root fails)
      connected[i]=false;
      for(uint64_t j=0 ; j < m_numNodes ; j++) if(m_vertexAdjTree.at(tree).at(i).at(j)==1) countOnes++;
      if(countOnes!=0) connected[i]=true;
    }
    
  //for each node, a starting wave is started
  bool treeParsed=false;
  for(uint64_t node=0 ; node < m_numNodes ; node++)
    {
      NS_LOG_LOGIC("node " << node << " connected(" << connected[node] << ") - treeParsed(" << treeParsed << ")");
      if(connected[node]==true && treeParsed==false)
        {
          treeParsed=true;
          
          isSpanningTree=false;
          
          uint64_t n=node;
          if(visited[n]==false)
            {
              NS_LOG_LOGIC("Start For: n: " << n);
              // initialize aux variables, the temporary matrix used for computation
              for(uint64_t i=0 ; i < m_numNodes ; i++)
                {
                  //visited[i]=false;
                  for(uint64_t j=0 ; j < m_numNodes ; j++)
                    {
                      // only links active at both sides are really active
                      if( m_vertexAdjTree.at(tree).at(i).at(j) == 1 && m_vertexAdjTree.at(tree).at(j).at(i)==1 )
                        {
                          treeTmpMx.at(i).at(j) = 1;
                          treeTmpMx.at(j).at(i) = 1;
                        }
                      else
                        {
                          treeTmpMx.at(i).at(j) = 0;
                          treeTmpMx.at(j).at(i) = 0;
                        } 
                    }
                }
              
              NS_LOG_LOGIC("Full link matrix:");
              PrintMx(treeTmpMx);
              
              // initial state
              pendingToVisit.push_back(n); //start branching at node n
              NS_LOG_LOGIC("Size of pendingToVisit: " << pendingToVisit.size());
              
              // loop
              bool allConnectedVisited=false;
              while(!allConnectedVisited)
                {
                  NS_LOG_LOGIC("Start iteration");
                  // take a node from pending to visit (we have arbitrarily decided the last one added) and mark it as visited
                  prev_n = n;
                  n = pendingToVisit.back();
                  pendingToVisit.pop_back();
                  visited[n] = true;
                  NS_LOG_LOGIC("n: " << n << " - prev_n: " << prev_n);
                  
                  // mark the connection as done in the matrix (virtually remove the connection so it can not be used again and provide loops back and forth through the same link)
                  treeTmpMx.at(n).at(prev_n) = 0;
                  treeTmpMx.at(prev_n).at(n) = 0;
                  NS_LOG_LOGIC("Connections [n<->prev_n] set to 0");
                  
                  PrintMx(treeTmpMx);
                  
                  // if node n is not connected to more nodes (there are no 1s in the row of TmpTreeMx), end of branch found
                  countOnes=0;
                  for(uint64_t i=0 ; i < treeTmpMx.at(n).size() ; i++ ) if( treeTmpMx.at(n).at(i) == 1 ) countOnes++;
                  NS_LOG_LOGIC("countOnes: " << countOnes);
                  if(countOnes==0)
                    {
                      // end of branch found, select next node to visit
                      if(lastBifurcation.size()!=0)
                        {
                          n = lastBifurcation.back();
                          lastBifurcation.pop_back();
                          NS_LOG_LOGIC("n set to last bifurcation: " << n);
                        }
                      else
                        {
                          NS_LOG_LOGIC("not more bifurcations, out of while");
                          allConnectedVisited=true;
                        }
                    }
                  else // check nodes to be visited and detect loop (error and exit) if these nodes are already visited
                    { 
                      // add 1s in TmpTreeMx to pendingToVisit and check for already visited                   
                      for(uint64_t i=0 ; i < treeTmpMx.at(n).size() ; i++ )
                        {
                          if( treeTmpMx.at(n).at(i) == 1 )
                            {
                              if(visited[i]==true)
                                {
                                  NS_LOG_LOGIC("ERROR: Loop found!!!");
                                  return 1;
                                }
                              pendingToVisit.push_back(i);
                            }
                        }
                        
                      // add to lastBifurcation nodes (add 'number of ones'-1, one for each additional path )
                      for(uint64_t i=1 ; i < countOnes ; i++ )
                        lastBifurcation.push_back(n);     
                    }
                }
            }
        }
    }         
             
  // printout traces with the results
  bool allVisited=true;
  for(uint64_t i=0 ; i < m_numNodes ; i++)
    {
      if(m_failedNodes.at(i)==false) //avoid checking a failed node
        {
          if(connected[i]==true)
            {
              if(visited[i]==true)
                allVisited=allVisited&true;
              else
                allVisited=false;
            }
          else
            {
              allVisited=false;
            }
        }
    }
  if(allVisited)
    {
      NS_LOG_LOGIC("CheckLoopsTree returns 0");
      return 0; // it is a spanning tree
    }
  else
    {
      NS_LOG_LOGIC("CheckLoopsTree returns -1");
      return -1; // no loops, but not a spanning tree yet
    }
}

bool
SimulationGod::CheckSymmetry()
{
  // checks if all trees are symmetric
  // returns true if symmetry, false otherwise
  
  // declare and init temporary variables
  /////////////////////////////////////////
  std::vector < uint64_t > rowTmpTree, rowTmpTree2;
  std::vector < std::vector < uint64_t > > mxTmpTree, mxTmpTree2;  
  std::vector < std::vector < std::vector < uint64_t > > > treeCostMx;      // cost matrices
  std::vector < std::vector < std::vector < uint64_t > > > treePredMx;   // next_hop matrices
  rowTmpTree.clear();
  for (uint64_t i=0 ; i < m_numNodes ; i++ ){ rowTmpTree.push_back(0); rowTmpTree2.push_back(999999); }
  for (uint64_t i=0 ; i < m_numNodes ; i++ ){ mxTmpTree.push_back(rowTmpTree); mxTmpTree2.push_back(rowTmpTree2); }
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
     treeCostMx.push_back(mxTmpTree);
     treePredMx.push_back(mxTmpTree2);
    }
          
  // process each tree
  //////////////////////
  for(uint64_t t=0 ; t < m_numNodes ; t++)
    {
      // convert the tree vertex adjacency matrices to cost matrices
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          for (uint64_t j=0 ; j < m_numNodes ; j++ )
            {
              if(m_vertexAdjTree.at(t).at(i).at(j)==1 && m_vertexAdjTree.at(t).at(j).at(i)==1)  // full active links
                {
                  treeCostMx.at(t).at(i).at(j) = 1;
                  treePredMx.at(t).at(i).at(j) = i;
                }
              if(m_vertexAdjTree.at(t).at(i).at(j)==0 || m_vertexAdjTree.at(t).at(j).at(i)==0) // not full active links
                treeCostMx.at(t).at(i).at(j) = 999999;  // put 999999 to 0s                  
              if(i==j)
                treeCostMx.at(t).at(i).at(j) = 0;// put 0s in diagonal          
            }
        }   
     
      // apply FloydWarshall to the cost matrix to get the predecessors matrix
      for (uint64_t k=0 ; k < m_numNodes ; k++ )
        {        
          for (uint64_t i=0 ; i < m_numNodes ; i++ )
            {
              for (uint64_t j=0 ; j < m_numNodes ; j++ )
                {
                  if(treeCostMx.at(t).at(i).at(k) + treeCostMx.at(t).at(k).at(j) < treeCostMx.at(t).at(i).at(j))
                    {
                      treeCostMx.at(t).at(i).at(j) = treeCostMx.at(t).at(i).at(k) + treeCostMx.at(t).at(k).at(j);
                      treePredMx.at(t).at(i).at(j) =  treePredMx.at(t).at(k).at(j);
                    }
                }
            }
        }

      // print the treeCostMx matrices for debbuging purposes
      NS_LOG_LOGIC("SimGod => TreeCostMx " << t);
      std::stringstream stringOut1;
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          for (uint64_t j=0 ; j < m_numNodes ; j++ )
            {
              stringOut1 << treeCostMx.at(t).at(i).at(j) << "\t";
            }
          stringOut1 << "\n";
        }
      NS_LOG_LOGIC(stringOut1.str());  

      // print the treePredMx matrices for debbuging purposes
      NS_LOG_LOGIC("SimGod => treePredMx " << t);
      std::stringstream stringOut2;
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          for (uint64_t j=0 ; j < m_numNodes ; j++ )
            {
              stringOut2 << treePredMx.at(t).at(i).at(j) << "\t";
            }
          stringOut2 << "\n";
        }
      NS_LOG_LOGIC(stringOut2.str());  
            
    } // end of for of trees
  
  // check node-to-node symmetry
  ////////////////////////////////
  bool originReached=false;
  bool symmetricTrees=true;
  uint64_t currentPred[2];
  for(uint64_t i=0 ; i < m_numNodes ; i++)
    {
      for(uint64_t j=0 ; j < m_numNodes ; j++)
        {
          originReached=false;
          if(i!=j && treePredMx.at(m_nodeId2bridgeId.at(i)).at(i).at(j)!=999999 && treePredMx.at(m_nodeId2bridgeId.at(i)).at(i).at(j)!=999999)
            {
              NS_LOG_LOGIC("i:" << i << " j:" << j);

              // start at destination (j), and take first predecessor of source tree and destination tree
              currentPred[0] = treePredMx.at(m_nodeId2bridgeId.at(i)).at(i).at(j);
              currentPred[1] = treePredMx.at(m_nodeId2bridgeId.at(j)).at(i).at(j);
              NS_LOG_LOGIC("currentPred[0]: " << currentPred[0] << " currentPred[1]: " << currentPred[1]);
     
              while(originReached==false && symmetricTrees==true)
                {
                  // if current predecessors are the same, continue to next predecessors, until source (i) is reached
                  if(currentPred[0] == currentPred[1])
                    {
                      if(currentPred[0] == i) // source reached
                        { 
                          NS_LOG_LOGIC("originReached");
                          originReached=true;
                        }
                      else
                        {
                          currentPred[0] = treePredMx.at(m_nodeId2bridgeId.at(i)).at(i).at(currentPred[0]);
                          currentPred[1] = treePredMx.at(m_nodeId2bridgeId.at(j)).at(i).at(currentPred[1]);
                          NS_LOG_LOGIC("currentPred[0]: " << currentPred[0] << " currentPred[1]: " << currentPred[1]);
                        }
                    }
                  // if current predecessors are not the same, there is no symmetry
                  else
                    {
                      symmetricTrees=false;
                    }
                } 
            }
        }
    }    
  return symmetricTrees;
}

void
SimulationGod::SetVertexAdjPhyMx(std::vector < std::vector < int > > *mx)
{
  m_vertexAdjPhyMx = *mx;
}

void
SimulationGod::SetSpbIsisTopVertAdjMx(uint64_t node, std::vector < std::vector < uint32_t > > *mx)
{
  
  // copy the value of the matrix
  for(uint64_t i=0 ; i < m_numNodes ; i++)
    {
      for(uint64_t j=0 ; j < m_numNodes ; j++)
        {
          m_spbIsisTopVertAdjMx.at(node).at(i).at(j) = (*mx).at(i).at(j);
        }
    }

  // print the matrices for debbuging purposes
  for (uint64_t n=0 ; n < m_numNodes ; n++ )
    {
      NS_LOG_LOGIC("LS-DB node " << n);
      std::stringstream stringOut;
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          for (uint64_t j=0 ; j < m_numNodes ; j++ )
            {
              stringOut << m_spbIsisTopVertAdjMx.at(n).at(i).at(j) << "\t";
            }
          stringOut << "\n";
        }
      NS_LOG_LOGIC(stringOut.str());
    }
  
  // Check if all matrices are equal
  if ( CheckSpbIsIsAllSameVertAdjMx() == true )
    {
      NS_LOG_INFO(Simulator::Now() << " SimGod => All nodes have the same LSP-DB!");
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          m_spbisiss.at(i)->SetPortsToForwarding(true);
        }
    }
  else
    {
      NS_LOG_INFO(Simulator::Now() << " SimGod => Some nodes have different LSP-DB!");
      for (uint64_t i=0 ; i < m_numNodes ; i++ )
        {
          m_spbisiss.at(i)->SetPortsToForwarding(false);
        }
    }
}


bool SimulationGod::CheckSpbIsIsSameVertAdjMx(uint64_t bridgeA, uint64_t bridgeB)
{
  uint64_t nodeA = m_bridgeId2nodeId.at(bridgeA);
  uint64_t nodeB = m_bridgeId2nodeId.at(bridgeB);
  
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          if( m_spbIsisTopVertAdjMx.at(nodeA).at(i).at(j) != m_spbIsisTopVertAdjMx.at(nodeB).at(i).at(j) )
            {
              return false;
            }
        }
    }
  return true; // if we get here, it means all elements are equal in all matrices (everyone is equal to 0, hence everyone is equal to everyone) 
}



bool SimulationGod::CheckSpbIsIsAllSameVertAdjMx()
{
  
  for (uint64_t n=0 ; n < m_numNodes ; n++ )
    {
      NS_LOG_LOGIC("CheckSpbIsIsAllSameVertAdjMx: Failed node " << n << ": " << m_failedNodes.at(n));
      if(m_failedNodes.at(n)==false) //avoid checking a failed node
        {
          for (uint64_t i=0 ; i < m_numNodes ; i++ )
            {
              for (uint64_t j=0 ; j < m_numNodes ; j++ )
                {
                  if( m_spbIsisTopVertAdjMx.at(0).at(i).at(j) != m_spbIsisTopVertAdjMx.at(n).at(i).at(j) )
                    {
                      return false;
                    }
                }
            }
        }
    }
    
  return true; // if we get here, it means all elements are equal in all matrices (everyone is equal to 0, hence everyone is equal to everyone)
}

void
SimulationGod::PrintSimInfo()
{
  NS_LOG_LOGIC("SimGod => NumNodes: " << m_numNodes);
} 

void
SimulationGod::PrintPhyMx()
{
  std::stringstream stringOut;

  NS_LOG_LOGIC("SimGod => PhyMx"); 
  PrintMx(m_vertexAdjPhyMx);
}

void
SimulationGod::PrintPortMx()
{
  //NS_LOG_LOGIC("SimGod => PortMx"); 
  //PrintMx(m_portMx);
}

void
SimulationGod::PrintTreeMx(uint64_t tree)
{
  NS_LOG_INFO("SimGod => TreeMx " << tree);

  // print the matrices for debbuging purposes
  std::stringstream stringOut;
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          stringOut << m_vertexAdjTree.at(tree).at(i).at(j) << "\t";
        }
      stringOut << "\n";
    }
  NS_LOG_INFO(stringOut.str());
}

void
SimulationGod::PrintMx(std::vector < std::vector < int > > mx)
{
  std::stringstream stringOut;

  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          stringOut << mx.at(i).at(j) << "\t";
        }
      stringOut << "\n";
    }
  NS_LOG_LOGIC(stringOut.str());
}

void 
SimulationGod::AddSTPPointer(Ptr<STP> p)
{
  m_stps.push_back(p);
}

void 
SimulationGod::AddRSTPPointer(Ptr<RSTP> p)
{
  m_rstps.push_back(p);
}

void 
SimulationGod::AddSPBISISPointer(Ptr<SPBISIS> p)
{
  m_spbisiss.push_back(p);
}

void
SimulationGod::PrintBridgesInfo()
{ 
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      if(m_stpMode==2) m_rstps.at(i)->PrintBridgeInfoSingleLine();
      if(m_stpMode==3) m_spbisiss.at(i)->PrintBridgeInfoSingleLine();
    }
}

void
SimulationGod::DoDispose (void)
{
}   

} //namespace ns3 
