package Graph;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author sriram
 */
import org.jgraph.graph.DefaultGraphModel;
import org.jgraph.graph.GraphLayoutCache;
import org.jgraph.JGraph;
import org.jgraph.graph.DefaultEdge;
import org.jgraph.graph.DefaultCellViewFactory;
import org.jgraph.graph.GraphConstants;
import org.jgraph.graph.DefaultGraphCell;

import java.awt.geom.Rectangle2D;
import java.awt.Color;
import java.awt.Font;
import javax.swing.ToolTipManager;
import javax.swing.BorderFactory;
import java.awt.event.MouseEvent;
import java.util.Collections;

import java.util.ArrayList;

import Data.*;

class processLabel extends DefaultGraphCell
{
    int procID;
    
    public processLabel(int pid)
    {
        super();
        procID = pid;
    }
    
    public void setText()
    {
        GraphConstants.setValue(this.getAttributes(), "Process"+procID);
        GraphConstants.setFont(this.getAttributes(), new Font("Tahoma", Font.BOLD, 14));
    }
}

public class ISPGraphModel extends JGraph {
    // datatypes
    public DefaultGraphModel Model;
    public GraphLayoutCache View;
    public ArrayList<Transition> Nodes;
    public ArrayList<DefaultEdge> Edges;
    public int CellWidth;
    public int CellHeight;
    // distance between cells
    public int xGap;
    public int yGap;
    
    // reference co-ordinate based on which location will be computed
    public int xBase;
    public int yBase;

    public ISPGraphModel() {
        Model = new DefaultGraphModel();
        View = new GraphLayoutCache(Model, new DefaultCellViewFactory());

        this.setModel(Model);
        this.setGraphLayoutCache(View);

        CellWidth = 120;
        CellHeight = 20;
        
        // alter xGap, yGap for cell positioning
        xGap = 200;
        yGap = 50;
        
        // origin
        xBase = -80;
        yBase = 35;

        // initializing nodes and edges
        Nodes = new ArrayList<Transition>();
        Edges = new ArrayList<DefaultEdge>();

        ToolTipManager.sharedInstance().registerComponent(this);
        this.setAutoResizeGraph(true);
        this.setAutoscrolls(true);        
    }

    public void copyNodes(ArrayList<Transition> copylist) {
        //Nodes.clear();
        try {
            Nodes.addAll(copylist);
        } catch (Exception e) {
            System.out.println("Exception:trying to add to node list");
        }
    }
    
    public void relocateCellsforIssueOrder() {
        
        int nP = GlobalStructure.getInstance().noProcess;
        // add process labels
        for(int i = 0; i < nP; i++)
            addProcessLabels(i, i);     // counter, pid
        
        int currProc = -1;
        int counter[] = new int[nP];
        int xLoc, yLoc, yRef;
        yRef = yGap + yBase;  
        
        for(int i = 0; i < Nodes.size(); i++) {
            currProc = Nodes.get(i).pID;
            counter[currProc]++;
            
            xLoc = ((currProc + 1) * xGap) + xBase;
            yLoc = counter[currProc] * yGap + yRef;            
            
            GraphConstants.setBounds(Nodes.get(i).getAttributes(),
                    new Rectangle2D.Double(xLoc, yLoc, CellWidth, CellHeight));            
        }
        
    }
    
    // relocates cells based on issue order
    
