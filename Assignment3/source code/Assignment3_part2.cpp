#include <cstdio>
#include <iostream>
#include <set>
#include <map>
#include <stack>
#include <string>
#include <algorithm>
#include <utility>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"

using namespace llvm;
const static int POS_INF = 1000;
const static int NEG_INF = -1000;

class Interval
{
  public:
	int lower;
	int upper;
	bool empty;
	Interval() { empty = true; }
	Interval(int lower, int upper)
	{
		if (lower <= NEG_INF)
			this->lower = NEG_INF;
		else if (lower >= POS_INF)
			this->lower = POS_INF;
		else
			this->lower = lower;

		if (upper >= POS_INF)
			this->upper = POS_INF;
		else if (upper <= NEG_INF)
			this->upper = NEG_INF;
		else
			this->upper = upper;
		empty = false;
	}
	bool isEmpty()
	{
		return this->empty;
	}
	int getUpper()
	{
		return this->upper;
	}
	int getLower()
	{
		return this->lower;
	}
	void setLower(int lower)
	{
		if (lower <= NEG_INF)
			this->lower = NEG_INF;
		else if (lower >= POS_INF)
			this->lower = POS_INF;
		else
			this->lower = lower;
	}
	void setUpper(int upper)
	{
		if (upper >= POS_INF)
			this->upper = POS_INF;
		else if (upper <= NEG_INF)
			this->upper = NEG_INF;
		else
			this->upper = upper;
	}
	bool operator==(const Interval &a)
	{
		return (a.lower == this->lower && a.upper == this->upper);
	}
	bool operator!=(const Interval &a)
	{
		return (a.lower != this->lower || a.upper != this->upper);
	}
	std::string toString()
	{
		return "[ " + (this->getLower() == NEG_INF ? "NEG_INF" : std::to_string(this->getLower())) + " , " +
			   (this->getUpper() == POS_INF ? "POS_INF" : std::to_string(this->getUpper())) + " ]";
	}
};

typedef std::map<std::string, Interval> BBANALYSIS;
std::map<std::string, BBANALYSIS> analysisMap;

std::set<std::string> activeBlocks;

bool CheckBbanalysisEquals(BBANALYSIS a, BBANALYSIS b)
{
	if (a.size() != b.size())
		return false;
	for (auto it = a.begin(); it != a.end(); ++it)
	{
		if (b.find(it->first) == b.end() || b[it->first] != it->second)
			return false;
	}
	return true;
}
std::string getSimpleVarName(const Instruction *Ins)
{
	if (NULL != Ins && !Ins->getName().empty())
		return Ins->getName().str();
	if (NULL != Ins)
	{
		std::string Str;
		raw_string_ostream OS(Str);
		Ins->printAsOperand(OS, false);
		return OS.str();
	}
	Ins->dump();
	return "";
}
std::string getSimpleNodeLabel(const BasicBlock *Node)
{
	if (!Node->getName().empty())
		return Node->getName().str();
	std::string Str;
	raw_string_ostream OS(Str);
	Node->printAsOperand(OS, false);
	return OS.str();
}
//======================================================================
// Check fixpoint reached
//======================================================================
// F used for add active block
bool fixPointReached(std::map<std::string, BBANALYSIS> oldAnalysisMap, Function *F)
{
	bool result = true;
	if (oldAnalysisMap.empty())
		result = false;
	for (auto it = analysisMap.begin(); it != analysisMap.end(); ++it)
	{

		if (!CheckBbanalysisEquals(oldAnalysisMap[it->first], it->second))
		{
			result = false;
			activeBlocks.insert(it->first);
			// add this block successor to the activeblock
			for (auto &BB : *F)
			{
				std::string blockName = getSimpleNodeLabel(&BB);
				if (!it->first.compare(blockName))
				{
					const TerminatorInst *TInst = BB.getTerminator();
					// Extract the number of successors the terminator instructor has
					int NSucc = TInst->getNumSuccessors();
					for (int i = 0; i < NSucc; ++i)
					{
						BasicBlock *Succ = TInst->getSuccessor(i);
						activeBlocks.insert(getSimpleNodeLabel(Succ));
					}
				}
			}
		}
	}
	return result;
}

