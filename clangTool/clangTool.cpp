/*
* clangTool.cpp
* Dhriti Khanna (dhritik@iiitd.ac.in)
*
* This code instruments an MPI program (file name passed through command line argument).
* The instrumented code calls a function storeInfo() (written in GenerateAssumes.h file) at appropriate places.
*
*/

#include <string.h>
#include <stdio.h>
#include <sstream>
#include <fstream>

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Basic/TokenKinds.h"    // for clang::tok
#include "clang/Lex/Lexer.h"

#include "IfInfo.h"
using namespace std;

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

Rewriter rewriter;
int numFunctions = 0;

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

string OperatorArray[] = {"bo_eq", "bo_ne", "bo_lt", "bo_gt", "bo_le", "bo_ge"};
string OppositeOperatorArray[] = {"bo_ne", "bo_eq", "bo_ge", "bo_le", "bo_gt", "bo_lt"};
IfInfo* ifData;
bool elseIfComing = false;
bool recvFound = false; // Whenever an MPI_Recv is found, this is set and there has to be instrumentation
std::vector<SourceLocation> startEndSrcLocs;

class Find_Includes : public PPCallbacks
{
	public:
	bool has_include;
	SourceLocation atLoc;

	void InclusionDirective(
		SourceLocation hash_loc,
		const Token &include_token,
		StringRef file_name,
		bool is_angled,
		CharSourceRange filename_range,
		const FileEntry *file,
		StringRef search_path,
		StringRef relative_path,
		const Module *imported)
	{
		if(!has_include)
		{
			has_include = true;
			atLoc = hash_loc;
		}
	}
};

class ExampleVisitor : public RecursiveASTVisitor<ExampleVisitor> 
{
	private:
	ASTContext *astContext; // used for getting additional AST info
	int ifStartPos;
	int ifEndPos;
	vector<int> callExpressionsToIgnore;
	map<int, string> variableNames; // Line number of Recv functions and the variable names in those recvs
	map<int, string> statusNames; // Line number of Recv functions and the status names in those recvs
	IfStmt* ifSGlobal;

	public:
	explicit ExampleVisitor(CompilerInstance *CI) : astContext(&(CI->getASTContext())) // initialize private members
	{
		rewriter.setSourceMgr(astContext->getSourceManager(), astContext->getLangOpts());
		ifStartPos = 0;
		ifEndPos = 0;
		ifSGlobal = 0;
	}

	/*virtual bool VisitFunctionDecl(FunctionDecl *func) 
	{
			numFunctions++;
			std::string funcName = func->getNameInfo().getName().getAsString(); funcName.c_str();
			if (strcmp(funcName.c_str(), "do_math") == 0) 
			{
					rewriter.ReplaceText(func->getLocation(), funcName.length(), "add5");
					errs() << "** Rewrote function def: " << funcName << "\n";
			} 
			return true;
	}*/

	// I was using this function to insert '#include "GenerateAssumes"' just after a UsingDirective declaration 
	// in the source. But now I am inserting this declaration just before the first IncludeDirective. And I am using
	// pre compilation phase to find the location of first include directive (see functions in ExampleFrontendAction class)
	// as include directives are not part of the AST. AST is built after the compilation phase is over.

	/*virtual bool VisitUsingDirectiveDecl(UsingDirectiveDecl* d)
	{
			SourceManager& s = astContext->getSourceManager();
			if(s.isInMainFile(d->getLocStart()))
			{
					SourceLocation srcLoc = d->getLocStart();
					//StringRef str = "#include \"GenerateAssumes.h\"\nextern \"C++\" void storeInfo(std::string, Operator, int);\n\n";
					StringRef str = "#include \"GenerateAssumes.h\"\n";
					rewriter.InsertTextAfter(srcLoc, str);
					//errs() << "\nFound a namespace declaration: " << d->getLocStart().printToString(s) << "\n";
			}
			return true;
	}*/

