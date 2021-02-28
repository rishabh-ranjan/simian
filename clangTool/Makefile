CXX = g++
CFLAGS = -fno-rtti -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0 -g

LLVM_BIN_PATH = ./clang+llvm-3.8.0-x86_64-linux-gnu-debian8/bin
LLVM_LIBS = ./clang+llvm-3.8.0-x86_64-linux-gnu-debian8/lib
LLVM_CONFIG_COMMAND = ./clang+llvm-3.8.0-x86_64-linux-gnu-debian8/bin/llvm-config --libs core --ldflags --system-libs
CLANG_BUILD_FLAGS = -I./clang+llvm-3.8.0-x86_64-linux-gnu-debian8/include \
					-I/usr/include/mpi \
					-L./clang+llvm-3.8.0-x86_64-linux-gnu-debian8/lib

CLANGLIBS := \
	-Wl,--start-group \
	-lclangAST \
	-lclangAnalysis \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangFrontendTool \
	-lclangLex \
	-lclangParse \
	-lclangSema \
	-lclangEdit \
	-lclangASTMatchers \
	-lclangRewrite \
	-lclangRewriteFrontend \
	-lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangSerialization \
	-lclangToolingCore \
	-lclangTooling \
	-lpthread \
	-Wl,--end-group

LLVMLIBS := \
	-Wl,--start-group \
	-lLLVMX86AsmParser \
	-lLLVMX86Desc \
	-lLLVMX86AsmPrinter \
	-lLLVMX86Info \
	-lLLVMX86Utils \
	-lLLVMipo \
	-lLLVMScalarOpts \
	-lLLVMInstCombine \
	-lLLVMTransformUtils \
	-lLLVMAnalysis \
	-lLLVMTarget \
	-lLLVMOption \
	-lLLVMMCParser \
	-lLLVMMC \
	-lLLVMObject \
	-lLLVMBitReader \
	-lLLVMCore \
	-lLLVMSupport \
	-Wl,--end-group

clangTool: clangTool.cpp
	    $(CXX) clangTool.cpp -o clangTool IfInfo.cpp $(CFLAGS) $(CLANG_BUILD_FLAGS) `$(LLVM_CONFIG_COMMAND)` $(CLANGLIBS) $(LLVMLIBS) -lz -lstdc++ -ltinfo

clean:
	rm -rf *.o *.ll clangTool
