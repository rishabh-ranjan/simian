package Data;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Sriram
 */
import java.util.*;

public class Interleavings {

    public ArrayList<ArrayList<Transition>> tList;    
    public int iNo;
    public int nP;

    public Interleavings(int nP) {
        if(nP != 0)
            this.nP = nP;
        else
            System.err.print("Invalid process in interleaving initialization");
        
        tList = new ArrayList<ArrayList<Transition>>();        

        if (this.nP > 0) {
            for (int i = 0; i < this.nP; i++) {
                tList.add(new ArrayList<Transition>());
            }
        } else {
            System.exit(0);
        }            
    }
        

    public void printInterleaving() {
        System.out.println("Interleaving No : " + iNo);
        for (int i = 0; i < tList.size(); i++) {
            for (int j = 0; j < tList.get(i).size(); j++) {
                tList.get(i).get(j).printTransition();
            }
        }
    }

    public Transition getTransition(int pID, int index) {
        if (pID != -1 && index != -1) {
            return tList.get(pID).get(index);
        }
        return null;
    }
}