// Performs set union
Interval union_intervals(Interval A, Interval B)
{
	return Interval(std::min(A.getLower(), B.getLower()), std::max(A.getUpper(), B.getUpper()));
}

// Performs analysis union
BBANALYSIS union_analysis(BBANALYSIS dest, BBANALYSIS source)
{
	for (auto it = source.begin(); it != source.end(); ++it)
	{
		if (dest.find(it->first) == dest.end())
			dest[it->first] = Interval(it->second.getLower(), it->second.getUpper());
		else
		{
			dest[it->first] = union_intervals(dest[it->first], it->second);
		}
	}

	return dest;
}

//======================================================================
// update Basic Block Analysis
//======================================================================
// Processing Alloca Instruction
void processAlloca(llvm::Instruction *I, BBANALYSIS *analysis)
{
	AllocaInst *allocInst = dyn_cast<AllocaInst>(I);
	(*analysis)[getSimpleVarName(allocInst)] = Interval(NEG_INF, POS_INF);
}

// Processing Store Instruction
void processStore(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		std::string name = getSimpleVarName(dyn_cast<Instruction>(op2));
		if ((*analysis).find(name) != (*analysis).end())
			(*analysis)[name] = Interval(op1Int, op1Int);
	}
	else
	{
		std::string name1 = getSimpleVarName(dyn_cast<Instruction>(op1));
		std::string name2 = getSimpleVarName(dyn_cast<Instruction>(op2));
		if ((*analysis).find(name1) != (*analysis).end())
			(*analysis)[name2] = (*analysis)[name1];
	}
}

// Processing Load Instruction
void processLoad(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	LoadInst *load = dyn_cast<LoadInst>(I);
	std::string name = getSimpleVarName(load);
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		(*analysis)[name] = Interval(op1Int, op1Int);
	}
	else
	{
		std::string name1 = getSimpleVarName(dyn_cast<Instruction>(op1));
		if ((*analysis).find(name1) != (*analysis).end())
			(*analysis)[name] = (*analysis)[name1];
	}
}
Interval extractInterval(Value *op, BBANALYSIS *analysis)
{
	if (isa<ConstantInt>(op))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op);
		int64_t op1Int = CI->getSExtValue();
		return Interval(op1Int, op1Int);
	}
	else
	{
		std::string name = getSimpleVarName(dyn_cast<Instruction>(op));
		if ((*analysis).find(name) != (*analysis).end())
			return (*analysis)[name];
		else
			return Interval();
	}
}
void processAdd(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	Interval one = extractInterval(op1, analysis);
	Interval two = extractInterval(op2, analysis);
	if (one.isEmpty() || two.isEmpty())
		return;
	std::string name = getSimpleVarName(I);
	int lower = NEG_INF;
	int upper = POS_INF;
	if (one.getLower() != NEG_INF && two.getLower() != NEG_INF)
	{
		lower = one.getLower() + two.getLower();
	}
	if (one.getUpper() != POS_INF && two.getUpper() != POS_INF)
	{
		upper = one.getUpper() + two.getUpper();
	}
	(*analysis)[name] = Interval(lower, upper);
}
void processSub(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	Interval one = extractInterval(op1, analysis);
	Interval two = extractInterval(op2, analysis);
	if (one.isEmpty() || two.isEmpty())
		return;
	std::string name = getSimpleVarName(I);
	int lower = NEG_INF;
	int upper = POS_INF;
	if (one.getLower() != NEG_INF && two.getUpper() != POS_INF)
	{
		lower = one.getLower() - two.getUpper();
	}
	if (one.getUpper() != POS_INF && two.getLower() != NEG_INF)
	{
		upper = one.getUpper() - two.getLower();
	}
	(*analysis)[name] = Interval(lower, upper);
}
void processMul(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	Interval one = extractInterval(op1, analysis);
	Interval two = extractInterval(op2, analysis);
	if (one.isEmpty() || two.isEmpty())
		return;
	std::string name = getSimpleVarName(I);
	std::vector<int> tmp;
	tmp.push_back(one.getLower() * two.getLower());
	tmp.push_back(one.getUpper() * two.getUpper());
	tmp.push_back(one.getLower() * two.getUpper());
	tmp.push_back(one.getUpper() * two.getLower());

	(*analysis)[name] = Interval(*(std::min_element(tmp.begin(), tmp.end())), *(std::max_element(tmp.begin(), tmp.end())));
}

