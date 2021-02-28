package Graph;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Sriram
 */
import Graph.DrawGraph;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import javax.swing.JMenu;
import java.util.ArrayList;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.util.Collections;
import org.jgraph.graph.GraphSelectionModel;
import javax.swing.SwingUtilities;

import org.apache.log4j.Logger;

import Data.*;

public class MyGraphMouseListener implements MouseListener {

    DrawGraph myGraph;
    Object[] objects;           // collects all of the selected items
    ArrayList<Transition> transitions;
    ArrayList<processLabel> procLabels;
    JPopupMenu jPopupViewOptions;
    JMenu submenu;
    JMenu jMenuViewIntraCB;
    JMenu jMenuViewInterCB;
    JMenu jMenuViewBoth;
    JMenuItem jMenuViewIntraCBRoot;
    JMenuItem jMenuViewIntraCBSeparate;
    JMenuItem jMenuViewInterCBRoot;
    JMenuItem jMenuViewInterCBSeparate;
    JMenuItem jMenuViewBothRoot;
    JMenuItem jMenuViewBothSeparate;
    JMenuItem jMenuClearAll;
    JPopupMenu popupclear;
    
    JPopupMenu jPopupProcIntraCB;
    JMenuItem jMenuitemViewIntraCB;
    
    Logger logger;

    MyGraphMouseListener(DrawGraph graph) {
        objects = null;
        myGraph = graph;
        int nop = GlobalStructure.getInstance().noProcess;

        myGraph.getSelectionModel().setSelectionMode(GraphSelectionModel.MULTIPLE_GRAPH_SELECTION);
        transitions = new ArrayList<Transition>();
        procLabels = new ArrayList<processLabel>();
        logger = Logger.getLogger(MyGraphMouseListener.class);

        initComponents();

    }