	virtual bool VisitCallExpr(CallExpr* call)
	{
		SourceManager &s = astContext->getSourceManager();
		string whichFunc;
		const Expr* callee = call->getCallee();
		if(const ImplicitCastExpr* Cast = dyn_cast<ImplicitCastExpr>(callee))
		{   
				callee = Cast->getSubExpr();
				if(const DeclRefExpr* decl = dyn_cast<DeclRefExpr>(callee))
				{
						const NamedDecl* n = decl->getFoundDecl();
						whichFunc = n->getNameAsString(); 
				}
		}

		if(strcmp(whichFunc.c_str(), "MPI_Init") == 0)
		{
			if(s.isInMainFile(call->getLocStart()))
			{
					//Insert code to create an object of ProcessIfs class.
					//SourceLocation srcLocation = call->getLocStart();
					//std::string toInsert = "ProcessIfs* ifs = new ProcessIfs();";
					//rewriter.InsertTextBefore(srcLocation, toInsert);

					SourceLocation endLocation = call->getLocEnd().getLocWithOffset(2);
					std::string toInsert = "int my_rank; MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);"; //"ifs->connectToScheduler(my_rank);\n";
					rewriter.InsertTextAfter(endLocation, toInsert);
			}
			return true;
		}

		if(strcmp(whichFunc.c_str(), "MPI_Finalize") == 0)
		{
			if(s.isInMainFile(call->getLocStart()))
			{
					//Insert a call to the function which makes connection with the server to transfer asserts.
					SourceLocation srcLocation = call->getLocEnd().getLocWithOffset(2);
					
					std::string toInsert = "ifs->transferInfoToScheduler(my_rank);";
					rewriter.InsertTextAfter(srcLocation, toInsert);
			}

			return true;
		}

		if (strcmp(whichFunc.c_str(), "MPI_Recv") != 0)
			return true;

		// An MPI_Recv function is found
		if(s.isInMainFile(call->getLocStart()))
		{
			//errs() << "Found a receive at: " << s.getSpellingLineNumber(call->getLocStart()) << "\n";
			int lineno = s.getSpellingLineNumber(call->getLocStart());
			string callLoc = call->getLocStart().printToString(s);
			stringstream ss(callLoc);
			string tok;
			getline(ss, tok, ':');
			getline(ss, tok, ':');

			int callLocSource = atoi(tok.c_str());

			//errs() << "\nifStart: " << ifStartPos;
			//errs() << "\nifEnd: " << ifEndPos;
			//errs() << "\ncallLocSource: " << callLocSource;
			//errs() << "\ncallExpressionsToIgnore: " << "\n";
			//for (int i=0; i<callExpressionsToIgnore.size(); i++)
			//{
				//errs() << callExpressionsToIgnore[i] << "\t";
			//}
			//errs() << "\n";


			if(std::find(callExpressionsToIgnore.begin(), callExpressionsToIgnore.end(), callLocSource) != callExpressionsToIgnore.end())
				return true;

			LangOptions LangOpts;
			LangOpts.CPlusPlus = true;
			PrintingPolicy Policy(LangOpts);

			/*std::string TypeS;           
			raw_string_ostream raw(TypeS);
			call->printPretty(raw, 0, Policy);
			errs() << "Function: " << raw.str() << "\n";*/

			string variableName;
			string statusName;
			int flag = 0;
			for(int i=0, j=call->getNumArgs(); i<j; i++)
			{
				if(i==0)
				{
					std::string TypeS;
					raw_string_ostream raw(TypeS);
					call->getArg(i)->printPretty(raw, 0, Policy);
					variableName = raw.str();
					if(variableName.at(0) == '&')
					{
						variableName = variableName.substr(1, variableName.length()-1);
					}
				}
				if(i==3) // If third argument is MPI_ANY_SOURCE, store the variable name in a vector
				{
					std::string TypeS;
					raw_string_ostream raw(TypeS);
					call->getArg(i)->printPretty(raw, 0, Policy);
					const char* src = raw.str().c_str();
					//llvm::errs() << "\nPut a variableName: " << src;
					//if(strcmp(raw.str().c_str(), "-1") == 0) // -1 = MPI_ANY_SOURCE
					
					if(src[1] == '-') // MPI_ANY_SOURCE (nagative number; it most probably is (-2))
					{
						// Problem with insert function is that it does not replace the previous item in a vector
						// So, if I find two Recv functions with the variable of the same name, 
						// then the map will contain the line number pertaining to the first one always.
						//variableNames.insert(std::pair<int, string>(lineno, variableName));

						// But we have to make sure that we do not replace the line no with the line no of the receive which is inside the 'if' statement.
						variableNames[lineno] = variableName;
					}
				}
				if(i==4)
				{
					std::string TypeS;
					raw_string_ostream raw(TypeS);
					call->getArg(i)->printPretty(raw, 0, Policy);
					const char* src = raw.str().c_str();
					//if(strcmp(raw.str().c_str(), "-1") == 0) // -1 = MPI_ANY_TAG
					if(src[1] == '-') // MPI_ANY_TAG
					{
						// Store the name of the status pointer in statusNames vector
						flag = 1;
					}
				}
				if(i==6 && flag==1)
				{
					std::string TypeS;
					raw_string_ostream raw(TypeS);
					call->getArg(i)->printPretty(raw, 0, Policy);
					statusName = raw.str();
					if(statusName.at(0) == '&')
					{
						statusName = statusName.substr(1, statusName.length()-1);
					}
					// Store the name of the status pointer in statusNames vector
					//statusNames.insert(std::pair<int, string>(lineno, statusName));

					// But we have to make sure that we do not replace the line no with the line no of the receive which is inside the 'if' statement.
					statusNames[lineno] = statusName;

					/*bool canNotReplace = false;
					std::vector<std::pair<int, int> >::iterator it;
					for(it = ifData->startEndPositions.begin(); it!=ifData->startEndPositions.end(); ++it)
					{
						errs() << "first: "  << (*it).first << " second: " << (*it).second << "\n";
						errs() << lineno << "\n";

						if(lineno>=(*it).first)
						{
							canNotReplace = true; 
							break; 
						}
					}
					if(!canNotReplace)	
						statusNames[lineno] = statusName;
					else*/
				}
			}
		}
		return true;
	}


