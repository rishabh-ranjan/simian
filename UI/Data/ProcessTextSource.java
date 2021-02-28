/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package Data;

import java.util.ArrayList;

/**
 *
 * @author Sriram Aananthakrishnan <sriram@cs.utah.edu>
 */
public class ProcessTextSource {
    public int pID;
    public String Contents;
    public int bOffset;
    public ArrayList<String> FileNames;
    
    public ProcessTextSource() {
        Contents = new String();
        bOffset = 0;
        FileNames = new ArrayList<String>();
    }
    
    public void appendContents(String contents) {
        Contents += contents;
        bOffset = Contents.length();
    }
    
    public void addFileName(String filename) {
        if(!isPresent(filename))
            FileNames.add(filename);
    }
    
    public boolean isPresent(String filename) {
        for(int i = 0; i < FileNames.size(); i++) {
            if(FileNames.get(i).equals(filename))
                return true;
        }
        return false;
    }
}
