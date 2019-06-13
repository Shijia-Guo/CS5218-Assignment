#include <cstdio>
#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <set>
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
struct AffectItem
{
    std::string key;
    std::set<std::string> vals;
};

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
    
    std::map<std::string,std::vector<AffectItem>> blockAffectMap;
    std::set<std::string> allocate_vars = std::set<std::string>();
    std::map<std::string,std::set<std::string>> resultMap;
    std::map<std::string,std::set<std::string>> previousBlockMap;
    std::list<std::string> blockOrder;
    std::map<std::string,int> blockStatus;
    // 1.Extract Function main from Module M
    Function *F = M->getFunction("main");
    std::set<std::string> last_load_var_name_set = std::set<std::string>();
    for (auto &BB: *F){
        std::vector<AffectItem> affect_values;
        std::string blockName = getSimpleNodeLabel(&BB);
        //init the result and previous block relation 
        resultMap[blockName] = std::set<std::string>();
        previousBlockMap[blockName] = std::set<std::string>();

        for(auto &I : BB){
           if(isa<AllocaInst>(I)){
                AllocaInst *allocInst = dyn_cast<AllocaInst>(&I);
                allocate_vars.insert(allocInst->getName());
                if("source" == allocInst->getName()){
                    resultMap[blockName].insert("source");
                }
           }
           if(isa<LoadInst>(I)){
		        std::string load_var_name = getSimpleVarName(dyn_cast<Instruction>(I.getOperand(0)));
                if(allocate_vars.find(load_var_name) != allocate_vars.end()){
                    last_load_var_name_set.insert(load_var_name);
                }
	        }
           if(isa<ICmpInst>(I)){
               last_load_var_name_set.clear();
           }
           if(isa<StoreInst>(I)) { 
		        Value* v = I.getOperand(0);
		        Instruction * v_0 = dyn_cast<Instruction>(v);
                std::string var_1 = getSimpleVarName(dyn_cast<Instruction>(I.getOperand(1)));
                
                //remove the item that key = var_1 first
                for(std::vector<AffectItem>::iterator item = affect_values.begin();item != affect_values.end();){
                    if((*item).key == var_1){
                        item = affect_values.erase(item);
                    }else{
                        item++;
                    }
                }

		       
                std::set<std::string> tmp = std::set<std::string>();
                tmp.insert(last_load_var_name_set.begin(),last_load_var_name_set.end());
                AffectItem one = AffectItem();
                one.key = var_1;
                one.vals = tmp;
                affect_values.push_back(one);
		        last_load_var_name_set.clear();
           }
        } 
        blockAffectMap[blockName] = affect_values;
        //onstruct a order to access all blocks
        blockOrder.push_back(blockName);
        if(resultMap[blockName].size() != 0)
            blockStatus[blockName] = 1;
        else
            blockStatus[blockName] = 0;
    }  
    for (auto &BB: *F){
        // Extract the last instruction in the stack (Terminator Instruction)
        std::string blockName = getSimpleNodeLabel(&BB);
        const TerminatorInst *TInst = (&BB)->getTerminator();
        
        // Extract the number of successors the terminator instructor has
	    int NSucc = TInst->getNumSuccessors();
        for (int i = 0;  i < NSucc; ++i) {
            BasicBlock *Succ = TInst->getSuccessor(i);
            std::string succBlockName = getSimpleNodeLabel(Succ);
            previousBlockMap[succBlockName].insert(blockName);

    	}	
            
    }
        
     while(true){
         int blockChanged = 0;
         for(std::list<std::string>::iterator ite = blockOrder.begin();ite != blockOrder.end();ite++){
             std::string name = *ite;
             std::set<std::string> parentNodes = previousBlockMap[name];
             std::set<std::string> original_result = std::set<std::string>();
             original_result.insert(resultMap[name].begin(),resultMap[name].end());
             for(std::set<std::string>::iterator parentIter = parentNodes.begin();parentIter != parentNodes.end();parentIter++){
                std::string parentName = *parentIter;
                resultMap[name].insert(resultMap[parentName].begin(),resultMap[parentName].end());
             }
             for(std::vector<AffectItem>::iterator itemIter = blockAffectMap[name].begin(); itemIter != blockAffectMap[name].end(); itemIter++){
                    std::string storeVar = (*itemIter).key;
                    std::set<std::string> loadVars =  (*itemIter).vals;
                    
                    if(resultMap[name].find(storeVar) == resultMap[name].end() ){
                        for(std::set<std::string>::iterator item = loadVars.begin(); item != loadVars.end();item++){
                            if(resultMap[name].find(*item) != resultMap[name].end()){
                                resultMap[name].insert(storeVar);
                                break;
                            }
                        }
                        
		            }
                    // if the taint variable were assign a untaint variable or constant, need to remove this
                    else {
                        bool remove = true;
                        for(std::set<std::string>::iterator item = loadVars.begin(); item != loadVars.end();item++){
                            if(resultMap[name].find(*item) != resultMap[name].end()){
                                remove = false;
                                break;
                            }
                        }
                        if(remove){
                            resultMap[name].erase(storeVar);
                        }
                        
                    }
	        }
             std::map<std::set<std::string>,int> tmp;
             tmp[original_result] = 1;
             if(tmp.find(resultMap[name]) == tmp.end()){
                 blockStatus[name] = 1;
                 blockChanged++;
             }else{
                 blockStatus[name] = 0;
             }
         }
         if(blockChanged == 0){
             break;
         }

     }
    
	std::map<std::string,std::set<std::string>>::iterator ite;
        for(ite = resultMap.begin(); ite != resultMap.end(); ite++) {
        std::cout << "Block name:"<< ite->first << std::endl;
        std::cout << "taint varabile have: ";
           std::set<std::string>::iterator siter;
           for( siter = ite->second.begin();siter != ite->second.end(); siter++){
           std::cout << *siter << " ";
           }
           std::cout << std::endl;
        }
    
    
    return 0;
}

std::string getSimpleVarName(const Instruction* Ins){
if (NULL != Ins &&  !Ins->getName().empty())
        return Ins->getName().str();
if(NULL != Ins){
    std::string Str;
    raw_string_ostream OS(Str);
    Ins->printAsOperand(OS, false);
    return OS.str();
}
    return "";
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