	bool ifMPIFunctionPresent(IfStmt* ifS, bool isIf)
	{
		Stmt::child_iterator I, E;
		if(isIf == true)
		{
			I = ifS->getThen()->child_begin();
			E = ifS->getThen()->child_end();
		}
		else
		{
			I = ifS->getElse()->child_begin();
			E = ifS->getElse()->child_end();
		}
		SourceManager &s = astContext->getSourceManager();      
		//llvm::errs() << "\nStart of an If: " << ifS->getLocStart().printToString(s) << "\n";
		for (; I != E; I++) 
		{
			if (Stmt* Child = *I) 
			{
				//errs() << "\nIf's Body: " << Child->getLocStart().printToString(s) << "\n";
				if(const CallExpr* call = dyn_cast<CallExpr>(Child))
				{
					const Expr* callee = call->getCallee();
					if (const ImplicitCastExpr* Cast = dyn_cast<ImplicitCastExpr>(callee))
					{
						callee = Cast->getSubExpr();
						if (const DeclRefExpr* decl = dyn_cast<DeclRefExpr>(callee))
						{
							const NamedDecl* n = decl->getFoundDecl();
							string funcName = n->getNameAsString();
							if(strncmp(funcName.c_str(), "MPI_", strlen("MPI_")) == 0)
								return true;
						}
					}
				}
				if (IfStmt* innerIfS = dyn_cast<IfStmt>(Child))
				{
					bool res = ifMPIFunctionPresent(innerIfS, true);
					if(res)
						return res;
				}
			}
		}
		return false;
	}

	// This just tells whether a value is present in the map or not
	template<typename K, typename V>
	bool findByValue(std::map<K, V> mapOfElemen, V value)
	{
		bool bResult = false;
		auto it = mapOfElemen.begin();
		// Iterate through the map
		while(it != mapOfElemen.end())
		{
			// Check if value of this entry matches with given value
			if(it->second == value)
			{
				// Yes found
				bResult = true;
				break;
			}
			// Go to next entry in map
			it++;
		}
		return bResult;
	}

