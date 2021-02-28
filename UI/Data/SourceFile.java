/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package Data;

import java.io.File;
import java.io.FileReader;
import java.io.FileNotFoundException;
import java.io.BufferedReader;

import javax.swing.JFileChooser;
import java.util.ArrayList;

/**
 *
 * @author Sriram Aananthakrishnan <sriram@cs.utah.edu>
 */
public class SourceFile {
    public String fileName;
    public boolean isOpen;
    public String fileContents;
    public JFileChooser jfileChooser;
    public ArrayList<Integer> Offset;
    public int myCharCount;
    
    public SourceFile(String srcFile) {
        isOpen = false;
        fileName = srcFile;        
        jfileChooser = new JFileChooser();
        fileContents = new String();
        Offset = new ArrayList<Integer>();
        myCharCount = 0;
    }   
    
    public int getCharCount() {
        if(! Offset.isEmpty())
            return Offset.get(Offset.size()-1);
        else
            return -1;
    }
    
    // opens file and sets offset
    public void readFile(int proc) {        
        int charCount;
        FileReader fR = getFileReader(proc);
        ArrayList<Integer> offsetCounts = new ArrayList<Integer>();        
        try{            
            BufferedReader br = new BufferedReader(fR);
            String line = new String();
            charCount = 0;
            while (line != null) {
                line = br.readLine();
                if (line != null) {
                    fileContents += line;
                    fileContents += "\n";
                    offsetCounts.add(charCount);        // not required
                    
                    Offset.add(charCount);
                    charCount += line.length() + 1;
                }
            }                        
            // set base offset
            myCharCount = charCount;
            // set offset for all transitions using this file
            GlobalStructure.getInstance().setOffset(proc, Offset, fileName);          
            
        }
        catch(Exception e)
        {
            new javax.swing.JOptionPane().
                    showMessageDialog(null, "Err reading file for process : " + proc);                        
        }            
    }
    
    public FileReader getFileReader(int proc) {        
        File file;
        FileReader fr;
        boolean localdir = false;
        fr = null;
        file = null;
        try {
            try {
                try {
                    file = new File(fileName);
                    fr = new FileReader(file);
                    localdir = false;
                }
                catch(FileNotFoundException e)
                {
                    localdir = true;
                }
                if(localdir) 
                {   
                    file = new File(GlobalStructure.getInstance().CurrentDirectory, fileName);            
                    fr = new FileReader(file);
                }                    
            }   
            catch(FileNotFoundException e)
            {
                new javax.swing.JOptionPane().
                    showMessageDialog(null, "Locate the file to load for process : " + proc);
                
                int returnval = jfileChooser.showOpenDialog(null);
                jfileChooser.setCurrentDirectory(GlobalStructure.getInstance().CurrentDirectory);
                if(returnval == JFileChooser.APPROVE_OPTION)
                {
                    file = null;
                    file = jfileChooser.getSelectedFile();                
                    fr = new FileReader(file);                    
                }
                else                              
                    fr = null;                
            }                        
        } catch (Exception e) {            
            new javax.swing.JOptionPane().
                    showMessageDialog(null, "File not available to read for process : " + proc);
                    System.exit(-1);
        }        
        return fr;
    }  
}