void processDiv(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	Interval one = extractInterval(op1, analysis);
	Interval two = extractInterval(op2, analysis);
	if (one.isEmpty() || two.isEmpty())
		return;
	std::string name = getSimpleVarName(I);
	if (two.getLower() == 0 && two.getUpper() == 0)
	{
		(*analysis)[name] = Interval(NEG_INF, POS_INF);
	}
	std::vector<int> tmp;
	if (two.getUpper() == 0)
	{
		tmp.push_back(one.getLower() * (-1));
		tmp.push_back(one.getUpper() * (-1));
		tmp.push_back(one.getLower() / two.getLower());
		tmp.push_back(one.getUpper() / two.getLower());
	}
	else if (two.getLower() == 0)
	{
		tmp.push_back(one.getLower());
		tmp.push_back(one.getUpper());
		tmp.push_back(one.getLower() / two.getUpper());
		tmp.push_back(one.getUpper() / two.getUpper());
	}
	else if (two.getUpper() > 0 and two.getLower() < 0)
	{
		tmp.push_back(one.getLower() * (-1));
		tmp.push_back(one.getUpper() * (-1));
		tmp.push_back(one.getLower() / two.getLower());
		tmp.push_back(one.getUpper() / two.getLower());

		tmp.push_back(one.getLower());
		tmp.push_back(one.getUpper());
		tmp.push_back(one.getLower() / two.getUpper());
		tmp.push_back(one.getUpper() / two.getUpper());
	}
	else
	{
		tmp.push_back(one.getLower() / two.getLower());
		tmp.push_back(one.getUpper() / two.getLower());
		tmp.push_back(one.getLower() / two.getUpper());
		tmp.push_back(one.getUpper() / two.getUpper());
	}
	(*analysis)[name] = Interval(*(std::min_element(tmp.begin(), tmp.end())), *(std::max_element(tmp.begin(), tmp.end())));
}

void processRem(llvm::Instruction *I, BBANALYSIS *analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	Interval one = extractInterval(op1, analysis);
	Interval two = extractInterval(op2, analysis);
	if (one.isEmpty() || two.isEmpty())
		return;
	std::string name = getSimpleVarName(I);

	if (one.getUpper() == POS_INF && two.getUpper() == POS_INF)
	{
		(*analysis)[name] = Interval(0, POS_INF);
	}
	else if (one.getUpper() == POS_INF)
	{
		(*analysis)[name] = Interval(0, two.getUpper() - 1);
	}
	else if (two.getUpper() == POS_INF)
	{
		(*analysis)[name] = Interval(0, one.getUpper());
	}
	else if (one.getUpper() < two.getUpper())
	{
		(*analysis)[name] = Interval(0, one.getUpper());
	}
	else
	{
		(*analysis)[name] = Interval(0, two.getUpper() - 1);
	}
}

BBANALYSIS updateBBAnalysis(BasicBlock *BB, BBANALYSIS analysis)
{
	// Loop through instructions in BB

	for (auto &I : *BB)
	{
		if (isa<AllocaInst>(I))
		{
			processAlloca(&I, &analysis);
		}
		else if (isa<StoreInst>(I))
		{
			processStore(&I, &analysis);
		}
		else if (isa<LoadInst>(I))
		{
			processLoad(&I, &analysis);
		}
		else if (I.getOpcode() == BinaryOperator::Add)
		{
			processAdd(&I, &analysis);
		}
		else if (I.getOpcode() == BinaryOperator::Sub)
		{
			processSub(&I, &analysis);
		}
		else if (I.getOpcode() == BinaryOperator::Mul)
		{
			processMul(&I, &analysis);
		}
		else if (I.getOpcode() == BinaryOperator::SDiv)
		{
			processDiv(&I, &analysis);
		}
		else if (I.getOpcode() == BinaryOperator::SRem)
		{
			processRem(&I, &analysis);
		}
	}
	return analysis;
}

