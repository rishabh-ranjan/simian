/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package Data;

import org.jgraph.graph.DefaultEdge;
import org.jgraph.graph.GraphConstants;
import java.awt.Color;

/**
 *
 * @author Sriram Aananthakrishnan <sriram@cs.utah.edu>
 */
public class ISPMatchEdge extends DefaultEdge {
    
    Color color;
    float[] pattern;
    boolean isDeterministic;

    public ISPMatchEdge(boolean isdeterministic){
        super();
        color = Color.BLACK;
        pattern = new float[2];
        isDeterministic = isdeterministic;
        setProperties();
    }
    
    public void setPattern() {
        if(!isDeterministic) {
            pattern[0] = 3; pattern[1] = 2;
        }
    }
        
    
    public void setProperties() {
        int arrow = GraphConstants.ARROW_CLASSIC;
        setPattern();

        if(!isDeterministic)
            GraphConstants.setDashPattern(this.getAttributes(), pattern);
        
        GraphConstants.setLineColor(this.getAttributes(), color);
        GraphConstants.setLineEnd(this.getAttributes(), arrow);
        GraphConstants.setEndFill(this.getAttributes(), true);
    }
    
    public String getToolTipString()
    {
        String tooltip;
        
        if(!isDeterministic)
            tooltip = "Non-Deterministic Edge\n";
        else
            tooltip = "Deterministic Edge\n";
        
        return tooltip;
        
    }
}