	// This returns the keys corresponding to a value from a map
	template<typename K, typename V>
	bool findByValue(std::vector<K> & vec, std::map<K, V> mapOfElemen, V value)
	{
		bool bResult = false;
		auto it = mapOfElemen.begin();
		// Iterate through the map
		while(it != mapOfElemen.end())
		{
			// Check if value of this entry matches with given value
			if(it->second == value)
			{
				// Yes found
				bResult = true;
				// Push the key in given map
				vec.push_back(it->first);
			}
			// Go to next entry in map
			it++;
		}
		return bResult;
	}

	// Find all those Recv calls which do not lie between if-else boundaries. From those, select the one which is nearest to the boundary
	int findNearestRecv(int & maxL, IfInfo* ifData, std::vector<int> vec)
	{
		int l;
		maxL = vec[0];
		std::vector<std::pair<int, int> > startEndPositions = ifData->startEndPositions;
		for(int i=0; i<vec.size(); ++i)
		{
			l = vec[i];
			bool flag = true;
			std::pair<int, int> p;
			std::vector<std::pair<int, int> >::iterator it = startEndPositions.begin();
			while(it != startEndPositions.end())
			{
				p = *it;
				if(l>p.first && l<=p.second)
				{
					flag = false;
					break;
				}
				else if(l>p.second)
				{
					flag = false;
					break;
				}
				it++;
			}
			if(flag)
				if(l>maxL) // The call must be above the if-else boundary
				{
					maxL = l;
				}
		}
		return 0;
	}