BBANALYSIS ConstConstCondition(llvm::CmpInst *cmp, BBANALYSIS predSet, Value *op1, Value *op2, bool flag)
{
	llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
	int64_t op1Int = CI->getSExtValue();
	llvm::ConstantInt *CII = dyn_cast<ConstantInt>(op2);
	int64_t op2Int = CII->getSExtValue();
	BBANALYSIS emptySet;
	switch (cmp->getPredicate())
	{
	case llvm::CmpInst::ICMP_EQ:
	{
		if ((flag == true && op1Int == op2Int) || (flag == false && op1Int != op2Int))
			return predSet;
		else
		{
			return emptySet;
		}
	}
	case llvm::CmpInst::ICMP_NE:
	{
		if ((flag == true && op1Int != op2Int) || (flag == false && op1Int == op2Int))
			return predSet;
		else
		{
			return emptySet;
		}
	}
	case llvm::CmpInst::ICMP_SGT:
	{
		if ((flag == true && op1Int > op2Int) || (flag == false && op1Int <= op2Int))
			return predSet;
		else
		{
			return emptySet;
		}
	}
	case llvm::CmpInst::ICMP_SLT:
	{
		if ((flag == true && op1Int < op2Int) || (flag == false && op1Int >= op2Int))
			return predSet;
		else
		{
			return emptySet;
		}
	}
	case llvm::CmpInst::ICMP_SGE:
	{
		if ((flag == true && op1Int >= op2Int) || (flag == false && op1Int < op2Int))
			return predSet;
		else
		{
			return emptySet;
		}
	}
	case llvm::CmpInst::ICMP_SLE:
	{
		if ((flag == true && op1Int <= op2Int) || (flag == false && op1Int > op2Int))
			return predSet;
		else
		{
			return emptySet;
		}
	}
	}
}