    public void relocateCellsforTimeOrder() {        
        int nP = GlobalStructure.getInstance().noProcess;
        int timeStep = 0;
        int collectiveTimeStep = 0, collectiveCount = 0;
        int finalizeTimeStep = 0;
        boolean collective = false, finalize = false;
    
        // add process labels
        for(int i = 0; i < nP; i++)
            addProcessLabels(i, i);     // counter, pid
                   
        int currProc;               
        int yRef;        
        int cProc;
        String collectiveName = new String();
        yRef = yGap + yBase;  
        Transition transition, tDstRef = null;
        
        int currInterleaving = GlobalStructure.getInstance().currInterleaving;
        Interleavings interleaving = GlobalStructure.getInstance().interleavings.get(currInterleaving-1);                       
        
        for(int i = 0; i < Nodes.size(); i++)
        {            
            transition = Nodes.get(i);
            currProc = transition.pID + 1;            
/*            transition.xLoc = (currProc * xGap) + xBase;                        
//            transition.yLoc = (timeStep+1) * yGap + yRef;  
            
            if(transition.isCollective()) {                            
                if( (!transition.function.equals(collectiveName)) || collectiveCount > interleaving.nP ) {
                    collective = false;
                    collectiveName = transition.function;
                    collectiveCount = 1;
                }
                
                if(!collective) {
                    timeStep++;                
                    collectiveName = transition.function;
                    collectiveTimeStep = timeStep;                    
                    cProc = transition.pID;
                }                    
                else
                    collectiveCount++;
                
                transition.yLoc = collectiveTimeStep * yGap + yRef;
                transition.timeOrdered = true;
                collective = true;
            }
            else if (transition.isPointToPointSend()) {
                 timeStep++;        
                 transition.yLoc = timeStep * yGap + yRef;                          
                 transition.timeOrdered = true;                 
                 tDstRef = interleaving.getTransition(transition.match_pID, transition.match_index);            
                 if(tDstRef != null) {
                       tDstRef.yLoc = transition.yLoc;               
                       tDstRef.timeOrdered = true;
                 }
            }
            else if(transition.isPointToPointReceive()) {
                if( ! transition.timeOrdered) {
                    if(transition.match_pID == -1) {
                        timeStep++;
                        transition.yLoc = timeStep * yGap + yRef;                          
                        transition.timeOrdered = true;                 
                    }                        
                }
             }  
            else if (transition.isWait()) {
                       timeStep++;
                       transition.yLoc = timeStep * yGap + yRef;                          
                       transition.timeOrdered = true;                 
            }
            
            else if(transition.isFinalize()) {                                
                if(!finalize) {
                    timeStep++;                
                    finalizeTimeStep = timeStep;                 
                }
                   
                transition.yLoc = finalizeTimeStep * yGap + yRef;
                transition.timeOrdered = true;
                finalize = true;
            }                        
            else{
                timeStep++;
                finalize = false;
                collective = false;
                if( ! transition.timeOrdered)
                    transition.timeOrdered = true;
            }                
                
                
            if(transition.timeOrdered) { */
            timeStep++;
            transition.xLoc = (currProc * xGap) + xBase;                        
            transition.yLoc = (timeStep+1) * yGap + yRef;  
            
            GraphConstants.setBounds(Nodes.get(i).getAttributes(),
                    new Rectangle2D.Double(transition.xLoc, transition.yLoc, CellWidth, CellHeight));            
            }
        }            
    
    // build issue order graph
    public void buildIssueOrderGraph() {
        this.removeAll();          
        Collections.sort(Nodes, new InternalIssueOrderSorter());
        relocateCellsforIssueOrder();
        addNodes();
        addEdges();
        addDeadlock();        
    }
    
    // build time ordered graph
    public void buildTimeOrderedGraph() {
        this.removeAll();        
        
        Collections.sort(Nodes, new InternalIssueOrderSorter());        
        relocateCellsforTimeOrder();
        addNodes();
        addEdges();
        addDeadlock();
    }

    public void setCellLocation() {        
        int xLoc = -80;
        int yLoc = 35;
        int currProc = -1;
        int pCounter = 0;

        for (int i = 0; i < Nodes.size(); i++) {
            if (currProc < Nodes.get(i).pID) {
                currProc = Nodes.get(i).pID;
                addProcessLabels(pCounter, currProc);
                xLoc += xGap;   
                yLoc = yGap + yBase;        // reset Y
                pCounter++;
            }
            yLoc += yGap;
            GraphConstants.setBounds(Nodes.get(i).getAttributes(),
                    new Rectangle2D.Double(xLoc, yLoc, CellWidth, CellHeight));            
        }
    }

    public void addNodes() {
        if (!Nodes.isEmpty()) {
            this.getGraphLayoutCache().insert(Nodes.toArray());
        }
    }

    public void addEdges() {
        if (!Edges.isEmpty()) {
            this.getGraphLayoutCache().insert(Edges.toArray());
        }
    }        

