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

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

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

class ExampleASTConsumer : public ASTConsumer 
{
  //private:
  //ExampleVisitor *visitor; // doesn't have to be private

  public:
  // override the constructor in order to pass CI
  explicit ExampleASTConsumer(CompilerInstance *CI)
  { }

  // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) 
  {
      /* we can use ASTContext to get the TranslationUnitDecl, which is
      a single Decl that collectively represents the entire source file */
      //visitor->TraverseDecl(Context.getTranslationUnitDecl());
  }
};

class Include_Matching_Action : public ASTFrontendAction
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
    ASTContext &astContext = ci.getASTContext();
    SourceManager &s = astContext.getSourceManager();

    Find_Includes *find_includes_callback = (Find_Includes*)pp.getPPCallbacks();

    // do whatever you want with the callback now
    if (find_includes_callback->has_include)
      errs() << "Found at least one include at: " << s.getSpellingLineNumber(find_includes_callback->atLoc) << "\n";
  }
};

int main(int argc, const char **argv) 
{
  // parse the command-line args passed to your code
  CommonOptionsParser op(argc, argv, MyToolCategory);        
  // create a new Clang Tool instance (a LibTooling environment)
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  // run the Clang Tool, creating a new FrontendAction (explained below)
  int result = Tool.run(newFrontendActionFactory<Include_Matching_Action>().get());
  return result;
}