	virtual bool VisitStmt(Stmt *st) 
	{
		/*
		string tmpstr;
		raw_string_ostream rso(tmpstr);
		st->printPretty(rso, nullptr, PrintingPolicy(LangOptions()));
		rso.flush();
		clog << tmpstr << endl;
		*/
		if(elseIfComing == false)
		{
			ifData = new IfInfo();
			startEndSrcLocs.clear();
		}

		SourceManager &s = astContext->getSourceManager();      

		if (IfStmt* ifS = dyn_cast<IfStmt>(st))
		{       
			bool recvFound = false;   
			ifSGlobal = dyn_cast<IfStmt>(st);
			if(s.isInMainFile(ifS->getLocStart()))
			{
				//llvm::errs() << "Found an If statement at: " << s.getSpellingLineNumber(ifS->getLocStart());
				if (const BinaryOperator *binOp = dyn_cast<BinaryOperator>(ifS->getCond()))
				{
					if (binOp->getOpcode() == BO_EQ || 
							binOp->getOpcode() == BO_NE ||
							binOp->getOpcode() == BO_LT ||
							binOp->getOpcode() == BO_GT ||
							binOp->getOpcode() == BO_LE ||
							binOp->getOpcode() == BO_GE)
					{
						if (binOp->getOpcode() == BO_EQ) ifData->ops.push_back(bo_eq); 
						else if (binOp->getOpcode() == BO_NE) ifData->ops.push_back(bo_ne); 
						else if (binOp->getOpcode() == BO_LT) ifData->ops.push_back(bo_lt);
						else if (binOp->getOpcode() == BO_GT) ifData->ops.push_back(bo_gt);
						else if (binOp->getOpcode() == BO_LE) ifData->ops.push_back(bo_le);
						else if (binOp->getOpcode() == BO_GE) ifData->ops.push_back(bo_ge);

						// Extracting LHS
						const Expr* LHS = binOp->getLHS();
						if (const ImplicitCastExpr* Cast = dyn_cast<ImplicitCastExpr>(LHS)) 
						{
							std::string structMember = "";
							LHS = Cast->getSubExpr();
							if (const MemberExpr* mem = dyn_cast<MemberExpr>(LHS))
							{
								structMember = mem->getMemberNameInfo().getName().getAsString();
								LHS = mem->getBase(); // For status.MPI_TAG. 'mem' will contain .MPI_TAG
							}
							if (const DeclRefExpr* decl = dyn_cast<DeclRefExpr>(LHS))
							{
								if(s.isInMainFile(decl->getLocStart()))
								{ 
									string variableName = decl->getNameInfo().getName().getAsString();
									if( !findByValue(variableNames, variableName) && !findByValue(statusNames, variableName) )
									{
										return true; // If there exists no variable in seen receives so far
									}
									else
									{
										if(structMember.compare("MPI_TAG")==0)
											//ifData->varName = "tag"; // This signifies that the tag is being compared in the 'if' condition
											ifData->varNames.push_back(variableName+"."+structMember);
										else
											ifData->varNames.push_back(variableName);

										// Find if the variable occurs in variableNames or statusNames
										std::vector<int> vec;
										int l;
										if(findByValue(vec, variableNames, variableName))
										{
											//Chose the appropriate Recv call and use it's line number
											findNearestRecv(l, ifData, vec);
											ifData->correspondingRecvLineno = l;
										}
										else
										{
											findByValue(vec, statusNames, variableName);
											findNearestRecv(l, ifData, vec);
											ifData->correspondingRecvLineno = l;
										}
									}
								}
								else return true;
							}
							else return true;
						}

						// Extracting RHS (TODO: cast this also into a variable)
						const Expr* RHS = binOp->getRHS();
						llvm::APSInt res;
						bool isFine = RHS->isIntegerConstantExpr(res, *astContext);
						if(isFine)
						{
								//res.print(errs(), true);
								ifData->literals.push_back((int)res.getExtValue());
						}

						/*if (const IntegerLiteral* integerLiteral = dyn_cast<IntegerLiteral>(RHS))
						{
								if(s.isInMainFile(integerLiteral->getLocStart()))
								{
									ifData->literals.push_back((int)integerLiteral->getValue().signedRoundToDouble());
								}
						}*/
					}
				}

				// Storing the start and end positions of 'if'
				SourceLocation srcLoc;
				for (Stmt::child_iterator I = ifS->getThen()->child_begin(), E = ifS->getThen()->child_end(); I != E; I++) 
				{
					Stmt* st = *I;
					srcLoc = st->getLocStart();
				}

				int strt = s.getSpellingLineNumber(ifS->getLocStart());
				int end = s.getSpellingLineNumber(srcLoc);
				ifData->startEndPositions.push_back(std::pair<int, int>(strt, end));
				ifData->sendsComingFromIfs.push_back(-1);
				startEndSrcLocs.push_back(srcLoc);

				// Check if an MPI function occurs in 'if' body
				for (Stmt::child_iterator I = ifS->getThen()->child_begin(), E = ifS->getThen()->child_end(); I != E; I++) 
				{
					if (Stmt* Child = *I) 
					{
						if(const CallExpr* call = dyn_cast<CallExpr>(Child))
						{
							const Expr* callee = call->getCallee();
							if (const ImplicitCastExpr* Cast = dyn_cast<ImplicitCastExpr>(callee))
							{
								callee = Cast->getSubExpr();
								if (const DeclRefExpr* decl = dyn_cast<DeclRefExpr>(callee))
								{
									const NamedDecl* n = decl->getFoundDecl();
									string funcName = n->getNameAsString();
									if(strncmp(funcName.c_str(), "MPI_", strlen("MPI_")) == 0)
									{
										// If the MPI function found is a Send, then record this information
										if(strncmp(funcName.c_str(), "MPI_Send", strlen("MPI_Send")) == 0)
										{
											SourceLocation srcLocation = call->getLocStart();
											int loc = s.getSpellingLineNumber(srcLocation);
											//llvm::errs() << "\ndk " << loc << "\n";
											(ifData->sendsComingFromIfs).push_back(loc);
											//rewriter.InsertTextAfter(srcLocation, toInsert);
										}
										
										// Instrument in the call to encoding function
										recvFound = true;
										break;
									}
								}
							}

							// We do not want to visit this call expression again, so put this in black list
							// if(s.isInMainFile(Child->getLocStart()))
							// {
							// 		string callLoc = call->getLocStart().printToString(s);
							// 		string tok;
							// 		stringstream ss(callLoc);
							// 		getline(ss, tok, ':');
							// 		getline(ss, tok, ':');
							// 		callExpressionsToIgnore.push_back(atoi(tok.c_str()));
							// }
						}
					}
				}

				//recvFound = ifMPIFunctionPresent(ifS, true);
				//errs() << "\nrecvfound is : " << recvFound << "\n";

				// For the 'else' part
				if(ifS->getElse() != NULL) // there is an else or an else-if
				{
					Stmt* ss = ifS->getElse(); // In cae of an elseif, ss starts with an 'if'. Otherwise it starts with '{'.
					
					LangOptions LangOpts;
					LangOpts.CPlusPlus = true;
					PrintingPolicy Policy(LangOpts);
					std::string TypeS;
					raw_string_ostream raw(TypeS);
					ss->printPretty(raw, 0, Policy);
					std::string elsePart = raw.str();
					std::string _if = "if";
					std::string theSubstring = elsePart.substr(0, 2);
					if(theSubstring.compare("if") == 0) // this is an else if
					{
						 elseIfComing = true;
					}
					else // this is an else
					{
						// Storing the start and end positions of 'else'
						SourceLocation srcLoc;
						for (Stmt::child_iterator I = ifS->getElse()->child_begin(), E = ifS->getElse()->child_end(); I != E; I++) 
						{
							Stmt* st = *I;
							srcLoc = st->getLocStart();
						}

						int strt = s.getSpellingLineNumber(ifS->getElse()->getLocStart());
						int end = s.getSpellingLineNumber(srcLoc);

						ifData->startEndPositions.push_back(std::pair<int, int>(strt, end));
						ifData->sendsComingFromIfs.push_back(-1);
						startEndSrcLocs.push_back(srcLoc);
						
						/*if(!recvFound)
						{
							// Check if a MPI_Recv occurs in 'else' body
							recvFound = ifMPIFunctionPresent(ifS, false);							
						}
						errs() << "\nInside else: " << recvFound << "\n";*/
						bool found = false;
						for (Stmt::child_iterator I = ifS->getElse()->child_begin(), E = ifS->getElse()->child_end(); I != E; I++) 
						{
							if (Stmt* Child = *I) 
							{
								if(const CallExpr* call = dyn_cast<CallExpr>(Child))
								{
									const Expr* callee = call->getCallee();
									if (const ImplicitCastExpr* Cast = dyn_cast<ImplicitCastExpr>(callee))
									{
										callee = Cast->getSubExpr();
										if (const DeclRefExpr* decl = dyn_cast<DeclRefExpr>(callee))
										{
											const NamedDecl* n = decl->getFoundDecl();
											string funcName = n->getNameAsString();
											if(strncmp(funcName.c_str(), "MPI_", strlen("MPI_")) == 0)
											{
												// If the MPI function found is a Send, then record this information
												if(strncmp(funcName.c_str(), "MPI_Send", strlen("MPI_Send")) == 0)
												{
													SourceLocation srcLocation = call->getLocStart();
													int loc = s.getSpellingLineNumber(srcLocation);
													//llvm::errs() << "\ndk " << loc << "\n";
													(ifData->sendsComingFromIfs).push_back(loc);
													found = true;
													//rewriter.InsertTextAfter(srcLocation, toInsert);
												}

												// Instrument in the call to encoding functin
												recvFound = true;
												break;
											}
										}
									}
								}
							}
						}

						if(!recvFound)
							(ifData->sendsComingFromIfs).push_back(-1);

						// Store the info
						if(recvFound == true)
							storeInfo();
						elseIfComing = false;
					}
				}
				else // There is no else part
				{
					// Store the info
					if(recvFound == true)
						storeInfo();
					elseIfComing = false;
					recvFound = false;
				} 
			}  
		}
		return true;
	}

