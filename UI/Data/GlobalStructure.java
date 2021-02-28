package Data;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Sriram
 */
import java.util.ArrayList;
import java.awt.Color;
import java.io.File;
import java.util.Random;
import org.jgraph.graph.DefaultGraphCell;

class communicatorColor {
    String comm;
    Color commcolor;

    communicatorColor(String Comm, Color clr) {
        comm = Comm;
        commcolor = clr;
    }

    boolean isEqual(String Comm) {
        if (Comm.equals(comm)) {
            return true;
        } else {
            return false;
        }
    }
}

public class GlobalStructure {

    private static GlobalStructure instance = null;

    public static GlobalStructure getInstance() {
        if (instance == null) {
            instance = new GlobalStructure();
        }
        return instance;
    }        

    public void flush() {
//        instance = null;
        interleavings.clear();
        Exception = null;
        deadlocked = false;
        noProcess = 0;
        nInterleavings = 0;
        currProcess = -1;
        currInterleaving = 0;
        paintLeft = false;
        CommunicatorColor.clear();
        filenames.clear();
        sourceFiles.clear();
        OpenedFiles.clear();   
        deadlockRef = null;
    }

    public void printAllInterleavings() {
        for (int i = 0; i < interleavings.size(); i++) {
            interleavings.get(i).printInterleaving();
        }
    }

    public void setOffset(int proc, ArrayList<Integer> offsets, String filename) {
        ProcessTextSource procWindow = sourceFiles.get(proc);
        int bOffset = procWindow.bOffset;
        
        for(int i = 0; i < interleavings.size(); i++) {
            Interleavings interleaving = interleavings.get(i);
            for(int j = 0; j < noProcess; j++) {
                for(int k = 0; k < interleaving.tList.get(j).size(); k++) {
                    Transition transition = interleaving.tList.get(j).get(k);
                    if(transition.filename.equals(filename)) {
                        transition.offset = offsets.get(transition.lineNo) + bOffset;
                    }
                }
            }
        }
        
        // set offset of procWindow

        SourceFile srcFileRef = getOpenedFile(filename);
        if(srcFileRef != null)
            procWindow.bOffset = srcFileRef.myCharCount ;
       
//        for (int i = 0; i < interleavings.size(); i++) {
//            Interleavings interleaving = interleavings.get(i);
//            for (int j = 0; j < interleaving.tList.get(proc).size(); j++) {
//                int lno = interleaving.tList.get(proc).get(j).lineNo;
//                if (interleaving.tList.get(proc).get(j).filename.equals(filename)) {
//                    interleaving.tList.get(proc).get(j).offset = offsets.get(lno);
//                }
//            }
//        }
    }
    
    public void detProcColors() {
        Random r = new Random();
        for (int i = 0; i < noProcess; i++) {
            Color c = new Color(r.nextInt(256), r.nextInt(256), r.nextInt(256));
            procColors.add(c);
        }
    }
    
    public void resetTimeOrderFlag() {
        int iIndex = currInterleaving-1;
        Interleavings interleaving = interleavings.get(iIndex);
        
        for(int i = 0; i < noProcess; i++) {
            for(int j = 0; j < interleaving.tList.get(i).size(); j++) {
                Transition trans = interleaving.tList.get(i).get(j);
                trans.timeOrdered = false;                        
            }
        }
    }
    
    public void initSystem() {
        detProcColors();
        ProcessTextSource procTextSrcRef;
        
        // init ProcessTextSource
        for(int i = 0; i < noProcess; i++) {
            procTextSrcRef = new ProcessTextSource();
            procTextSrcRef.pID = i;
            sourceFiles.add(procTextSrcRef);
        }            
    }
    
    public void addSourceFile(String filename) {
        SourceFile srcFileRef = new SourceFile(filename);        
        if(!OpenedFiles.contains(srcFileRef))
            OpenedFiles.add(srcFileRef);
    }
    
    public SourceFile getOpenedFile(String filename) {
        SourceFile srcFileRef = null;
        
        int index = isOpened(filename);
        
        if(index != -1)
            srcFileRef = OpenedFiles.get(index);
        
        return srcFileRef;
    }
    
    public int isOpened(String filename) {          
        for(int i = 0; i < OpenedFiles.size(); i++) {
            if(OpenedFiles.get(i).fileName.equals(filename))
                return i;
        }
        return -1;
    }    

    public GlobalStructure() {
        deadlocked = false;
        noProcess = 0;
        nInterleavings = 0;
        currProcess = -1;
        currInterleaving = 0;
        interleavings = new ArrayList<Interleavings>();
        CommunicatorColor = new ArrayList<communicatorColor>();
        filenames = new ArrayList<String>();
        procColors = new ArrayList<Color>(); 
        sourceFiles = new ArrayList<ProcessTextSource>();
        OpenedFiles = new ArrayList<SourceFile>();
        paintLeft = false;
        Exception = null;
    }
    public int noProcess;
    public int nInterleavings;
    public boolean deadlocked;
    public ArrayList<String> filenames;    // GUI related, variables will be set when Action is Performed
    public int currProcess;
    public int currInterleaving;
    public boolean paintLeft;
    public String Exception;
    public ArrayList<Interleavings> interleavings;
    public ArrayList<communicatorColor> CommunicatorColor;   
    public ArrayList<Color> procColors;
    public ArrayList<SourceFile> OpenedFiles;
    public File CurrentDirectory;
    public DefaultGraphCell deadlockRef;
    
    public ArrayList<ProcessTextSource> sourceFiles;
}
