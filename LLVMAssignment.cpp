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
  bool in_arg_call = false;
  Function* tmp_func;

  FuncPtrPass() : ModulePass(ID) {}

  void handleModule(Module &M) {
    for(Module::iterator fi = M.begin(); fi != M.end(); ++fi) {
      handleFunc(*fi);
    }
  }

  void handleFunc(Function &func, int lineno = 0) {
    for(Function::iterator bbi = func.begin(); bbi != func.end(); ++bbi) {
      handleBasicBlock(*bbi, lineno);
    }
  }

  void handleBasicBlock(BasicBlock &bb, int lineno = 0) {
    for(BasicBlock::iterator insti = bb.begin(); insti != bb.end(); ++insti) {
      handleInst(*insti, lineno);
    }
  }

  void handleInst(Instruction &inst, int lineno = 0){
    if(lineno) {
      if(ReturnInst* retinst = dyn_cast<ReturnInst>(&inst)) {
        Value* val = retinst->getReturnValue();
        handleValue(val, lineno);
      } else {
        if(isa<CallInst>(&inst)){
          CallInst* callinst = dyn_cast<CallInst>(&inst);
          if(Function* func = callinst->getCalledFunction()) {
            handleFunc(*func, lineno);
          } else {
            handleCallInst(callinst, lineno);
          }
        } else {
          // ...
        }
      }
    } else {
      if(CallInst* callinst = dyn_cast<CallInst>(&inst)){
        int lineno = callinst->getDebugLoc().getLine();
        if(lineno == 0) return;
        handleCallInst(callinst, lineno);
      } else {
        // ...
      }
    }
  }

  void handleCallInst(CallInst* callinst, int lineno) {
    if(Function* func = callinst->getCalledFunction()) {
      if(func->isIntrinsic()) return;
      std::string func_name = func->getName();
      lineno_func[lineno].insert(func_name);
    } else {
      Value* val = callinst->getCalledOperand();
      if(CallInst* ncallinst = dyn_cast<CallInst>(val)) { // CallInst(CallInst)
        handleNestCallInst(ncallinst, lineno);
      } else {
        handleValue(val, lineno);
      }
      // val->dump();
    }
  }

  void handleNestCallInst(CallInst* callinst, int lineno) {
    if(Function* callfunc = callinst->getCalledFunction()) {
      handleFunc(*callfunc, lineno);
    } else {
      Value* val = callinst->getCalledOperand();
      if(PHINode* phi_node = dyn_cast<PHINode>(val)) {
        for(Use* ui = phi_node->op_begin(); ui != phi_node->op_end(); ++ui) {
          if(Function* func = dyn_cast<Function>(ui)) {
            handleFunc(*func, lineno);
          } else {
            // val->dump();
          }
        }
      }
    }
  }

  void handleValue(Value* val, int lineno) {
    if(isa<PHINode>(val)) {
      PHINode* phi_node = dyn_cast<PHINode>(val);
      handlePHINode(phi_node, lineno);
    } else if(isa<Argument>(val)) {
      Argument* args = dyn_cast<Argument>(val);
      // called as args
      handleArgumentCall(args, lineno);
    } else if(isa<CallInst>(val)){
      CallInst* callinst = dyn_cast<CallInst>(val);
      if(Function* func = callinst->getCalledFunction()) {
        handleFunc(*func, lineno);
      } else {
        tmp_callinst = callinst;
        handleCallInst(callinst, lineno);
      }
    } else if(isa<Function>(val)){
      Function* func = dyn_cast<Function>(val);
      if(func->isIntrinsic()) return;
      if(!in_arg_call) {
        std::string func_name = func->getName();
        in_arg_call = true;
        lineno_func[lineno].insert(func_name);
      } else {
        handleFunc(*func, lineno);
      }
    } else {
      // ...
    }
  }

  void handlePHINode(PHINode* phi_node, int lineno) {
    for(Use* ui = phi_node->op_begin(); ui != phi_node->op_end(); ++ui) {
      if(Function* func = dyn_cast<Function>(*ui)) {
        if(func->isIntrinsic()) return;
        std::string func_name = func->getName();
        lineno_func[lineno].insert(func_name);
      } else if(PHINode* ph_node = dyn_cast<PHINode>(*ui)){ // more than 1
        handlePHINode(ph_node, lineno);
      } else if(Argument* args = dyn_cast<Argument>(*ui)){
        handleArgumentCall(args, lineno);
      } else {
        // ...
      }
    }
  }

  void handleArgumentCall(Argument* args, int lineno) {
    int arg_pos = args->getArgNo(); // position
    Function* parentFunc = args->getParent();
    if(parentFunc->user_empty()) {
      if(tmp_callinst != nullptr) {
        in_arg_call = false;
        handleValue(tmp_callinst->getOperandList()[arg_pos], lineno);
        return;
      }
      // handleValue(tmp_callinst->getOperandList()[arg_pos], lineno);
    }
    int parent_arg_num = parentFunc->arg_size();
    for(Value::user_iterator ui = parentFunc->user_begin(); ui != parentFunc->user_end(); ++ui) {
      User* user = dyn_cast<User>(*ui);
      if(isa<PHINode>(user)) {  // inner func phi
        PHINode* phi = dyn_cast<PHINode>(user);
        for(Value::user_iterator pu = phi->user_begin(); pu != phi->user_end(); ++pu) {
          User* pui = dyn_cast<User>(*pu);
          Value* val = pui->getOperandList()[arg_pos];
          val->dump();
          handleValue(val, lineno);
        }
      } else if(isa<CallInst>(user)) {
        CallInst* callinst = dyn_cast<CallInst>(user);
        Function* func = callinst->getCalledFunction();
        // TODO: 解决函数参数嵌套调用

        Value* val = callinst->getOperandList()[arg_pos];
        handleValue(val, lineno);
        val->dump();
      } else {
        // ...
      }
    } 
  }

  void printLinenoFunc() {
    for(std::map<int, std::set<std::string>>::iterator iter = lineno_func.begin(); iter != lineno_func.end(); ++iter) {
      printf("%d%s", iter->first, " : ");
      std::set<std::string> func_set = iter->second;
      for(std::set<std::string>::iterator iter_func = func_set.begin(); iter_func != func_set.end(); ++iter_func) {
        if(iter_func != func_set.begin()) printf("%s", ", ");
        printf("%s", (*iter_func).c_str());
      }
      printf("%s", "\n");
    }
  }
  
  bool runOnModule(Module &M) override {
    // M.dump();
    // errs() << "-------------\n";
    handleModule(M);
    // errs() << "-------------\n";
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

