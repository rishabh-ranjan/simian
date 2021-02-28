/*
 * SourceView.java
 *
 * Created on January 8, 2009, 5:40 PM
 */
package Graph;

/**
 *
 * @author  sriram
 */
import Data.GlobalStructure;
import Data.Interleavings;
import Data.Transition;
import Data.ProcessTextSource;
import Data.SourceFile;


import javax.swing.JTextPane;
import java.awt.GridLayout;
import javax.swing.JFrame;
import javax.swing.BorderFactory;
import javax.swing.JScrollPane;
import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.JTabbedPane;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.border.LineBorder;
import javax.swing.JMenuItem;
import java.awt.Component;
import java.awt.Color;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import java.io.*;

import com.Ostermiller.Syntax.HighlightedDocument;
import javax.swing.text.*;

import java.util.ArrayList;
import java.util.Stack;

public class SourceView extends javax.swing.JPanel {

    /** Creates new form SourceView */
    public SourceView() {
        initComponents();
        initTextPanes();
        initMenus();
        jfilechooser = new JFileChooser();
//        painter = new DefaultHighlighter.DefaultHighlightPainter(Color.YELLOW);
        painters = new ArrayList<DefaultHighlighter.DefaultHighlightPainter>();
        isActive = true;
        hasContents = false;
        isFilled = new ArrayList<Boolean>(GlobalStructure.getInstance().noProcess);        
        initPainters();
    }
    
    public void initMenus()
    {
        viewTabbed = new JMenuItem();
        viewTabbed.setText("Tabbed View");
        viewTabbed.addActionListener(new ActionListener(){
            public void actionPerformed(ActionEvent evt){
                switchTabbedView();
            }
        });
        
        viewSplit = new JMenuItem();
        viewSplit.setText("Split View");
        viewSplit.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt){
                switchSplitView();
            }
        });
        
        viewmenu = new JMenu("View");
        viewmenu.setSize(60, 40);
        viewmenu.setBorder(LineBorder.createBlackLineBorder());
        viewmenu.setBorderPainted(true);
        viewmenu.add(viewTabbed);
        viewmenu.addSeparator();
        viewmenu.add(viewSplit);
        menubar = new JMenuBar();
        menubar.setBorder(LineBorder.createGrayLineBorder());
        menubar.setBorderPainted(true);        
        menubar.add(viewmenu);        
    }
    
    public void switchSplitView()
    {
        if(window != null)
        {                        
            window.remove(this);
            addEditorPanes(); 
            window.add(this);
            window.pack();  
        }
    }
    
    public void switchTabbedView()
    {
        if(window != null)
        {
            window.remove(this);
            addTabbedPanes();
            window.add(this);
            window.pack();
        }
    }
    
    public void updateSource(SourceFile sourcefile, int proc) {
        sourcefile.readFile(proc);
        if( ! sourcefile.fileContents.isEmpty()) {
            viewPanes.get(proc).setText(sourcefile.fileContents);
            sourcefile.isOpen = true;
            editorPanes.get(proc).repaint();
        }
    }

    // by default loads the first file from transition list for all processes
    public void writeSource() {
        //getSourceFileNames();
        //fillTextPanes();        
        int iIndex = GlobalStructure.getInstance().currInterleaving-1;
        Interleavings interleaving = GlobalStructure.getInstance().interleavings.get(iIndex);        
        
        ProcessTextSource procWindow;
        SourceFile srcFileRef;
        Transition transition;
        
        for(int i = 0; i < interleaving.nP; i++) {
            // get the first transition from every process
            transition = interleaving.tList.get(i).get(0);      // first transition            
            int srcIndex = GlobalStructure.getInstance().isOpened(transition.filename);
            if(srcIndex == -1)
                GlobalStructure.getInstance().addSourceFile(transition.filename);
            srcFileRef = GlobalStructure.getInstance().getOpenedFile(transition.filename);                        
            
            if(srcFileRef.fileContents.isEmpty())
                srcFileRef.readFile(i);
            
            procWindow = GlobalStructure.getInstance().sourceFiles.get(transition.pID);
            
            if( ! procWindow.isPresent(transition.filename)) {
                procWindow.addFileName(transition.filename);
                procWindow.appendContents(srcFileRef.fileContents);
            }
                                        
            if(!procWindow.Contents.isEmpty())
                viewPanes.get(i).setText(procWindow.Contents);
            else
                System.err.print("Err : WriteSource()");                                                
        }
        hasContents = true;
    }
    
    public void initPainters()
    {
        int np = GlobalStructure.getInstance().noProcess;
        for(int i = 0;i < np; i++)
        {
            Color c = GlobalStructure.getInstance().procColors.get(i);
            DefaultHighlighter.DefaultHighlightPainter lighter = new 
                    DefaultHighlighter.DefaultHighlightPainter(c);
            painters.add(lighter);
        }
    }

    public void showWindow() {
        if (window != null && isActive) {
            this.flushSourceDisplay();
            window.setVisible(true);
        }
    }

    public void showSource() {

        if (hasContents) {
            window = new JFrame("Source");
            window.addWindowListener(new WindowAdapter(){
                public void windowClosing(WindowEvent e)
                {
                    UI.jRadioButton_hideSource.setSelected(true);
                    window.setVisible(false);
                }
            });
            window.setJMenuBar(menubar);
//            window.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
            addTabbedPanes();                                    
            window.add(this);
            isActive = true;
            window.pack();
            window.setVisible(true);
        }
    }
    
    public void addTabbedPanes()
    {       
        this.removeAll();
        this.remove(TabbedPane);
        TabbedPane.removeAll();
        int np = GlobalStructure.getInstance().noProcess;
        for(int i=0; i < np; i++)
            if(hasContents)
            {
                Component panel = makePanel(editorPanes.get(i));
                String proc = "Process  : " + i;                
                TabbedPane.addTab(proc, panel);
            }
        this.add(TabbedPane);        
    }
    

    public void addEditorPanes()
    {    
        this.removeAll();
        int np = GlobalStructure.getInstance().noProcess;
        for(int i=0; i < np; i++)
            if(hasContents)
            {
                Component panel = makePanel(editorPanes.get(i));
                //String proc = "Process  : " + i;                
                this.add(panel);                
            }        
    }
    
    public Component makePanel(JScrollPane scrollPane)
    {
        JPanel panel = new JPanel(false);
        panel.add(scrollPane,null);
        panel.setLayout(new GridLayout(1,1));
        panel.add(scrollPane);        
        return panel;
    }

    public void killWindow() {
        if (window != null) {
            window.setVisible(false);            
            window.dispose();
        }
    }

