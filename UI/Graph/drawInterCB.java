package Graph;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author sriram
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
import java.awt.geom.Rectangle2D;
import java.awt.geom.Line2D;

import Data.*;

public class drawInterCB {

    ISPGraphModel myGraph;

    public drawInterCB(ISPGraphModel Graph) {
        myGraph = Graph;
    }

    public void CreateEdges(ArrayList<Transition> Nodes, boolean both) {
        int iIndex = GlobalStructure.getInstance().currInterleaving - 1;
        DefaultEdge eTemp;
        Transition source, target;
        int arrow = GraphConstants.ARROW_SIMPLE;
        interCBRouting myRouting = new interCBRouting();

        for (int i = 0; i < Nodes.size(); i++) {

            for (int j = 0; j < Nodes.get(i).interCB.size(); j++) {
                source = Nodes.get(i);
                int pid = Nodes.get(i).interCB.get(j).pID;
                int index = Nodes.get(i).interCB.get(j).index;
                target = GlobalStructure.getInstance().interleavings.get(iIndex).tList.get(pid).get(index);

                if (!myGraph.Nodes.contains(source)) {
                    myGraph.Nodes.add(source);
                }
                if (!myGraph.Nodes.contains(target)) {
                    myGraph.Nodes.add(target);
                }
                if (!Nodes.contains(target) && both) {
                    Nodes.add(target);
                }
                eTemp = new DefaultEdge();
                eTemp.setSource(source.getChildAt(0));
                eTemp.setTarget(target.getChildAt(0));
                GraphConstants.setLineColor(eTemp.getAttributes(), Color.RED);
                GraphConstants.setLineEnd(eTemp.getAttributes(), arrow);
                GraphConstants.setEndFill(eTemp.getAttributes(), true);
                GraphConstants.setLineWidth(eTemp.getAttributes(), 2);
                GraphConstants.setRouting(eTemp.getAttributes(), myRouting);
                myGraph.Edges.add(eTemp);
            }

        }
    }
    //Routing Class
    public static class interCBRouting implements Edge.Routing {

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
}
