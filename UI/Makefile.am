JAVAC = @JAVAC@
AM_JAVACFLAGS = -source 1.5 -target 1.5 -classpath ".:lib/*"
JAVAROOT = .
bin_SCRIPTS = ispui.jar

if COND_COMPILE_JAVA

src_JAVA = \
	Data/GlobalStructure.java \
	Data/Transition.java \
	Data/Interleavings.java \
	Data/InterCBTemplate.java \
	Data/ISPMatchEdge.java \
	Data/ProcessTextSource.java \
	Data/SourceFile.java \
	Parser/Parser.java \
	Graph/ISPGraphModel.java \
	Graph/SourceView.java \
	Graph/MyGraphMouseListener.java \
	Graph/DrawGraph.java \
	Graph/drawIntraCB.java \
	Graph/drawInterCB.java \
	Graph/displayIntraCB.java \
	Graph/InternalIssueOrderSorter.java \
	UI/ispMainUI.java

ispui.jar: classsrc.stamp
	-rm -rf org com ispui.jar
	@JAR@ xf lib/log4j-1.2.15.jar org
	@JAR@ xf lib/syntax.jar com
	@JAR@ xf lib/jgraph.jar org
	@JAR@ cfm ispui.jar manifest.txt Data/*.class Graph/*.class UI/*.class Parser/*.class org com
	-rm -rf org com

install-srcJAVA:
uninstall-srcJAVA:

clean: clean-srcJAVA
	-rm -f *.jar Data/*.class Graph/*.class UI/*.class Parser/*.class

distclean: distclean-am
	-rm -f *.jar Data/*.class Graph/*.class UI/*.class Parser/*.class

endif