	void storeInfo();
};

void ExampleVisitor::storeInfo()
{
	std::string toInsert;
	unsigned noIfs;
	
	//errs() << "\nThere there: " << ifData->correspondingRecvLineno << "\n";

	if( (ifData->startEndPositions).size() > (ifData->varNames).size() )
		noIfs = startEndSrcLocs.size()-1;
	else
		noIfs = startEndSrcLocs.size();

	for(unsigned i=0; i<noIfs; ++i)
	{
		for(unsigned j=0; j<ifData->varNames.size(); ++j)
		{
			if(i == j)
			{
				//int sz = (ifData->startEndPositions).size()>(ifData->varNames).size() ? (ifData->startEndPositions).size() : (ifData->varNames).size();
				toInsert = "";
				toInsert = "ifs->storeInfo("; // + std::to_string(sz) + ",";		


				toInsert += "\"" + ifData->varNames[j] + 
							"\"," + OperatorArray[ifData->ops[j]] + 
							"," + std::to_string(ifData->literals[j]) + 
							"," + std::to_string(ifData->startEndPositions[j].first+2) + 
							"," + std::to_string(ifData->startEndPositions[j].second+2) + ",";
				/*else
				toInsert += "\"" + ifData->varNames[j] + 
							"\"," + OppositeOperatorArray[ifData->ops[j]] + 
							"," + std::to_string(ifData->literals[j]) + 
							"," + std::to_string(ifData->startEndPositions[j].first+2) + 
							"," + std::to_string(ifData->startEndPositions[j].second+2) + ",";*/

				toInsert += std::to_string(ifData->correspondingRecvLineno+2) + ",";
				toInsert += std::to_string(ifData->sendsComingFromIfs[i]==-1 ? -1 : ifData->sendsComingFromIfs[i]+2) + "); ";
				rewriter.InsertTextAfter(startEndSrcLocs[i], toInsert);
			}
		}
	}

	if( (ifData->startEndPositions).size() > (ifData->varNames).size() ) // There is an else
	{
		//For else part
		//for(unsigned i=0; i<ifData->varNames.size(); ++i)
		//{
		// if(i != j)
		// {
				int sizeVarNames = (ifData->varNames).size();
				int sizeStartEndPositions = (ifData->startEndPositions).size();
				int sizeSendsComingFromIfs = (ifData->sendsComingFromIfs).size();
				//llvm::errs() << "\nsizeStartEndPositions: " << sizeStartEndPositions << "\n";
				toInsert = "ifs->storeInfo(";
				toInsert += "\"" + ifData->varNames[sizeVarNames-1] + 
							"\"," + OperatorArray[ifData->ops[sizeVarNames-1]] + 
							"," + ifData->varNames[sizeVarNames-1] + 
							"," + std::to_string(ifData->startEndPositions[sizeStartEndPositions-1].first+2) + 
							"," + std::to_string(ifData->startEndPositions[sizeStartEndPositions-1].second+2) + 
							"," + std::to_string(ifData->correspondingRecvLineno+2) +
							"," + std::to_string(ifData->sendsComingFromIfs[sizeSendsComingFromIfs-1]==-1 ? -1 : ifData->sendsComingFromIfs[sizeSendsComingFromIfs-1]+2) + "); ";
				
				rewriter.InsertTextAfter(startEndSrcLocs.back(), toInsert);
			//}
		//}
	}
	else // There is no else, so we put explicitely (Refer Monte for use case)
	{
		toInsert = "";
		//for(unsigned i=0; i<ifData->varNames.size(); ++i)
		//{
			//SourceLocation next_loc( clang::Lexer::findLocationAfterToken(startEndSrcLocs.back().getLocEnd(), tok::r_brace, s, astContext->getLangOpts(), false) );
			//std::string str = next_loc.printToString(s);
			//std::string str = (ifSGlobal->getLocEnd()).printToString(s);
			//errs() << "\n" << str << "\n";

			int sizeVarNames = (ifData->varNames).size();
			int sizeStartEndPositions = (ifData->startEndPositions).size();
			
			toInsert = "ifs->storeInfo(";
			toInsert += "\"" + ifData->varNames[sizeVarNames-1] + 
						"\"," + OperatorArray[ifData->ops[sizeVarNames-1]] + 
						"," + ifData->varNames[sizeVarNames-1] + 
						"," + std::to_string(ifData->startEndPositions[sizeStartEndPositions-1].second+3) + 
						"," + std::to_string(ifData->startEndPositions[sizeStartEndPositions-1].second+3) + 
						"," + std::to_string(ifData->correspondingRecvLineno+2) + 
						"," + std::to_string(-1) + "); ";
		//}
		std::string toBeInserted = "} else { " + toInsert + " }";
		rewriter.ReplaceText(ifSGlobal->getLocEnd(), 1, toBeInserted);
	}
}