BBANALYSIS VarConstCondition(llvm::CmpInst *cmp, std::string varName, BBANALYSIS predSet, Interval op1, Value *op2, bool flag)
{

	llvm::ConstantInt *CII = dyn_cast<ConstantInt>(op2);
	int op2Int = CII->getSExtValue();
	BBANALYSIS emptySet;
	switch (cmp->getPredicate())
	{
	case llvm::CmpInst::ICMP_EQ:
	{
		if (flag == true)
		{
			if(op2Int >= op1.getLower() && op2Int <= op1.getUpper()){
				predSet[varName] = Interval(op2Int, op2Int);
				return predSet;
			}else{
				return emptySet;
			}
			
		}
		else
		{
			if(op1.getUpper() == op1.getLower() && op1.getLower() == op2Int){
				return emptySet;
			}else if(op1.getUpper() == op2Int){
				op1.setUpper(op1.getUpper() - 1);
				predSet[varName] = op1;
				return predSet;
			}else if(op1.getLower() == op2Int){
				op1.setLower(op1.getLower() + 1);
				predSet[varName] = op1;
				return predSet;
			}else{
				return predSet;
			}
			
		}
	
	}
	case llvm::CmpInst::ICMP_NE:
	{
		if(flag == true){

			if(op1.getUpper() == op1.getLower() && op1.getLower() == op2Int){
				return emptySet;
			}else if(op1.getUpper() == op2Int){
				op1.setUpper(op1.getUpper() - 1);
				predSet[varName] = op1;
				return predSet;
			}else if(op1.getLower() == op2Int){
				op1.setLower(op1.getLower() + 1);
				predSet[varName] = op1;
				return predSet;
			}else{
				return predSet;
			}
		}else{
			if(op2Int >= op1.getLower() && op2Int <= op1.getUpper()){
				predSet[varName] = Interval(op2Int, op2Int);
				return predSet;
			}else{
				return emptySet;
			}
		}


	}
	case llvm::CmpInst::ICMP_SGT:
	{
		if(flag == true){
			if(op1.getUpper() > op2Int){
				op1.setLower(std::max(op1.getLower(), op2Int + 1));
				predSet[varName] = op1;
				return predSet;
			}else{
				return emptySet;
			}

		}else{
			if(op1.getLower() > op2Int){
				return emptySet;
			}else{
				op1.setUpper(std::min(op1.getUpper(), op2Int));
				predSet[varName] = op1;
				return predSet;
			}

		}

	}
	case llvm::CmpInst::ICMP_SLT:
	{
		if(flag == true){
			if(op1.getLower() >= op2Int){
				return emptySet;
			}else{
				op1.setUpper(std::min(op2Int-1, op1.getUpper()));
				predSet[varName] = op1;
				return predSet;
			}
		}else{
			if(op1.getUpper() < op2Int){
				return emptySet;
			}else{
				op1.setLower(std::max(op1.getLower(),op2Int));
				predSet[varName] = op1;
				return predSet;
			}
		}
	}
	case llvm::CmpInst::ICMP_SGE:
	{
		if(flag == true){
			if(op1.getUpper() < op2Int){
				return emptySet;
			}else{
				op1.setLower(std::max(op1.getLower(),op2Int));
				predSet[varName] = op1;
				return predSet;
			}
		}
		else{
			if(op1.getLower() >= op2Int){
				return emptySet;
			}else{
				op1.setUpper(std::min(op2Int-1, op1.getUpper()));
				predSet[varName] = op1;
				return predSet;
			}
		}

	}
	case llvm::CmpInst::ICMP_SLE:
	{
		if(flag == true){
			if(op1.getLower() > op2Int){
				return emptySet;
			}else{
				op1.setUpper(std::min(op1.getUpper(), op2Int));
				predSet[varName] = op1;
				return predSet;
			}
		}else{
			if(op1.getUpper() > op2Int){
				op1.setLower(std::max(op1.getLower(), op2Int + 1));
				predSet[varName] = op1;
				return predSet;
			}else{
				return emptySet;
			}
		}

	}
	}
}

