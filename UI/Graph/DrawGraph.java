package Graph;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Sriram
 */
import org.jgraph.graph.GraphConstants;
import org.jgraph.graph.GraphLayoutCache;
import org.jgraph.graph.DefaultEdge;
import org.jgraph.plaf.basic.BasicGraphUI;
import org.jgraph.graph.Edge;
import org.jgraph.graph.EdgeView;
import org.jgraph.graph.PortView;
import org.jgraph.graph.DefaultPort;


import java.awt.Color;
import java.util.ArrayList;
import java.util.List;
import java.awt.event.MouseEvent;
import javax.swing.ToolTipManager;
import java.awt.geom.Rectangle2D;
import java.awt.geom.Line2D;

import Data.*;

public class DrawGraph extends ISPGraphModel {

    public ArrayList<DefaultEdge> MatchEdgeSet;
    public BasicGraphUI graphUI;
    public SourceView srcView;

    public DrawGraph() {
        super();
        MatchEdgeSet = new ArrayList<DefaultEdge>();
        ToolTipManager.sharedInstance().registerComponent(this);
        this.addMouseListener(new MyGraphMouseListener(this));
    }

    public void CreateCells(Interleavings interLeavings) {

        for (int i = 0; i < interLeavings.tList.size(); i++) {
            this.copyNodes(interLeavings.tList.get(i));
        }
    }

    public void CreateCells() {
        int iIndex = GlobalStructure.getInstance().currInterleaving - 1;
        Interleavings interleaving = GlobalStructure.getInstance().interleavings.get(iIndex);
        CreateCells(interleaving);
                
    }

    public void CreateEdges(Interleavings interLeavings) {

        int match_pID;
        int match_index;

        MatchEdgeSet.clear();
        
        Transition tSrc, tDst;     
        DefaultEdge eTemp;

        for (int i = 0; i < interLeavings.tList.size(); i++) {
            for (int j = 0; j < interLeavings.tList.get(i).size(); j++) {
                tSrc = interLeavings.tList.get(i).get(j);
                match_pID = tSrc.match_pID;
                match_index = tSrc.match_index;                
                
                if (match_pID != -1 && match_index != -1 && (tSrc.isPointToPointSend() || tSrc.isProbe()) ){                
                    tDst = interLeavings.tList.get(match_pID).get(match_index);                   
                    
                    if(tDst.isNonDeterministic() || tSrc.isNonDeterministic())
                        eTemp = new ISPMatchEdge(false);
                    else
                        eTemp = new ISPMatchEdge(true);
                                        
                      if(tSrc.isProbe()) {
                        eTemp.setSource(tDst.getChildAt(0));
                        eTemp.setTarget(interLeavings.tList.get(i).get(j).getChildAt(0));
                    }
                    else {
                        eTemp.setSource(interLeavings.tList.get(i).get(j).getChildAt(0));                    
                        eTemp.setTarget(tDst.getChildAt(0));
                    }
                    MatchEdgeSet.add(eTemp);
                }
            }
        }
    }

    public void setSourceView(SourceView src) {
        srcView = src;
    }

    public void highlightSource(ArrayList<Transition> transitions) {
        srcView.flushSourceDisplay();
//        srcView.identifySource(transition);
        
        for (int i = 0; i < transitions.size(); i++) {
            srcView.identifySource(transitions.get(i));
        }
    }

    public void addMatchEdges() {       
        DefaultEdge eTemp;        
        MyRouting myRouting = new MyRouting();
        for (int i = 0; i < MatchEdgeSet.size(); i++) {
            eTemp = MatchEdgeSet.get(i);
            GraphConstants.setRouting(eTemp.getAttributes(), myRouting);
        }
        this.getGraphLayoutCache().insert(MatchEdgeSet.toArray());
    }

    public void drawGraph() {
        // removes cells if any present
        this.removeAll();        
        buildGraph();
        addMatchEdges();
    }
    
    public void drawIssueOrderGraph() {
        // remove cells if any present
        this.removeAll();
        
        buildIssueOrderGraph();
        addMatchEdges();        
    }
    
    public void drawTimeOrderedGraph() {
        // remove cells if any present
        this.removeAll();
        
        buildTimeOrderedGraph();
        addMatchEdges();
    }

    //Routing Class
    public static class MyRouting implements Edge.Routing {

        public int getPreferredLineStyle(EdgeView edge) {
            return NO_PREFERENCE;
        }