class ExampleASTConsumer : public ASTConsumer 
{
	private:
	ExampleVisitor *visitor; // doesn't have to be private

	public:
	// override the constructor in order to pass CI
	explicit ExampleASTConsumer(CompilerInstance *CI) : visitor(new ExampleVisitor(CI)) // initialize the visitor
	{}

	// override this to call our ExampleVisitor on the entire source file
	virtual void HandleTranslationUnit(ASTContext &Context) 
	{
			/* we can use ASTContext to get the TranslationUnitDecl, which is
			a single Decl that collectively represents the entire source file */
			visitor->TraverseDecl(Context.getTranslationUnitDecl());
	}

	/*
	// override this to call our ExampleVisitor on each top-level Decl
	virtual bool HandleTopLevelDecl(DeclGroupRef DG) 
	{
			// a DeclGroupRef may have multiple Decls, so we iterate through each one
			for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; i++) 
			{
					Decl *D = *i;    
					visitor->TraverseDecl(D); // recursively visit each AST node in Decl "D"
			}
			return true;
	}
	*/
};

class ExampleFrontendAction : public ASTFrontendAction 
{	
	public:

	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) 
	{    
		//return new ExampleASTConsumer(&CI); // pass CI pointer to ASTConsumer
		return std::unique_ptr<clang::ASTConsumer>(new ExampleASTConsumer(&CI));
	}

	bool BeginSourceFileAction(CompilerInstance &CI, StringRef file)
	{
		Preprocessor &pp = CI.getPreprocessor();
		std::unique_ptr<Find_Includes> find_includes_callback(new Find_Includes());
		pp.addPPCallbacks(std::move(find_includes_callback));

		return true;
	}

	void EndSourceFileAction()
	{
		CompilerInstance &ci = getCompilerInstance();
		Preprocessor &pp = ci.getPreprocessor();
		//ASTContext &astContext = ci.getASTContext();
		//SourceManager &s = astContext.getSourceManager();

		Find_Includes *find_includes_callback = (Find_Includes*)pp.getPPCallbacks();

		// do whatever you want with the callback now
		if (find_includes_callback->has_include)
		{
			StringRef str = "#include \"GenerateAssumes.h\"\nProcessIfs* ifs = new ProcessIfs();\n";
			rewriter.InsertTextBefore(find_includes_callback->atLoc, str);

			//int firstIncludeAt = s.getSpellingLineNumber(find_includes_callback->atLoc);
		}
	}
};