BBANALYSIS ConstVarCondition(llvm::CmpInst *cmp, std::string varName, BBANALYSIS predSet, Value *op1, Interval op2, bool flag)
{

	llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
	int op1Int = CI->getSExtValue();
	BBANALYSIS emptySet;
	switch (cmp->getPredicate())
	{
	case llvm::CmpInst::ICMP_EQ:
	{
		if (flag == true)
		{
			if(op1Int >= op2.getLower() && op1Int <= op2.getUpper()){
				predSet[varName] = Interval(op1Int, op1Int);
				return predSet;
			}else{
				return emptySet;
			}
			
		}
		else
		{
			if(op2.getUpper() == op2.getLower() && op2.getLower() == op1Int){
				return emptySet;
			}else if(op2.getUpper() == op1Int){
				op2.setUpper(op2.getUpper() - 1);
				predSet[varName] = op2;
				return predSet;
			}else if(op2.getLower() == op1Int){
				op2.setLower(op2.getLower() + 1);
				predSet[varName] = op2;
				return predSet;
			}else{
				return predSet;
			}
			
		}
	
	}
	case llvm::CmpInst::ICMP_NE:
	{
		if(flag == true){

			if(op2.getUpper() == op2.getLower() && op2.getLower() == op1Int){
				return emptySet;
			}else if(op2.getUpper() == op1Int){
				op2.setUpper(op2.getUpper() - 1);
				predSet[varName] = op2;
				return predSet;
			}else if(op2.getLower() == op1Int){
				op2.setLower(op2.getLower() + 1);
				predSet[varName] = op2;
				return predSet;
			}else{
				return predSet;
			}
		}else{
			if(op1Int >= op2.getLower() && op1Int <= op2.getUpper()){
				predSet[varName] = Interval(op1Int, op1Int);
				return predSet;
			}else{
				return emptySet;
			}
		}


	}
	case llvm::CmpInst::ICMP_SGT:
	{
		if(flag == true){
			if(op2.getLower() >= op1Int){
				return emptySet;
			}else{
				op2.setUpper(std::min(op1Int-1, op2.getUpper()));
				predSet[varName] = op2;
				return predSet;
			}
		}
		else{
			
			if(op2.getUpper() < op1Int){
				return emptySet;
			}else{
				op2.setLower(std::max(op2.getLower(),op1Int));
				predSet[varName] = op2;
				return predSet;
			}
		}

	}
	case llvm::CmpInst::ICMP_SGE:
	{
		if(flag == true){
			if(op2.getLower() > op1Int){
				return emptySet;
			}else{
				op2.setUpper(std::min(op2.getUpper(), op1Int));
				predSet[varName] = op2;
				return predSet;
			}

		}else{
			
			if(op2.getUpper() > op1Int){
				op2.setLower(std::max(op2.getLower(), op1Int + 1));
				predSet[varName] = op2;
				return predSet;
			}else{
				return emptySet;
			}

		}

	}
	case llvm::CmpInst::ICMP_SLT:
	{
		if(flag == true){
			if(op2.getUpper() > op1Int){
				op2.setLower(std::max(op2.getLower(), op1Int + 1));
				predSet[varName] = op2;
				return predSet;
			}else{
				return emptySet;
			}
		}else{
			
			if(op2.getLower() > op1Int){
				return emptySet;
			}else{
				op2.setUpper(std::min(op2.getUpper(), op1Int));
				predSet[varName] = op2;
				return predSet;
			}
		}

		
	}
	
	case llvm::CmpInst::ICMP_SLE:
	{
		
		if(flag == true){
			if(op2.getUpper() < op1Int){
				return emptySet;
			}else{
				op2.setLower(std::max(op2.getLower(),op1Int));
				predSet[varName] = op2;
				return predSet;
			}
		}else{
			
			if(op2.getLower() >= op1Int){
				return emptySet;
			}else{
				op2.setUpper(std::min(op1Int-1, op2.getUpper()));
				predSet[varName] = op2;
				return predSet;
			}
		}
	}
	
	}
}


bool isIntersected(Interval one, Interval two)
{

	return std::max(one.getLower(), two.getLower()) <= std::min(one.getUpper(), two.getUpper());
}
Interval intersection(Interval one, Interval two)
{

	return Interval(std::max(one.getLower(), two.getLower()), std::min(one.getUpper(), two.getUpper()));
}