        @SuppressWarnings("empty-statement")
        public List<Object> route(GraphLayoutCache cache, EdgeView edge) {
            List<Object> points = new ArrayList<Object>();
            if (edge.getSource() instanceof PortView && edge.getTarget() instanceof PortView) {
                PortView sourcePort = (PortView) edge.getSource();
                PortView targetPort = (PortView) edge.getTarget();

                int horizontalOffset = 50;                

                double px1 = sourcePort.getLocation(edge).getX();
                double py1 = sourcePort.getLocation(edge).getY();
                double px2 = targetPort.getLocation(edge).getX();
                double py2 = targetPort.getLocation(edge).getY();

                double x0 = 0.0;
                double y0 = 0.0;

                Rectangle2D srcRect = edge.getSource().getParentView().getBounds();
                Rectangle2D trgRect = edge.getTarget().getParentView().getBounds();

                double rectHt = srcRect.getHeight();
                double rectWd = srcRect.getWidth();

                Line2D.Double line = new Line2D.Double(px1, py1, px2, py2);
                // Line is tested for intersection with cells along the path

                double X1 = srcRect.getX();
                double Y1 = srcRect.getY();

                double X2 = trgRect.getX();
                double Y2 = trgRect.getY();

                Rectangle2D.Double rTemp = new Rectangle2D.Double();
                boolean touch = false;

                if (X1 <= X2) // edge L->R
                {
                    X1 += 150;      // Skips the src cell for line rectangle intersection test  

                    X2 -= 150;      // Skips the trg cell for line rectangle intersection test

                    while (X1 <= X2) {
                        touch = true;
                        Y1 = srcRect.getY();    // tests cells for all Y corresponding to X

                        Y2 = trgRect.getY();
                        if (Y1 == Y2) // edge is pulled if cells are on same row
                        {
                            rTemp.setRect(X1, Y1, rectWd, rectHt);
                            if (rTemp.intersectsLine(line)) {
                                x0 = (px1 + px2) / 2;                   // midpoint is pulled out

                                y0 = (py1 + py2) / 2 - horizontalOffset;
                                GraphConstants.setLineStyle(edge.getAttributes(), GraphConstants.STYLE_SPLINE);
                                break;
                            } else {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2;
                            }
                        }
                        while (Y1 < Y2) // All Cells along a row are tested for intersection
                        {                                   // top to bottom    

                            Y1 += 75;
                            rTemp.setRect(X1, Y1, rectWd, rectHt);
                            if (rTemp.intersectsLine(line)) {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2 - horizontalOffset;
                                GraphConstants.setLineStyle(edge.getAttributes(), GraphConstants.STYLE_SPLINE);
                                break;
                            } else {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2;
                            }
                        }
                        while (Y1 > Y2) // bottom to top
                        {
                            Y1 -= 75;
                            rTemp.setRect(X1, Y1, rectWd, rectHt);
                            if (rTemp.intersectsLine(line)) {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2 - horizontalOffset;
                                GraphConstants.setLineStyle(edge.getAttributes(), GraphConstants.STYLE_SPLINE);
                                break;
                            } else {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2;
                            }
                        }
                        X1 += 150;
                    }
                    if (!touch) // this part executes if the cells are adjacent
                    {
                        x0 = (px1 + px2) / 2;
                        y0 = (py1 + py2) / 2;
                    }
                } else // edge R->L
                {
                    X1 -= 150;      // skips src cell

                    X2 += 150;      // skips target cell

                    while (X1 >= X2) {
                        touch = true;
                        Y1 = srcRect.getY();
                        Y2 = trgRect.getY();
                        if (Y1 == Y2) {
                            rTemp.setRect(X1, Y1, rectWd, rectHt);
                            if (rTemp.intersectsLine(line)) {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2 - horizontalOffset;
                                GraphConstants.setLineStyle(edge.getAttributes(), GraphConstants.STYLE_SPLINE);
                                break;
                            } else {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2;
                            }
                        }
                        while (Y1 < Y2) {
                            Y1 += 75;
                            rTemp.setRect(X1, Y1, rectWd, rectHt);
                            if (rTemp.intersectsLine(line)) {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2 - horizontalOffset;
                                GraphConstants.setLineStyle(edge.getAttributes(), GraphConstants.STYLE_SPLINE);
                                break;
                            } else {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2;
                            }
                        }
                        while (Y1 > Y2) {
                            Y1 -= 75;
                            rTemp.setRect(X1, Y1, rectWd, rectHt);
                            if (rTemp.intersectsLine(line)) {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2 - horizontalOffset;
                                GraphConstants.setLineStyle(edge.getAttributes(), GraphConstants.STYLE_SPLINE);
                                break;
                            } else {
                                x0 = (px1 + px2) / 2;
                                y0 = (py1 + py2) / 2;
                            }
                        }

                        X1 -= 150;
                    }
                    if (!touch) {
                        x0 = (px1 + px2) / 2;
                        y0 = (py1 + py2) / 2;
                    }
                }

                points.add(sourcePort.getLocation(edge));
                points.add(edge.getAttributes().createPoint(x0, y0));
                points.add(targetPort.getLocation(edge));
            }
            return points;
        }
    }

    void destroy() {
        CleanUp();
        MatchEdgeSet.clear();
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
        
        if(obj instanceof ISPMatchEdge) {
            return ((ISPMatchEdge)obj).getToolTipString();
        }
        return null;
    }
}