//    public void getSourceFileNames() {
//        // get the first interleaving
//        GlobalStructure instance = GlobalStructure.getInstance();
//        Interleavings interleavings = instance.interleavings.get(0);
//
//        int nP = instance.noProcess;
//
//        for (int i = 0; i < nP; i++) {
//            instance.filenames.add(interleavings.tList.get(i).get(0).filename);
//        }        
//    }
//
//    public void fillTextPanes() {
//        int nP = GlobalStructure.getInstance().noProcess;
//        String contents = new String();
//
//        String filename;
//
//        for (int i = 0; i < nP; i++) {
//            filename = GlobalStructure.getInstance().filenames.get(i);
//            FileReader fr = getFileReader(filename, i);
//            if(fr != null)
//                 contents = readFile(fr, i,filename);
//            if(contents != null)
//                viewPanes.get(i).setText(contents);
//        }
//    }
//    
//    public String readFile(FileReader fr,int proc, String filename)
//    {        
//        int charCount;
//        ArrayList<Integer> offsetCounts = new ArrayList<Integer>();
//        String fileContents = new String();        
//        try{            
//            BufferedReader br = new BufferedReader(fr);
//            String line = new String();
//            charCount = 0;
//            while (line != null) {
//                line = br.readLine();
//                if (line != null) {
//                    fileContents += line;
//                    fileContents += "\n";
//                    offsetCounts.add(charCount);
//                    charCount += line.length() + 1;
//                }
//            }
//            isFilled.add(proc, true);
//            hasContents = true;
//            GlobalStructure.getInstance().setOffset(proc, offsetCounts, filename);          
//        }
//        catch(Exception e)
//        {
//            new javax.swing.JOptionPane().
//                    showMessageDialog(this, "Err reading file for process : " + proc);            
//            isFilled.add(proc, false);
//        }
//        return fileContents;
//    }
    
    public void CleanUp()
    {
        documents.clear();
        viewPanes.clear();
        editorPanes.clear();
    }

