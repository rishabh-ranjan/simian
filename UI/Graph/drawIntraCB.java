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
import org.jgraph.graph.Edge;
import org.jgraph.graph.EdgeView;
import org.jgraph.graph.PortView;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.awt.geom.Rectangle2D;

import Data.*;

public class drawIntraCB {

    ISPGraphModel myGraph;

    public drawIntraCB(ISPGraphModel graph) {
        myGraph = graph;
    }

    public void CreateEdges(ArrayList<Transition> Nodes, boolean forProcess) {
        int iIndex = GlobalStructure.getInstance().currInterleaving - 1;
        Interleavings interleaving = GlobalStructure.getInstance().interleavings.get(iIndex);

        Transition source, target;

        for (int i = 0; i < Nodes.size(); i++) {
            boolean lPaint = GlobalStructure.getInstance().paintLeft;
            int pIndex = Nodes.get(i).pID;
            int index = Nodes.get(i).index;

            if (!forProcess) {
                for (int j = 0; j < index; j++) {
                    Transition tTemp = interleaving.tList.get(pIndex).get(j);
                    if (tTemp.intraCB.contains(Nodes.get(i).index)) {
                        source = tTemp;
                        target = Nodes.get(i);
                        if (!myGraph.Nodes.contains(source)) {
                            makeEdges(lPaint, source, target);
                        } else {
                            if (!Nodes.contains(source)) {
                                makeEdges(lPaint, source, target);
                            }
                        }
                    }
                }
            }

            for (int j = 0; j < Nodes.get(i).intraCB.size(); j++) {
                source = Nodes.get(i);
                target = GlobalStructure.getInstance().interleavings.get(iIndex).tList.get(pIndex).get(Nodes.get(i).intraCB.get(j));
                makeEdges(lPaint, source, target);
            }
            if (!lPaint) {
                GlobalStructure.getInstance().paintLeft = true;
            } else {
                GlobalStructure.getInstance().paintLeft = false;
            }
        }
    }

    public void makeEdges(boolean lPaint, Transition source, Transition target) {
        DefaultEdge eTemp;
        Random randColor;
        Color edgeColor;
        int arrow = GraphConstants.ARROW_SIMPLE;
        ispRouting myRouting = new ispRouting();
        //randColor = new Random();
        //edgeColor = new Color(randColor.nextInt(256), randColor.nextInt(256), randColor.nextInt(256));
        
        edgeColor = Color.BLUE;

        if (!myGraph.Nodes.contains(source)) {
            myGraph.Nodes.add(source);
        }
        if (!myGraph.Nodes.contains(target)) {
            myGraph.Nodes.add(target);
        }
        eTemp = new DefaultEdge();     
        GraphConstants.setLineColor(eTemp.getAttributes(), edgeColor);
        if (!lPaint) {
            eTemp.setSource(source.getChildAt(1));
            eTemp.setTarget(target.getChildAt(1));
        } else {
            eTemp.setSource(source.getChildAt(2));
            eTemp.setTarget(target.getChildAt(2));
        }
        if (!myGraph.Edges.contains(eTemp)) {
            GraphConstants.setLineEnd(eTemp.getAttributes(), arrow);
            GraphConstants.setEndFill(eTemp.getAttributes(), true);
            GraphConstants.setLineWidth(eTemp.getAttributes(), 2);
            GraphConstants.setLineStyle(eTemp.getAttributes(), GraphConstants.STYLE_SPLINE);
            GraphConstants.setRouting(eTemp.getAttributes(), myRouting);
            myGraph.Edges.add(eTemp);
        }
    }

//Routing Class
    public static class ispRouting implements Edge.Routing {

        public int getPreferredLineStyle(EdgeView edge) {
            return NO_PREFERENCE;
        }

        public List<Object> route(GraphLayoutCache cache, EdgeView edge) {
            List<Object> points = new ArrayList<Object>();
            if (edge.getSource() instanceof PortView && edge.getTarget() instanceof PortView) {
                PortView sourcePort = (PortView) edge.getSource();
                PortView targetPort = (PortView) edge.getTarget();
                double srcPortX, srcPortY, trgPortX, trgPortY;
                srcPortX = srcPortY = trgPortX = trgPortY = 0.0;
                try {
                    srcPortX = sourcePort.getLocation(edge).getX();
                    srcPortY = sourcePort.getLocation(edge).getY();
                    trgPortX = targetPort.getLocation(edge).getX();
                    trgPortY = targetPort.getLocation(edge).getY();
                } catch (Exception e) {
                    System.out.println(e.getCause());
                }

                double x0 = 0.0, x1 = 0.0, x2 = 0.0, x3 = 0.0;
                double y0 = 0.0, y1 = 0.0, y2 = 0.0, y3 = 0.0;

                Rectangle2D srcRect = edge.getSource().getParentView().getBounds();
                Rectangle2D trgRect = edge.getTarget().getParentView().getBounds();

                double srcCellX = srcRect.getX();

                if (srcPortX <= srcCellX) {
                    x0 = srcPortX - srcRect.getWidth() / 2;
                    y0 = srcPortY;

                    y1 = (srcPortY + trgPortY) / 2;
                    //x1 = x0 - horizantalOffset;
                    x1 = x0 - Math.sqrt(trgPortY - srcPortY);


                    x2 = trgPortX - trgRect.getWidth() / 2;
                    y2 = trgPortY;
                } else if (srcPortX > srcCellX) {
                    x0 = srcPortX + srcRect.getWidth() / 2;
                    y0 = srcPortY;

                    y1 = (srcPortY + trgPortY) / 2;
                    //x1 = x0 - horizantalOffset;
                    x1 = x0 + Math.sqrt(trgPortY - srcPortY);


                    x2 = trgPortX + trgRect.getWidth() / 2;
                    y2 = trgPortY;
                }
                points.add(sourcePort.getLocation(edge));
                points.add(edge.getAttributes().createPoint(x1, y1));
                points.add(edge.getAttributes().createPoint(x2, y2));
            }
            return points;
        }
    }
}
