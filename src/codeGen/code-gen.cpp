#include "code-gen.h"
#include <tokenizer/token.h>


void CodeGenerator::generateNode(const Node *current, StatementBlock *scope){
    if (!current){
        return;
    }

    switch (current->tag){
    
    case Node::NODE_DECLARATION:{
        Declaration *d = (Declaration *)current;
        buffer << ".data\n";
        for (const auto &decl : d->decln){
            // Initialize to 0
            buffer << decl.identifier.string << ": .word 0\n";
        }
        buffer << ".text\n";
        break;
    }

    case Node::NODE_STMT_BLOCK:{
        StatementBlock *b = (StatementBlock *)current;
        for (auto &stmt : b->statements){
            generateNode(stmt, scope);
        }
        break;
    }

    case Node::NODE_RETURN:{
        ReturnNode *r = (ReturnNode *)current;
        // Load immediate into a0 (return value register)
        Subexpr *s = (Subexpr *)r->returnVal;

        if (s->subtag == Subexpr::SUBEXPR_LEAF){
            buffer << "    li a0, ";

            if (_matchv(s->leaf, LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
                buffer << s->leaf.string << "\n";
            }
        }

        StatementBlock *funcScope = scope->getParentFunction();
        // jump to function epilogue instead of ret
        buffer << "    j ."<< funcScope->funcName.string << "_ep\n";
        break;
    }

    default:
        break;
    }
}

void CodeGenerator::generateFunction(Function *foo){
    buffer << "    .globl " << foo->funcName.string << "\n";
    buffer << foo->funcName.string << ":\n";
    // Function prologue
    buffer << "    addi sp, sp, -16\n"; // Allocate stack space
    buffer << "    sd ra, 8(sp)\n";     // Save return address

    // Moving the parameters to registers
    for (int i = 5; auto &param : foo->parameters)
    {
        buffer << "mv a" << (i--) << ", a0\n";
    }

    // Function opeartions
    // Needs improvement
    generateNode(foo->block, foo->block);

    // Function epilogue
    buffer << "."<<foo->funcName.string << "_ep:\n";
    buffer << "    ld ra, 8(sp)\n";    // Restore return address
    buffer << "    addi sp, sp, 16\n"; // Deallocate stack space
    buffer << "    ret\n";             // Return from function
}

// Generated from AI to write to a .s file
void CodeGenerator::writeAssemblyToFile(const char *filename)
{
    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << outputBuffer.str(); // Write the combined assembly to file
        outFile.close();
        std::cout << "Assembly written to " << filename << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

void CodeGenerator::printAssembly(){
    std::cout << outputBuffer.str() << std::endl;
}


void CodeGenerator::generateAssembly(IR *ir){


    outputBuffer << "    .text\n";

    for (auto &pair: ir->functions.entries){
        generateFunction(&pair.second.info);

        Function *foo = &pair.second.info;

        // Appending the function assembly to outputBuffer
        outputBuffer << buffer.str();
        buffer.str("");
        buffer.clear();
        
    }
}
