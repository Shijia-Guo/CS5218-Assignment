#include <cstdio>
#include <iostream>
#include <set>
#include <map>
#include <stack>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

std::string getSimpleNodeLabel(const BasicBlock *Node);
std::string getSimpleVarName(const Instruction* Ins);
int main(int argc, char **argv)
{
    // Read the IR file.
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;

    // Extract Module M from IR (assuming only one Module exists)
    Module *M = ParseIRFile(argv[1], Err, Context);
    if (M == nullptr)
    {
      fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
      return EXIT_FAILURE;
    }
    
    std::map<std::string,std::set<std::string>> blockInitMap;
    // 1.Extract Function main from Module M
    Function *F = M->getFunction("main");
    for (auto &BB: *F){
        std::set<std::string> emptySet;
        std::string blockName = getSimpleNodeLabel(&BB);
    	//llvm::outs() << "Label:" << blockName << "\n";
        for(auto &I : BB){
           if(isa<StoreInst>(I)) { 
                Value* v = I.getOperand(1);
		std::string var = getSimpleVarName(dyn_cast<Instruction>(v));
                if(!var.empty())
                	emptySet.insert(var);
		//llvm::outs() << cast<StoreInst>(I).getOperand(1)->getName()<< "\n";
           }
	}
	blockInitMap[blockName] = emptySet;
    }
   
    // 2. Traversing the CFG in Depth First Order
    // In order to do so, we use a stack: traversalStack.
    // First, we add the entry basic block and a integer 0 representing depth to 
    // the stack. The depth represents the distance of the BB from entryBB.  
    std::stack<std::pair<BasicBlock*,int>> traversalStack;
    BasicBlock* entryBB = &F->getEntryBlock();
    std::pair<BasicBlock*,int > succAnalysisNode = std::make_pair(entryBB,0);
    traversalStack.push(succAnalysisNode);
    // 3. while the stack is not empty we pop the top Basic Block, print it and
    // add it's successor nodes to the stack plus an updated integer
    std::map<int,std::string> routes;
    while(!traversalStack.empty()){
        // Pop the top Basic Block and the depth from stack
        std::pair<BasicBlock*,int > succAnalysisNode = traversalStack.top();
        BasicBlock* BB = succAnalysisNode.first;
        int depth = succAnalysisNode.second;
      	traversalStack.pop();
      	
        routes[depth] = getSimpleNodeLabel(BB);

	
        // Extract the last instruction in the stack (Terminator Instruction)
        const TerminatorInst *TInst = BB->getTerminator();
        
        // Extract the number of successors the terminator instructor has
	int NSucc = TInst->getNumSuccessors();
        if(0 == NSucc){
          std::map<int,std::string> ::iterator iter;
          iter=routes.begin();
          // update the block init variable at the same time
          std::set<std::string> root;
          root = blockInitMap[iter->second];
          for(int i = 0; i<= depth; i++){
            if(i != 0){
             blockInitMap[iter->second].insert(root.begin(),root.end());
             root = blockInitMap[iter->second];
            }
            iter++;
          }
        }
        
	for (int i = 0;  i < NSucc; ++i) {
            // For all successor basic blocks, add them to the stack
	    // Increase the value of depth by 1
            BasicBlock *Succ = TInst->getSuccessor(i);    
            std::pair<BasicBlock*,int > succAnalysisNode = std::make_pair(Succ,depth+1);
            traversalStack.push(succAnalysisNode);
    	}	
    }
    
	std::map<std::string,std::set<std::string>>::iterator ite;
        for(ite = blockInitMap.begin(); ite != blockInitMap.end(); ite++) {
        std::cout << "Block name:"<< ite->first << std::endl;
        std::cout << "initialized varabile have: ";
           std::set<std::string>::iterator siter;
           for( siter = ite->second.begin();siter != ite->second.end(); siter++){
           std::cout << *siter << " ";
           }
           std::cout << std::endl;
        }
    return 0;
}

std::string getSimpleVarName(const Instruction* Ins){
if (!Ins->getName().empty())
        return Ins->getName().str();
    std::string Str;
    raw_string_ostream OS(Str);
    Ins->printAsOperand(OS, false);
    return OS.str();
}

// Printing Basic Block Label 
std::string getSimpleNodeLabel(const BasicBlock *Node) {
    if (!Node->getName().empty())
        return Node->getName().str();
    std::string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}