//    public FileReader getFileReader(String filename, int proc) {        
//        File file;
//        FileReader fr;
//        boolean localdir = false;
//        fr = null;
//        file = null;
//        try {
//            try {
//                try {
//                    file = new File(filename);
//                    fr = new FileReader(file);
//                    localdir = false;
//                }
//                catch(FileNotFoundException e)
//                {
//                    localdir = true;
//                }
//                if(localdir) 
//                {   
//                    file = new File(GlobalStructure.getInstance().CurrentDirectory, filename);            
//                    fr = new FileReader(file);
//                }
//                    
//            }   
//            catch(FileNotFoundException e)
//            {
//                new javax.swing.JOptionPane().
//                    showMessageDialog(this, "Locate the file to load for process : " + proc);
//                
//                int returnval = jfilechooser.showOpenDialog(null);
//                jfilechooser.setCurrentDirectory(GlobalStructure.getInstance().CurrentDirectory);
//                if(returnval == JFileChooser.APPROVE_OPTION)
//                {
//                    file = null;
//                    file = jfilechooser.getSelectedFile();                
//                    fr = new FileReader(file);                    
//                }
//                else 
//                {
//                    isFilled.add(proc, false);
//                    fr = null;
//                }
////                readFile(fr, proc);
//            }                        
//        } catch (Exception e) {            
//            new javax.swing.JOptionPane().
//                    showMessageDialog(this, "File not available to read for process : " + proc);
////            filled = false;
//        }
//        return fr;
//    }

    public String getSelection(int proc, int start, int end) {
        viewPanes.get(proc).setSelectionStart(start);
        viewPanes.get(proc).setSelectionEnd(end);
        return viewPanes.get(proc).getSelectedText();
    }

    public Element getElementLine(int proc, int offset) {
        return Utilities.getParagraphElement(viewPanes.get(proc), offset);
    }

    public void identifySource(Transition transition) {
        int proc = transition.pID;
        int srcIndex = GlobalStructure.getInstance().isOpened(transition.filename);
        
        SourceFile srcFileRef;
        ProcessTextSource procWindow;
        
        if(srcIndex == -1)
            GlobalStructure.getInstance().addSourceFile(transition.filename);
        
        srcFileRef = GlobalStructure.getInstance().getOpenedFile(transition.filename);
        procWindow = GlobalStructure.getInstance().sourceFiles.get(proc);
        
        if(srcFileRef != null) {
            if(srcFileRef.fileContents.isEmpty())
                srcFileRef.readFile(proc);                        
            
            if(! procWindow.isPresent(transition.filename)) {
                procWindow.addFileName(transition.filename);
                procWindow.appendContents(srcFileRef.fileContents);
                viewPanes.get(proc).setText(procWindow.Contents);
                viewPanes.get(proc).repaint();
            }
        }
        
        
        
        // check if the file is open
//        int currInterleaving = GlobalStructure.getInstance().currInterleaving;
        
//        Interleavings interleaving = GlobalStructure.getInstance().
//                interleavings.get(currInterleaving-1);
//        
//        Process process = interleaving.procs.get(proc);
//        
//        SourceFile srcFile = process.returnOpenFile();
//        
//        if(! srcFile.fileName.equals(transition.filename)) {
//            process.setClose(srcFile.fileName);
//            srcFile = process.getFile(transition.filename);
//            if(srcFile != null)
//                updateSource(srcFile, proc);
//            else
//                System.err.print("Err : Loading New file");
//        }        
        
        Element elem = getElementLine(proc, transition.offset - 1);

        int start = elem.getStartOffset();
        int end = elem.getEndOffset();
        int tend = end;                             

        String selection = getSelection(proc, start, end);        

        while (!isMpiCall(selection, transition)) {
            start = start - 1;
            elem = getElementLine(proc, start);
            start = elem.getStartOffset();
            tend = elem.getEndOffset();
            selection = getSelection(proc, start, tend) + selection;
            if(start <= documents.get(proc).getStartPosition().getOffset())
            {                 
                 new javax.swing.JOptionPane().
                        showMessageDialog(this, "Source file might have changed since compilation ");                            
                killWindow();                
                UI.jRadioButton_hideSource.setSelected(true);
                //UI.jToggleButton1.setSelected(false);
                //UI.jToggleButton1.setText("Show Source");
               // CleanUp();
                break;
            }
        }

        int temp = start;
        start = start + selection.indexOf("MPI_");
        end = temp + selection.indexOf(");") + 2;

        highlightSource(proc, start, end);
        viewPanes.get(proc).setSelectionStart(0);
        viewPanes.get(proc).setSelectionEnd(0); 
    }

    public void cleanSrcView() {
        if (this != null) {
            //this.flushSourceDisplay();            
            this.CleanUp();
            this.killWindow();            
        }
    }

    public boolean isMpiCall(String text, Transition transition) {
        Stack<String> stack = new Stack<String>();

        for (int i = 0; i < text.length(); i++) {
            if (text.charAt(i) == '(') {
                stack.push("(");
            }
            if (!stack.empty()) {
                if (text.charAt(i) == ')') {
                    stack.pop();
                }
            }
        }

        if (stack.empty()) {
            String func = "MPI_" + transition.function;
            if (text.toLowerCase().contains(func.toLowerCase())) {
                return true;
            } else {
                return false;
            }
        } else {
            stack.clear();
            return false;
        }

    }

    public void initTextPanes() {
        int nP = GlobalStructure.getInstance().noProcess;
        documents = new ArrayList<HighlightedDocument>(nP);
        viewPanes = new ArrayList<JTextPane>(nP);
        editorPanes = new ArrayList<JScrollPane>(nP);  
        TabbedPane = new JTabbedPane();
        // set Grid layout        
        this.setLayout(new GridLayout(1, nP));

        try {
            for (int i = 0; i < nP; i++) {
                documents.add(new HighlightedDocument());                
                //documents[i] = new HighlightedDocument();
                documents.get(i).setHighlightStyle(HighlightedDocument.C_STYLE);
                viewPanes.add(new JTextPane(documents.get(i)));
                viewPanes.get(i).setBorder(BorderFactory.createLineBorder(Color.GRAY));                
                viewPanes.get(i).setBackground(Color.WHITE);
                viewPanes.get(i).setDragEnabled(true);
                viewPanes.get(i).setEditable(false);
                editorPanes.add(new JScrollPane(viewPanes.get(i)));                                 
            }
        } catch (Exception e) {
        }
    }

    public void highlightSource(int proc, int start, int end) {
        try {
            viewPanes.get(proc).getHighlighter().addHighlight(start, end, painters.get(proc));
            viewPanes.get(proc).setCaretPosition(start);
            viewPanes.get(proc).grabFocus();
            
        } catch (Exception e) {
            //System.out.println(e.getMessage());
        }
    }
    
    public void setUI(UI.ispMainUI UI)
    {
        this.UI = UI;
    }

    public void flushSourceDisplay() {
        int np = GlobalStructure.getInstance().noProcess;
        for (int i = 0; i < np; i++) {
            viewPanes.get(i).getHighlighter().removeAllHighlights();
        }
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        setPreferredSize(new java.awt.Dimension(1000, 700));

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(this);
        this.setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 1000, Short.MAX_VALUE)
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 700, Short.MAX_VALUE)
        );
    }// </editor-fold>//GEN-END:initComponents
    public JFrame window;
    ArrayList<JTextPane> viewPanes;
    ArrayList<JScrollPane> editorPanes;
    ArrayList<HighlightedDocument> documents;
    public JFileChooser jfilechooser;
    ArrayList<DefaultHighlighter.DefaultHighlightPainter> painters;
    DefaultHighlighter.DefaultHighlightPainter painter;
    boolean isActive;
    boolean hasContents;
    ArrayList<Boolean> isFilled; 
    JTabbedPane TabbedPane;
    JMenuBar menubar;
    JMenu viewmenu;
    JMenuItem viewTabbed;
    JMenuItem viewSplit;
    UI.ispMainUI UI;        // pointer
    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables
}