    // program order graph
    public void buildGraph() {
        // sort the cells in program order
        this.removeAll();
        Collections.sort(Nodes);
        
        setCellLocation();
        addNodes();
        addEdges();
        addDeadlock();
    }

    public void buildintraCBGraph() {
	this.removeAll();

	Collections.sort(Nodes);
	setCellLocation();
	addNodes();
	addEdges();
   }
        
    public void addDeadlock() {        
        // add deadlock cell
        int nRows = 0;
        int yRef = yGap + yBase;
        int iIndex = GlobalStructure.getInstance().currInterleaving;
        int nP = GlobalStructure.getInstance().noProcess;
        for(int i = 0; i < nP; i++)
            nRows += GlobalStructure.getInstance().interleavings.get(iIndex-1).tList.size();
        nRows = nRows / nP;
        
        if(iIndex == GlobalStructure.getInstance().nInterleavings) {
            if(GlobalStructure.getInstance().deadlocked) {                
                // compute x,y for deadlock cell
                int xLoc = (xBase + xGap + CellWidth + CellWidth/2);      // starts after first row
                int yLoc = (nRows + 3) * yGap + yRef;       // allow 3 approximations 
                double width = (GlobalStructure.getInstance().noProcess-2) * (xGap) ;
                addDeadlockCell(xLoc, yLoc, width);
                
            }
        }
    }
         
    private void addDeadlockCell(int xLoc, int yLoc, double width) {
                DefaultGraphCell deadlock = GlobalStructure.getInstance().deadlockRef;
                if(deadlock == null) {
                    deadlock = new DefaultGraphCell("DEADLOCK");
                //GraphConstants.setGradientColor(deadlock.getAttributes(), Color.RED);
                    Color color = new Color((float)0.98, (float)0.01, (float)0.28);
                    GraphConstants.setBorder(deadlock.getAttributes(), BorderFactory.createLineBorder(color, 3));         
                    GraphConstants.setOpaque(deadlock.getAttributes(), true);
                    GraphConstants.setEditable(deadlock.getAttributes(), false);
                    GraphConstants.setMoveable(deadlock.getAttributes(), false);                               
                    GraphConstants.setBounds(deadlock.getAttributes(), new Rectangle2D.Double(xLoc, yLoc, width, 20));
                    GraphConstants.setFont(deadlock.getAttributes(), new Font("Tahoma", Font.BOLD, 14));
                    GlobalStructure.getInstance().deadlockRef = deadlock;
                }
                else {
                    GraphConstants.setBounds(deadlock.getAttributes(), 
                            new Rectangle2D.Double(xLoc, yLoc, CellWidth, CellHeight));
                }                                    
                // insert the cell
                this.getGraphLayoutCache().insert(deadlock); 
                
    }
        
    private void addProcessLabels(int pCounter, int pID) {
        int X, Y;        
        processLabel Cell = new processLabel(pID);
        Cell.setText();
        
        // compute X,Y
        X = ((pCounter + 1) * xGap )+ xBase;
        Y = (1 * yGap) + yBase;           // y = 1 -> process label being the first 
        
        GraphConstants.setBounds(Cell.getAttributes(),
                new Rectangle2D.Double(X, Y, CellWidth, CellHeight));
        Color color = GlobalStructure.getInstance().procColors.get(pID);
        //GraphConstants.setGradientColor(Cell.getAttributes(), color);
        GraphConstants.setBorder(Cell.getAttributes(), BorderFactory.createLineBorder(color, 3));
        GraphConstants.setOpaque(Cell.getAttributes(), true);
        GraphConstants.setEditable(Cell.getAttributes(), false);
        GraphConstants.setMoveable(Cell.getAttributes(), false);
        this.getGraphLayoutCache().insert(Cell);
    }

    public void CleanUp() {
        Nodes.clear();
        Edges.clear();
        this.removeAll();
    }

    @Override
    public String getToolTipText(MouseEvent e) {
        Object obj = null;
        if (e != null) {
            obj = this.getFirstCellForLocation(e.getX(), e.getY());
        }
        if (obj instanceof Transition) {
            return ((Transition) obj).getToolTipString();
        }
        return null;
    }
}
