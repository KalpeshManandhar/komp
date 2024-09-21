#include "asm.h"

#include <string>

void IR::generateAssembly(Node *const current) 
{
    // Yoinked from printParseTree()
    if (!current) 
    {
        return;
    }

    switch (current->tag) {
    case Node::NODE_DECLARATION: {
        Declaration *d = (Declaration*) current;
        std::cout << ".data\n";
        for (const auto &decl : d->decln) 
        {
            // Initialize to 0
            std::cout << decl.identifier.string << ": .word 0\n"; 
        }
        std::cout << ".text\n";
        break;
    }

    case Node::NODE_STMT_BLOCK: 
    {
        StatementBlock *b = (StatementBlock*) current;
        for (auto &stmt : b->statements) 
        {
            generateAssembly(stmt);
        }
        break;
    }

    case Node::NODE_RETURN: 
    {
        ReturnNode *r = (ReturnNode*) current;
        // Load immediate into a0 (return value register)
        std::cout << "    li a0, "; 
        if (r->returnVal->tag == Node::NODE_SUBEXPR) 
        {
            Subexpr *s = (Subexpr *)r->returnVal;
            if (s->subtag == Subexpr::SUBEXPR_LEAF) 
            {
                std::cout << s->leaf.string << "\n"; 
            }
        }
        std::cout << "    ret\n"; 
        break;
    }

    default:
        break;
    }
}

void IR::funcAssembly(Splice funcName, Function *foo) 
{
    std::cout << ".globl "<<funcName <<"\n";
    std::cout << funcName<<":\n";
    
    std::cout << "    addi sp, sp, -16\n"; // Allocate stack space
    std::cout << "    sd ra, 8(sp)\n";     // Save return address  

    for (int i = 5; auto &param : foo->parameters) 
    {        
        std::cout << "mv a" << (i--) << ", a0\n";
    }

    
    generateAssembly(foo->block);
    
    std::cout << "    ld ra, 8(sp)\n";      // Restore return address
    std::cout << "    addi sp, sp, 16\n";   // Deallocate stack space
    std::cout << "    ret\n";                // Return from main
}


    

    // for (auto &pair: ir.p.functions.entries)
    // {
    //     std::cout<<"Function: " <<pair.second.identifier <<"{\n";
    //     Function *foo = &pair.second.info;
    //     std::cout<<"\tReturn type: " <<dataTypePrintf(foo->returnType)<<"\n";
    //     std::cout<<"\tParameters: " <<"{\n";
    //     for (auto &param: foo->parameters){
    //         std::cout<<"\t\t"<<param.identifier.string<< " : " <<dataTypePrintf(param.type)<<"\n";
    //     }
    //     std::cout<<"\t}\n";
        
    //     printParseTree(foo->block, 1);
    //     std::cout<<"}\n";
        
    // }

