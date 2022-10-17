#include "statement.hh"
#include "ast.hh"

ExpressionStatement::ExpressionStatement(Expression* expr) {
    this->expr = expr;
}

void ExpressionStatement::debug(size_t depth) {
    this->expr->debug(depth);
}

void ExpressionStatement::fold(AST* ast) {
    this->expr = this->expr->fold(ast);
}

llvm::Value* ExpressionStatement::codegen(AST* ast) {
    return this->expr->codegen(ast);
}

StatementBlock::StatementBlock(std::vector<Statement*> statements) {
    this->statements = statements;
}

void StatementBlock::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ') << "Block {" << std::endl;
    for (size_t i = 0; i < this->statements.size(); i++) {
        std::cout << std::string((depth + 1) * 4 - 2, ' ') << "Statement: " << i << std::endl;
        this->statements[i]->debug(depth + 1);
    }
    std::cout << std::string(depth * 4, ' ') << "}" << std::endl;
}

void StatementBlock::fold(AST* ast) {
    ast->push_scope();
    for (size_t i = 0; i < this->statements.size(); i++) {
        this->statements[i]->fold(ast);
    }
    ast->pop_scope();
}

llvm::Value* StatementBlock::codegen(AST* ast) {
    for (long unsigned int i = 0; i < this->statements.size(); i++) {
        this->statements[i]->codegen(ast);
    }
    return nullptr;
}

ReturnStatement::ReturnStatement(Expression* expr) {
    this->expr = expr;
}

void ReturnStatement::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ')<< "RETURN" << std::endl;
    this->expr->debug(depth + 1);
}

void ReturnStatement::fold(AST* ast) {
    this->expr = this->expr->fold(ast);
}

llvm::Value* ReturnStatement::codegen(AST* ast) {
    ast->Builder->CreateRet(this->expr->codegen(ast));
    return nullptr;
}

AssignmentStatement::AssignmentStatement(Expression* identifier, Expression* value, bool mut, AtomType type) {
    this->identifier = identifier;
    this->value = value;
    this->mut = mut;
    this->type = type;
}

void AssignmentStatement::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ');
    std::cout << "let " << ((this->mut)?"mutable ":"static ") << ((ExpressionAtomic*)this->identifier)->str << " : ";
    switch (this->type) {
        case t_bool: std::cout << "bool "; break;
        case t_number: std::cout << "int "; break;
        case t_char: std::cout << "char "; break;
        case t_float: std::cout << "float "; break;
        case t_string: std::cout << "string "; break;
        default: std::cerr << "Unknown type for assignment"; break;
    }
    std::cout << " = " << std::endl;
    this->value->debug(depth + 1);
}

void AssignmentStatement::fold(AST* ast) {
    this->value = this->value->fold(ast);
    if (((ExpressionAtomic*)this->value)->type != this->type)
        ast->push_err("Attempted to assign incorrect type to variable");
    ast->scope->push_value(((ExpressionAtomic*)this->identifier)->str, new ScopeValue(this->mut, this->value->get_atomic_type(ast)));
}

llvm::Value* AssignmentStatement::codegen(AST* ast) {
    auto OldBindings = ast->NamedValues;

    llvm::Function *TheFunction = ast->Builder->GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    const std::string VarName = ((ExpressionAtomic*)this->identifier)->str;
    Expression* Init = this->value;

    // Emit the initializer before adding the variable to scope, this prevents
    // the initializer from referencing the variable itself, and permits stuff
    // like this:
    //  var a = 1 in
    //    var a = a in ...   # refers to outer 'a'.
    llvm::Value* InitVal = Init->codegen(ast);
    llvm::Type* init_type;
    switch (this->type) {
        case t_bool: init_type = llvm::Type::getInt1Ty(*(ast->TheContext)); break;
        case t_number: init_type = llvm::Type::getInt64Ty(*(ast->TheContext)); break;
        case t_float: init_type = llvm::Type::getDoubleTy(*(ast->TheContext)); break;
        case t_char: init_type = llvm::Type::getInt8Ty(*(ast->TheContext)); break;
        default: return nullptr; break;
    }
    llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(ast, TheFunction, VarName, init_type);
    ast->Builder->CreateStore(InitVal, Alloca);

    // Remember this binding.
    ast->NamedValues[VarName] = std::make_pair(Alloca, init_type);

    // Return the body computation.
    return InitVal;
}

IfStatement::IfStatement(Expression* condition, Statement* block1, Statement* block2) {
    this->condition = condition;
    this->block1 = block1;
    this->block2 = block2;
}

