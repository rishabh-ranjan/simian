package Data;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Sriram
 * 
 * 
 */
import java.util.*;
import org.jgraph.graph.DefaultGraphCell;
import org.jgraph.graph.GraphConstants;
import java.awt.geom.Point2D;
import java.awt.Color;
import java.awt.Font;
import org.jgraph.graph.DefaultPort;
import javax.swing.BorderFactory;

public class Transition extends DefaultGraphCell implements Comparable<Transition> {

    public int pID;
    public int interleaving_no;
    public String function;
    public String Communicator;
    public String src_or_dst;
    public String tag;
    public ArrayList<Integer> intraCB;
    public ArrayList<InterCBTemplate> interCB;
    public int match_pID;
    public int match_index;
    public int index;
    public final static int WIDTH = 80;
    public final static int HEIGHT = 20;
    public String hashKey;
    public String filename;
    public int lineNo;    
    public int xLoc;
    public int yLoc;
    public boolean showInterCB;
    public boolean intraCBCell;
    public Color color;
    public DefaultPort lPort,  rPort,  defaultPort;
    public int offset;
    public int orderID;     //program order
    public int issueID;     // internal issue order by ISP
    public boolean timeOrdered;

    public Transition() {
        super();
        intraCB = new ArrayList<Integer>();
        interCB = new ArrayList<InterCBTemplate>();
        intraCBCell = false;
        lPort = new DefaultPort();
        rPort = new DefaultPort();
        defaultPort = new DefaultPort();
        showInterCB = false;
        src_or_dst = "0";
        xLoc = 0;
        yLoc = 0;
        timeOrdered = false;        
//        CreateCell();
    }

    public void printTransition() {
        System.out.println(function);
        System.out.println(pID);
        System.out.println(index);
        System.out.println(Communicator);
        System.out.println("IntraCBs");
        for (int i = 0; i < intraCB.size(); i++) {
            System.out.println(intraCB.get(i));
        }
        System.out.println(match_pID);
        System.out.println(match_index);
        System.out.println("File : " + filename);
        System.out.println("LineNo : " + lineNo);
        System.out.println();
    }

    public void CreateCell() {
        Color color = Color.BLACK;

        this.add(defaultPort);
        defaultPort.setParent(this);
        GraphConstants.setOffset(lPort.getAttributes(), new Point2D.Double(0, GraphConstants.PERMILLE / 2));
        GraphConstants.setOffset(rPort.getAttributes(), new Point2D.Double(GraphConstants.PERMILLE, GraphConstants.PERMILLE / 2));
        this.add(lPort);
        this.add(rPort);
        lPort.setParent(this);
        rPort.setParent(this);

        // gets new color for collective calls               

        communicatorColor commColor, cPtr;
        if (GlobalStructure.getInstance().CommunicatorColor.isEmpty()) {
            color = new Color((float)0, (float)0.71, (float)0.93);
            commColor = new communicatorColor(this.Communicator, color);
            GlobalStructure.getInstance().CommunicatorColor.add(commColor);
        } else {
            if (Communicator != null) {
                boolean flag = false;
                for (int i = 0; i < GlobalStructure.getInstance().CommunicatorColor.size(); i++) {
                    cPtr = GlobalStructure.getInstance().CommunicatorColor.get(i);
                    if (Communicator != null && cPtr.isEqual(Communicator)) {
                        color = cPtr.commcolor;
                        flag = true;
                        break;
                    }
                }
                if (!flag) {
                    color = getCommunicatorColor();
                    commColor = new communicatorColor(Communicator, color);
                    GlobalStructure.getInstance().CommunicatorColor.add(commColor);
                }
            }
        }

        if ((function.contains("recv") || function.contains("Recv") || function.contains("probe") || function.contains("Probe")) && Integer.parseInt(src_or_dst) == -1) {
            //GraphConstants.setBorder(this.getAttributes(), BorderFactory.createLineBorder(Color.RED));
            GraphConstants.setBorder(this.getAttributes(), BorderFactory.createLineBorder(Color.RED, 3));
        } else {
            GraphConstants.setBorder(this.getAttributes(), BorderFactory.createLineBorder(color, 3));
        }
        GraphConstants.setValue(this.getAttributes(), function);
        //GraphConstants.setGradientColor(this.getAttributes(), color);
        GraphConstants.setOpaque(this.getAttributes(), true);
        GraphConstants.setEditable(this.getAttributes(), false);
        GraphConstants.setMoveable(this.getAttributes(), false);
        GraphConstants.setResize(this.getAttributes(), false);
        GraphConstants.setFont(this.getAttributes(), new Font("Tahoma", Font.BOLD, 14));
//        GraphConstants.setSizeable(this.getAttributes(), false);
    }