    public void initComponents() {
        jPopupViewOptions = new JPopupMenu();
        jPopupProcIntraCB = new JPopupMenu();
        
        jMenuViewIntraCB = new JMenu("View IntraCB");
        jMenuViewInterCB = new JMenu("View InterCB");
        jMenuViewBoth = new JMenu("View Both");

        jMenuViewIntraCBRoot = new JMenuItem();
        jMenuViewIntraCBRoot.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                jMenuViewIntraCBRootActionPerformed(e);
            }
        });
        jMenuViewIntraCBSeparate = new JMenuItem();
        jMenuViewIntraCBSeparate.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                jMenuViewIntraCBSeparateActionPerformed(e);
            }
        });
        jMenuViewIntraCBRoot.setText("in Root Window");
        jMenuViewIntraCBSeparate.setText("in Separate Window");

        jMenuViewInterCBRoot = new JMenuItem();
        jMenuViewInterCBRoot.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                jMenuViewInterCBRootActionPerformed(e);
            }
        });
        jMenuViewInterCBSeparate = new JMenuItem();
        jMenuViewInterCBSeparate.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                jMenuViewInterCBSeparateActionPerformed(e);
            }
        });
        jMenuViewInterCBRoot.setText("in Root Window");
        jMenuViewInterCBSeparate.setText("in Separate Window");


        jMenuViewBothRoot = new JMenuItem();
        jMenuViewBothRoot.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                jMenuViewBothRootActionPerformed(e);
            }
        });
        jMenuViewBothSeparate = new JMenuItem();
        jMenuViewBothSeparate.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                jMenuViewBothSeparateActionPerformed(e);
            }
            ;
        });
        jMenuViewBothRoot.setText("in Root Window");
        jMenuViewBothSeparate.setText("in Separate Window");

        jMenuClearAll = new JMenuItem();
        jMenuClearAll.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent evt) {
                myGraph.getGraphLayoutCache().remove(myGraph.Edges.toArray());
                myGraph.Edges.clear();
                myGraph.buildGraph();
                myGraph.repaint();
            }
        });
        
        // to display intraCB per process
        jMenuitemViewIntraCB = new JMenuItem();
        jMenuitemViewIntraCB.setText("View IntraCB");
        jMenuitemViewIntraCB.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e){
                jMenuViewProcIntraCB(e);
            }
        });
            
        jMenuViewIntraCB.add(jMenuViewIntraCBRoot);
        jMenuViewIntraCB.addSeparator();
        jMenuViewIntraCB.add(jMenuViewIntraCBSeparate);

        jMenuViewInterCB.add(jMenuViewInterCBRoot);
        jMenuViewInterCB.addSeparator();
        jMenuViewInterCB.add(jMenuViewInterCBSeparate);

        jMenuViewBoth.add(jMenuViewBothRoot);
        jMenuViewBoth.addSeparator();
        jMenuViewBoth.add(jMenuViewBothSeparate);

        jPopupViewOptions.add(jMenuViewIntraCB);
        jPopupViewOptions.addSeparator();
        jPopupViewOptions.add(jMenuViewInterCB);
        jPopupViewOptions.addSeparator();
        jPopupViewOptions.add(jMenuViewBoth);       

        jPopupProcIntraCB.add(jMenuitemViewIntraCB);

    //jPopupViewOptions.pack();
    }
    
    void jMenuViewProcIntraCB(ActionEvent evt) {
        int pID;
        int iIndex = GlobalStructure.getInstance().currInterleaving-1;
        Interleavings interleaving = GlobalStructure.getInstance().interleavings.get(iIndex);
        transitions.clear();
        ISPGraphModel intraCBGraph = new ISPGraphModel();    
        drawIntraCB intraCB = new drawIntraCB(intraCBGraph);
        
        for(int i=0;i < procLabels.size();i++)
        {
            pID = procLabels.get(i).procID;         
            intraCB.CreateEdges(interleaving.tList.get(pID),true);            
        }
        
        intraCBGraph.buildintraCBGraph();
        displayIntraCB Graph = new displayIntraCB(intraCBGraph); 
        transitions.clear();        
    }

    void jMenuViewIntraCBRootActionPerformed(ActionEvent evt) {
        logger.info("View IntraCB -> in Root Window clicked");
        Collections.sort(transitions);
        drawIntraCB intraCB = new drawIntraCB(myGraph);
        intraCB.CreateEdges(transitions, false);
        myGraph.buildGraph();
        myGraph.addEdges();
        myGraph.repaint();
        transitions.clear();

    }

    void jMenuViewIntraCBSeparateActionPerformed(ActionEvent evt) {
        logger.info("View IntraCB -> in Separate Window clicked");
        ISPGraphModel intraCBGraph = new ISPGraphModel();
        Collections.sort(transitions);
        intraCBGraph.copyNodes(transitions);
        drawIntraCB intraCB = new drawIntraCB(intraCBGraph);
        intraCB.CreateEdges(transitions, false);
        intraCBGraph.buildGraph();
        displayIntraCB Graph = new displayIntraCB(intraCBGraph);
        transitions.clear();
    }

    void jMenuViewInterCBRootActionPerformed(ActionEvent evt) {
        logger.info("View InterCB -> in Root Window clicked");
        Collections.sort(transitions);
        drawInterCB interCB = new drawInterCB(myGraph);
        myGraph.setCellLocation();
        interCB.CreateEdges(transitions, false);
        myGraph.buildGraph();
        myGraph.repaint();
        transitions.clear();
    }

    void jMenuViewInterCBSeparateActionPerformed(ActionEvent evt) {
        logger.info("View InterCB in Separate Window clicked");
        ISPGraphModel interCBGraph = new ISPGraphModel();
        Collections.sort(transitions);
        interCBGraph.copyNodes(transitions);
        drawInterCB interCB = new drawInterCB(interCBGraph);
        interCB.CreateEdges(transitions, false);
        interCBGraph.buildGraph();
        displayIntraCB Graph = new displayIntraCB(interCBGraph);
        interCBGraph.CleanUp();
        transitions.clear();
    }

    void jMenuViewBothRootActionPerformed(ActionEvent evt) {
        logger.info("View Both -> in Root Window clicked");
        Collections.sort(transitions);
        drawInterCB interCB = new drawInterCB(myGraph);
        interCB.CreateEdges(transitions, false);
        drawIntraCB intraCB = new drawIntraCB(myGraph);
        intraCB.CreateEdges(transitions, false);
        myGraph.setCellLocation();
        myGraph.buildGraph();
        myGraph.repaint();
        transitions.clear();

    }

    void jMenuViewBothSeparateActionPerformed(ActionEvent evt) {
        logger.info("View Both -> in Separate Window clicked");
        ISPGraphModel CBGraph = new ISPGraphModel();
        Collections.sort(transitions);
        CBGraph.copyNodes(transitions);
        drawInterCB interCB = new drawInterCB(CBGraph);
        interCB.CreateEdges(transitions, false);
        drawIntraCB intraCB = new drawIntraCB(CBGraph);
        intraCB.CreateEdges(transitions, false);
        CBGraph.buildGraph();
        displayIntraCB Graph = new displayIntraCB(CBGraph);
        transitions.clear();
    }
    
    void jMenuViewProcessIntraCB(ActionEvent e) {
        
    }

    public void mouseExited(MouseEvent e) {
    }

    public void mouseEntered(MouseEvent e) {
        myGraph.setEnabled(true);

    }

    public void mouseReleased(MouseEvent e) {
        
        objects = myGraph.getSelectionCells();
        if (objects != null) {
            for (int i = 0; i < objects.length; i++) {
                if (objects[i] instanceof Transition) {
                    Transition temp = (Transition) objects[i];
                    if (!transitions.contains(temp)) {
                        transitions.add(temp);
                    }
                }
                else if(objects[i] instanceof processLabel)
                {
                    processLabel lTemp = (processLabel) objects[i];
                    if( !procLabels.contains(lTemp))
                        procLabels.add(lTemp);                    
                }
            }
            objects = null;
        }
        if (myGraph.srcView.isShowing() && !transitions.isEmpty()) {            
                myGraph.highlightSource(transitions);       // only one transition
        }
    }

    public void mouseClicked(MouseEvent e) {
    }

    public void mousePressed(MouseEvent e) {
        if (SwingUtilities.isRightMouseButton(e)) {
            if( !transitions.isEmpty()) {
            myGraph.setEnabled(false);
            jPopupViewOptions.show(e.getComponent(), e.getX(), e.getY());
            logger.info("Showing Popup");
            }
            else if( !procLabels.isEmpty()) {
                myGraph.setEnabled(false);
                jPopupProcIntraCB.show(e.getComponent(), e.getX(), e.getY());
            }
        } else {
            transitions.clear();
            procLabels.clear();
        }
    }
}