void IfStatement::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ') << "If" << std::endl;
    this->condition->debug(depth + 1);
    std::cout << std::string(depth * 4, ' ') << "Then" << std::endl;
    this->block1->debug(depth + 1);
    if (this->block2 != nullptr) {
        std::cout << std::string(depth * 4, ' ') << "Else" << std::endl;
        this->block2->debug(depth + 1);
    }
}

void IfStatement::fold(AST* ast) {
    this->condition = this->condition->fold(ast);
    this->block1->fold(ast);
    this->block2->fold(ast);
}

llvm::Value* IfStatement::codegen(AST* ast) {
    llvm::Value *CondV = this->condition->codegen(ast);
    if (!CondV)
        return nullptr;

    llvm::Function *TheFunction = ast->Builder->GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(*(ast->TheContext), "then", TheFunction);
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*(ast->TheContext), "else");
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*(ast->TheContext), "ifcont");

    ast->Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit then value.
    ast->Builder->SetInsertPoint(ThenBB);

    this->block1->codegen(ast);

    ast->Builder->CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    ThenBB = ast->Builder->GetInsertBlock();

    // Emit else block.
    TheFunction->getBasicBlockList().push_back(ElseBB);
    ast->Builder->SetInsertPoint(ElseBB);

    this->block2->codegen(ast);

    ast->Builder->CreateBr(MergeBB);
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
    ElseBB = ast->Builder->GetInsertBlock();

    // Emit merge block.
    TheFunction->getBasicBlockList().push_back(MergeBB);
    ast->Builder->SetInsertPoint(MergeBB);
    llvm::PHINode *PN = ast->Builder->CreatePHI(llvm::Type::getInt1Ty(*(ast->TheContext)), 2, "iftmp");

    PN->addIncoming(llvm::ConstantInt::get(*(ast->TheContext), llvm::APInt(1, 0, false)), ThenBB);
    PN->addIncoming(llvm::ConstantInt::get(*(ast->TheContext), llvm::APInt(1, 1, false)), ElseBB);
    return PN;
}

WhileStatement::WhileStatement(Expression* cond, Statement* block) {
    this->condition = cond;
    this->block = block;
}

void WhileStatement::debug(size_t depth) {
    std::cout << std::string(depth * 4, ' ') << "While" << std::endl;
    this->condition->debug(depth + 1);
    std::cout << std::string(depth * 4, ' ') << "Do" << std::endl;
    this->block->debug(depth + 1);
}

void WhileStatement::fold(AST* ast) {
    this->condition = this->condition->fold(ast);
    this->block->fold(ast);
}

llvm::Value* WhileStatement::codegen(AST* ast) {
    llvm::Value *StartVal = this->condition->codegen(ast);
    if (!StartVal)
        return nullptr;

    // Make the new basic block for the loop header, inserting after current
    // block.
    llvm::Function *TheFunction = ast->Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *PreheaderBB = ast->Builder->GetInsertBlock();
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(*(ast->TheContext), "loop", TheFunction);

    // Create the "after loop" block and insert it.
    llvm::BasicBlock *LoopEndBB = ast->Builder->GetInsertBlock();
    llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(*(ast->TheContext), "afterloop", TheFunction);

    // Insert an explicit fall through from the current block to the LoopBB.
    //ast->Builder->CreateBr(LoopBB);
    ast->Builder->CreateCondBr(StartVal, LoopBB, AfterBB);

    // Start insertion in LoopBB.
    ast->Builder->SetInsertPoint(LoopBB);

    // Start the PHI node with an entry for Start.
    llvm::PHINode *Variable = ast->Builder->CreatePHI(llvm::Type::getInt1Ty(*(ast->TheContext)), 2, "looptmp");
    Variable->addIncoming(StartVal, PreheaderBB);

    // Within the loop, the variable is defined equal to the PHI node.  If it
    // shadows an existing variable, we have to restore it, so save it now.
    //Value *OldVal = NamedValues[VarName];
    //NamedValues[VarName] = Variable;

    // Emit the body of the loop.  This, like any other expr, can change the
    // current BB.  Note that we ignore the value computed by the body, but don't
    // allow an error.
    this->block->codegen(ast);

    // Compute the end condition.
    llvm::Value *EndCond = this->condition->codegen(ast);
    if (!EndCond)
        return nullptr;


    // Insert the conditional branch into the end of LoopEndBB.
    ast->Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    ast->Builder->SetInsertPoint(AfterBB);

    // Add a new entry to the PHI node for the backedge.
    Variable->addIncoming(StartVal, LoopEndBB);

    return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*(ast->TheContext)));
}
