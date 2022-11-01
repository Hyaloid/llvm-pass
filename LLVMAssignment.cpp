//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/Support/CommandLine.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#define EXCLUDE_FNAME "llvm.dbg.value"

using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang will
 * have optnone attribute which would lead to some transform passes disabled, like mem2reg.
 */
struct EnableFunctionOptPass: public FunctionPass {
    static char ID;
    EnableFunctionOptPass():FunctionPass(ID){}
    bool runOnFunction(Function & F) override{
        if(F.hasFnAttribute(Attribute::OptimizeNone))
        {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID=0;

	
///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  std::map<int, std::set<std::string>> lineno_func; // save lineno, function names

  FuncPtrPass() : ModulePass(ID) {}

  void handleModule(Module &M) {
    for(auto fi = M.begin(); fi != M.end(); ++fi) {
      handleFunc(*fi);
    }
  }

  void handleFunc(Function &func) {
    for(auto bbi = func.begin(); bbi != func.end(); ++bbi) {
      handleBasicBlock(*bbi);
    }
  }

  void handleBasicBlock(BasicBlock &bb) {
    for(auto insti = bb.begin(); insti != bb.end(); ++insti) {
      handleInst(*insti);
    }
  }

  void handleInst(Instruction &inst){
    if(CallInst* callinst = dyn_cast<CallInst>(&inst)){
      int lineno = inst.getDebugLoc().getLine();
      handleCallInst(lineno, callinst);
    } 
  }

  void handleCallInst(int lineno, CallInst* callinst) {
    Function *func = callinst->getCalledFunction();
    if(func) {
      std::string func_name = func->getName();
      // errs() << " lineno : "<< lineno << "func_name: ===" << func_name;
      if(func_name != EXCLUDE_FNAME) {
        lineno_func[lineno].insert(func_name);
      } else {
        // ...
      }
      // func->dump();
    } else {
      // not directly called
      Value* val = callinst->getCalledValue();
      if(PHINode* phi_node = dyn_cast<PHINode>(val)) {
        handlePHINode(lineno, phi_node);
      }
      // val->dump();
    }
  } 

  void handlePHINode(int lineno, PHINode* phi_node) {
    // auto opr = phi_node->incoming_values();
    for(Use* ui = phi_node->op_begin(); ui != phi_node->op_end(); ++ui) {
      if(Function* func = dyn_cast<Function>(*ui)) {
        std::string func_name = func->getName();
        lineno_func[lineno].insert(func_name);
      } else if(PHINode* ph_node = dyn_cast<PHINode>(*ui)){ // more than 1
        handlePHINode(lineno, ph_node);
      } else {
        errs() << "===handlePHINode else===";
      }
    }
    //...
  }

  void printLinenoFunc() {
    for(auto iter = lineno_func.begin(); iter != lineno_func.end(); ++iter) {
      printf("%d%s", iter->first, " : ");
      auto &func_set = iter->second;
      for(auto iter_func = func_set.begin(); iter_func != func_set.end(); ++iter_func) {
        if(iter_func != func_set.begin()) printf("%s", ", ");
        printf("%s", (*iter_func).c_str());
      }
      printf("%s", "\n");
    }
  }
  
  bool runOnModule(Module &M) override {
    handleModule(M);
    printLinenoFunc();
    return false;
  }
};


char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");

static cl::opt<std::string>
InputFilename(cl::Positional,
              cl::desc("<filename>.bc"),
              cl::init(""));


int main(int argc, char **argv) {
   LLVMContext &Context = getGlobalContext();
   SMDiagnostic Err;
   // Parse the command line to read the Inputfilename
   cl::ParseCommandLineOptions(argc, argv,
                              "FuncPtrPass \n My first LLVM too which does not do much.\n");


   // Load the input module
   std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
   if (!M) {
      Err.print(argv[0], errs());
      return 1;
   }

   llvm::legacy::PassManager Passes;
   	
   ///Remove functions' optnone attribute in LLVM5.0
   Passes.add(new EnableFunctionOptPass());
   ///Transform it to SSA
   Passes.add(llvm::createPromoteMemoryToRegisterPass());

   /// Your pass to print Function and Call Instructions
   Passes.add(new FuncPtrPass());
   Passes.run(*M.get());
}