BBANALYSIS VarVarCondition(llvm::CmpInst *cmp, std::string varName1, std::string varName2, BBANALYSIS predSet, Interval op1, Interval op2, bool flag)
{

	BBANALYSIS emptySet;
	switch (cmp->getPredicate())
	{
	case llvm::CmpInst::ICMP_EQ:
	{
		if (flag == true)
		{
			if (isIntersected(op1, op2))
			{
				Interval a = intersection(op1, op2);
				predSet[varName1] = a;
				predSet[varName2] = a;
				return predSet;
			}
			else
			{
				return emptySet;
			}
		}
		else
		{
			if (op1.getLower() == op1.getUpper() && op2.getLower() == op2.getUpper() && op1.getLower() == op2.getLower())
			{
				return emptySet;
			}
			else
			{
				return predSet;
			}
		}
	}
	case llvm::CmpInst::ICMP_NE:
	{
		if (flag == false)
		{
			if (isIntersected(op1, op2))
			{
				Interval a = intersection(op1, op2);
				predSet[varName1] = a;
				predSet[varName2] = a;
				return predSet;
			}
			else
			{
				return emptySet;
			}
		}
		else
		{
			if (op1.getLower() == op1.getUpper() && op2.getLower() == op2.getUpper() && op1.getLower() == op2.getLower())
			{
				return emptySet;
			}
			else
			{
				return predSet;
			}
		}
	}
	case llvm::CmpInst::ICMP_SGT:
	{
		if (flag == true)
		{
			if (op1.getUpper() <= op2.getLower())
				return emptySet;
			else
			{
				op1.setLower(std::min(op2.getLower() + 1, op1.getLower()));
				op2.setUpper(std::min(op1.getUpper() - 1, op2.getUpper()));
				predSet[varName1] = op1;
				predSet[varName2] = op2;
				return predSet;
			}
		}
		else
		{
			if (op1.getLower() > op2.getUpper())
			{
				return emptySet;
			}
			op1.setUpper(std::min(op1.getUpper(), op2.getUpper()));
			op2.setLower(std::max(op1.getLower(), op2.getLower()));
			predSet[varName1] = op1;
			predSet[varName2] = op2;
			return predSet;
		}
	}
	case llvm::CmpInst::ICMP_SLT:
	{
		if (flag == true)
		{
			if (op1.getLower() >= op2.getUpper())
			{
				return emptySet;
			}
			else
			{
				op1.setUpper(std::min(op1.getUpper(), op2.getLower() - 1));
				op2.setLower(std::max(op1.getLower() + 1, op2.getLower()));
				predSet[varName1] = op1;
				predSet[varName2] = op2;
				return predSet;
			}
		}else{
			if(op1.getUpper() < op2.getLower()){
				return emptySet;
			}else{
				op1.setLower(std::max(op1.getLower(),op2.getLower()));
				op2.setUpper(std::min(op1.getUpper(),op2.getUpper()));
				predSet[varName1] = op1;
				predSet[varName2] = op2;
				return predSet;
			}

		}
	}
	case llvm::CmpInst::ICMP_SGE:
	{
		if (flag == true)
		{
			if(op1.getUpper() < op2.getLower()){
				return emptySet;
			}else{
				op1.setLower(std::max(op1.getLower(),op2.getLower()));
				op2.setUpper(std::min(op1.getUpper(),op2.getUpper()));
				predSet[varName1] = op1;
				predSet[varName2] = op2;
				return predSet;
			}
		}
		else
		{
			if (op1.getLower() >= op2.getUpper())
			{
				return emptySet;
			}
			else
			{
				op1.setUpper(std::min(op1.getUpper(), op2.getLower() - 1));
				op2.setLower(std::max(op1.getLower() + 1, op2.getLower()));
				predSet[varName1] = op1;
				predSet[varName2] = op2;
				return predSet;
			}
		}
	}
	case llvm::CmpInst::ICMP_SLE:
	{
		if (flag == true)
		{
			if (op1.getLower() > op2.getUpper())
			{
				return emptySet;
			}
			op1.setUpper(std::min(op1.getUpper(), op2.getUpper()));
			op2.setLower(std::max(op1.getLower(), op2.getLower()));
			predSet[varName1] = op1;
			predSet[varName2] = op2;
			return predSet;
		}
		else
		{
			if (op1.getUpper() <= op2.getLower())
				return emptySet;
			else
			{
				op1.setLower(std::min(op2.getLower() + 1, op1.getLower()));
				op2.setUpper(std::min(op1.getUpper() - 1, op2.getUpper()));
				predSet[varName1] = op1;
				predSet[varName2] = op2;
				return predSet;
			}
		}
	}
	}
}

