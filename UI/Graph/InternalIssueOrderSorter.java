/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package Graph;

import Data.Transition;
import java.util.Comparator;

/**
 *
 * @author Sriram Aananthakrishnan <sriram@cs.utah.edu>
 */
public class InternalIssueOrderSorter implements Comparator<Transition> {
    
    public int compare(Transition T1, Transition T2) {
        if(T1.issueID == -1 && T2.issueID == -1)
            return (T1.orderID - T2.orderID);
        else if (T1.issueID == -1)
            return 1;
        else if (T2.issueID == -1)
            return -1;
        else
            return ((Integer)T1.issueID).compareTo((Integer)T2.issueID);
    }

}