    public void clearEdges() {
        defaultPort.removeAllChildren();
        lPort.removeAllChildren();
        rPort.removeAllChildren();
    }
    // returns true if the MPI calls is point to point
    public boolean isPointToPointSend() {
        if (this.function.contains("Send") || this.function.contains("send")) {
            return true;
        } else {
            return false;
        }
    }
    
    public boolean isWait() {
        if(this.function.contains("Wait") || this.function.contains("wait"))
            return true;
        return false;          
    }

    public boolean isPointToPointReceive() {
        if (this.function.contains("Recv") || this.function.contains("recv")) {
            return true;
        } else {
            return false;
        }
    }
    // returns true if the function is non-deterministic
    public boolean isNonDeterministic() {
        if (src_or_dst.equals("-1")) {
            return true;
        } else {
            return false;
        }
    }

    // returns true if the function is Probe or Iprobe
    public boolean isProbe() {
        if (this.function.contains("Probe") || this.function.contains("probe")) {
            return true;
        } else {
            return false;
        }
    }

    public boolean isCollective() {
        if (function.equals("Barrier") || function.equals("Allreduce") || function.equals("Bcast") ||
            function.equals("Reduce") || function.equals("Reduce_scatter") || function.equals("Scatter") || function.equals("Gather") || function.equals("Scatterv") || 
            function.equals("Gatherv") || function.equals("Allgather") || function.equals("Allgatherv") ||
            function.equals("Alltoall") || function.equals("Alltoallv") || function.equals("Scan") || function.equals("Comm_create") || function.equals("Cart_create") || 
            function.equals("Comm_dup") || function.equals("Comm_split") || function.equals("Comm_free") || function.equals("AllReduce")) {
            return true;
        } else {
            return false;
        }
    }
    
    public boolean isFinalize() {
        if(function.equals("Finalize"))
            return true;
        return false;
    }

    public Color getCommunicatorColor() {
        Random rand = new Random();
        Color color = new Color(rand.nextInt(256), rand.nextInt(256), rand.nextInt(256));
        return color;
    }
//    @Override
    public String getToolTipString() {
        StringBuffer name = new StringBuffer("MPI_" + function + "( ");

        if (function.equals("Isend") || function.equals("Send") || function.equals("Ssend")) {
            name.append(" dest = " + match_pID + ", ");
            name.append("Tag = " + tag + ", ");
        }
        if (function.equals("Recv") || function.equals("Irecv") || function.contains("Probe") || function.contains("probe")) {
            String src;
            if (Integer.parseInt(src_or_dst) < 0) {
                src = "MPI_ANY_SRC";
            } else {
                src = src_or_dst;
            }
            name.append("Src = " + src + ", ");
            name.append("Tag = " + tag + ", ");
        }
        if (!function.equals("Finalize")) {
            name.append("Comm = " + Communicator);
        }
        name.append(")");
        name.append("line : " + lineNo + ", ");
        name.append("File : " + filename + ".");
        return new String(name);
    }

    public int compareTo(Transition T) {
        if (this.pID == T.pID) {
            return this.index - T.index;
        } else {
            return this.pID - T.pID;
        }
    }     
}