BBANALYSIS applyCond(BBANALYSIS predSet, BasicBlock *predecessor, BasicBlock *BB)
{
	for (auto &I : *predecessor)
	{
		if (isa<BranchInst>(I))
		{
			BranchInst *br = dyn_cast<BranchInst>(&I);
			if (!br->isConditional())
				return predSet;
			llvm::CmpInst *cmp = dyn_cast<llvm::CmpInst>(br->getCondition());

			Value *op1 = cmp->getOperand(0);
			Value *op2 = cmp->getOperand(1);

			// now need to consider 3 cases: const-const  const-var   var-var
			bool flag;
			if (BB == br->getOperand(2))
				flag = true;
			if (BB == br->getOperand(1))
				flag = false;

			if (isa<ConstantInt>(op1) && isa<ConstantInt>(op2))
			{
				return ConstConstCondition(cmp, predSet, op1, op2, flag);
			}

			if (isa<LoadInst>(dyn_cast<Instruction>(cmp->getOperand(0))) && isa<ConstantInt>(op2))
			{
				Instruction *l = dyn_cast<Instruction>(cmp->getOperand(0));
				std::string varName = getSimpleVarName(dyn_cast<Instruction>(l->getOperand(0)));
				if (predSet.find(varName) == predSet.end())
				{
					return predSet;
				}

				return VarConstCondition(cmp, varName, predSet, predSet[varName], op2, flag);
			}

			if (isa<LoadInst>(dyn_cast<Instruction>(cmp->getOperand(0))) && isa<LoadInst>(dyn_cast<Instruction>(cmp->getOperand(1))))
			{
				Instruction *l1 = dyn_cast<Instruction>(cmp->getOperand(0));
				std::string varName1 = getSimpleVarName(dyn_cast<Instruction>(l1->getOperand(0)));
				Instruction *l2 = dyn_cast<Instruction>(cmp->getOperand(1));
				std::string varName2 = getSimpleVarName(dyn_cast<Instruction>(l2->getOperand(0)));

				if (predSet.find(varName1) == predSet.end() || predSet.find(varName2) == predSet.end())
				{
					return predSet;
				}
				return VarVarCondition(cmp,varName1,varName2,predSet,predSet[varName1],predSet[varName2],flag);
			}
		}
	}
	return predSet;
}

// update Graph Analysis
void updateGraphAnalysis(Function *F)
{
	for (auto &BB : *F)
	{
		BBANALYSIS predUnion;
		// Load the current stored analysis for all predecessor nodes
		for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
		{

			BasicBlock *predecessor = *it;
			std::string predecessorName = getSimpleNodeLabel(predecessor);
			if (activeBlocks.find(predecessorName) != activeBlocks.end())
			{
				// in this way, we need to merge all the predecessor information
				BBANALYSIS conditional = applyCond(analysisMap[getSimpleNodeLabel(predecessor)], predecessor, &BB);
				predUnion = union_analysis(predUnion, conditional);
			}
			/*
			else
			{
				llvm::errs() << "Block " << predecessorName << " not in the active list\n";
			}*/
		}

		BBANALYSIS BBAnalysis = updateBBAnalysis(&BB, predUnion);
		BBANALYSIS OldBBAnalysis = analysisMap[getSimpleNodeLabel(&BB)];
		if (!CheckBbanalysisEquals(OldBBAnalysis, BBAnalysis))
		{
			analysisMap[getSimpleNodeLabel(&BB)] = union_analysis(BBAnalysis, OldBBAnalysis);
		}
	}
}

//======================================================================
// main function
//======================================================================

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

	// 1.Extract Function main from Module M
	Function *F = M->getFunction("main");

	// 2.Define analysisMap as a mapping of basic block labels to empty set (of instructions):
	for (auto &BB : *F)
	{
		BBANALYSIS emptySet;
		std::string blockName = getSimpleNodeLabel(&BB);
		analysisMap[blockName] = emptySet;
		activeBlocks.insert(blockName);
	}
	// Note: All variables are of type "alloca" instructions. Ex.
	// Variable a: %a = alloca i32, align 4

	// Keeping a snapshot of the previous ananlysis
	std::map<std::string, BBANALYSIS> oldAnalysisMap;
	// Fixpoint Loop

	int i = 0;
	while (!fixPointReached(oldAnalysisMap, F))
	{
		oldAnalysisMap.clear();
		oldAnalysisMap.insert(analysisMap.begin(), analysisMap.end());
		updateGraphAnalysis(F);
		activeBlocks.clear();
	}

	for (auto it = analysisMap.begin(); it != analysisMap.end(); ++it)
	{
		llvm::errs() << "Block name is:" << it->first << "\n";
		BBANALYSIS analysis = it->second;
		for (auto it1 = analysis.begin(); it1 != analysis.end(); ++it1)
		{
			if(it1->first.find("%")== std::string::npos){
				llvm::errs() << it1->first;
				llvm::errs() << "\t";
				llvm::errs() << (it1->second).toString();
				llvm::errs() << "\n";
			}
			
		}
	}

	return 0;
}