int main(int argc, const char **argv) 
{
	// parse the command-line args passed to your code
	CommonOptionsParser op(argc, argv, MyToolCategory);        
	// create a new Clang Tool instance (a LibTooling environment)
	ClangTool Tool(op.getCompilations(), op.getSourcePathList());

	// run the Clang Tool, creating a new FrontendAction (explained below)
	if(argc != 2)
	{
		std::cout << "\nNot enough arguments! Specify a filename to instrument.\n";
		exit(0);
	}
	int result = Tool.run(newFrontendActionFactory<ExampleFrontendAction>().get());

	//errs() << "\nFound " << numFunctions << " functions.\n\n";
	// print out the rewritten source code ("rewriter" is a global var.)
	
	//rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(errs());
	
	string strArgv(argv[1]);
    vector <string> tokens;
    stringstream check1(strArgv); 
    string intermediate;
     
    // Tokenizing w.r.t. space '/'
    while(getline(check1, intermediate, '/'))
    {
        tokens.push_back(intermediate);
    }

	char outputFile[100];
	strcpy(outputFile, "./");
	int i;
    for(i = 0; i < tokens.size()-1; i++)
    {
    	strcat(outputFile, tokens[i].c_str());
    	strcat(outputFile, "/");
    }

	strcat(outputFile, "i_");
	strcat(outputFile, tokens[i].c_str());
	FILE* outFile = fopen(outputFile, "w");
	int fd = fileno(outFile);
	raw_fd_ostream newFile(fd, false);
	rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(newFile);
	return result;
}
