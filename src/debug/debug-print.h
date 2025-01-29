#include <IR/ir.h>

static void printMIRPrimitive(MIR_Primitive* p, int depth){
    if (!p){
        return;
    }

    auto printTabs = [](int n){
        for (int i = 0; i<n; i++){
            std::cout << "    ";
        }
    };

    switch (p->ptag){
    case MIR_Primitive::PRIM_IF:{
        MIR_If* inode = (MIR_If*) p;
        
        printTabs(depth);
        std::cout << "if: \n";
        while (inode) {

            if (inode->condition){                
                printTabs(depth + 1);
                std::cout << "condition: \n";
                printMIRPrimitive(inode->condition, depth + 1);
            }
    
            printTabs(depth + 1);
            std::cout << "scope: \n";
            printMIRPrimitive(inode->scope, depth + 1);

            inode = inode->next;
        }
                
        break;
    }
    case MIR_Primitive::PRIM_LOOP:{
        MIR_Loop* lnode = (MIR_Loop*) p;
        
        printTabs(depth);
        std::cout << "Loop: \n";

        printTabs(depth + 1);
        std::cout << "condition: \n";
        printMIRPrimitive(lnode->condition, depth + 1);

        printTabs(depth + 1);
        printMIRPrimitive(lnode->scope, depth + 1);
                
        break;
    }
    
    case MIR_Primitive::PRIM_STACK_ALLOC:{
        break;
    }
    case MIR_Primitive::PRIM_STACK_FREE:{
        break;
    }
    
    case MIR_Primitive::PRIM_RETURN:{
        MIR_Return* rnode = (MIR_Return*) p;
        printTabs(depth);
        std::cout << "ret : \n";
        
        printMIRPrimitive(rnode->returnValue, depth + 1);
    
        break;
    }
    case MIR_Primitive::PRIM_JUMP:{
        assert(false && "Implement jump print");
        break;
    }
    case MIR_Primitive::PRIM_EXPR:{
        MIR_Expr* enode = (MIR_Expr*) p;
        
        const char* EXPR_TYPE_STRINGS[] = {
            "expr_addressof",
            "expr_load",
            "expr_index",
            "expr_leaf",
            "expr_load_address",
            "expr_load_immediate",
            "expr_store",
            "expr_call",
            "expr_cast",
            "expr_binary",
            "expr_unary",
            "expr_function_call",
        };

        printTabs(depth);
        std::cout << EXPR_TYPE_STRINGS[enode->tag] << ": \n";

        switch (enode->tag){
        case MIR_Expr::EXPR_ADDRESSOF:{
            printMIRPrimitive(enode->addressOf.of, depth + 1);
            break;
        }
        case MIR_Expr::EXPR_LOAD:{
            printTabs(depth + 1);
            std::cout << "size: " << enode->load.size << "\n";
            printTabs(depth + 1);
            std::cout << "offset: " << enode->load.offset << "\n";
            printTabs(depth + 1);
            std::cout << "base address of: \n";
            printMIRPrimitive(enode->load.base, depth + 1);
            break;
        }
        case MIR_Expr::EXPR_INDEX:{
            printTabs(depth + 1);
            std::cout << "size: " << enode->index.size << "\n";
            printTabs(depth + 1);
            std::cout << "base address: \n";
            printMIRPrimitive(enode->index.base, depth + 1);
            printTabs(depth + 1);
            std::cout << "index: \n";
            printMIRPrimitive(enode->index.index, depth + 1);
            break;
        }
        case MIR_Expr::EXPR_LEAF:{
            printTabs(depth + 1);
            std::cout << "symbol: " << enode->leaf.val << "\n";
            break;
        }

        case MIR_Expr::EXPR_LOAD_ADDRESS:{
            printTabs(depth + 1);
            std::cout << "offset: " << enode->loadAddress.offset << "\n";
            printTabs(depth + 1);
            std::cout << "base address of: \n";
            printMIRPrimitive(enode->loadAddress.base, depth + 1);
            break;
        }
        case MIR_Expr::EXPR_LOAD_IMMEDIATE:{
            printTabs(depth + 1);
            std::cout << "immediate value: " << enode->immediate.val << "\n";
            break;
        }
        
        case MIR_Expr::EXPR_STORE:{
            printTabs(depth + 1);
            std::cout << "size: " << enode->store.size << "\n";
            printTabs(depth + 1);
            std::cout << "offset: " << enode->store.offset << "\n";
            printTabs(depth + 1);
            std::cout << "base address of: \n";
            printMIRPrimitive(enode->store.left, depth + 1);
            printTabs(depth + 1);
            std::cout << "value: \n";
            printMIRPrimitive(enode->store.right, depth + 1);
            break;
        }

        case MIR_Expr::EXPR_CALL:{
            assert(false && "print calls support baka");
            break;
        }
        
        case MIR_Expr::EXPR_CAST:{
            printTabs(depth + 1);
            std::cout << "from: " << enode->cast._from.name << "\n";
            printTabs(depth + 1);
            std::cout << "to: " << enode->cast._to.name << "\n";
            printTabs(depth + 1);
            std::cout << "value: \n";
            printMIRPrimitive(enode->cast.expr, depth + 1);
            break;
        }
        case MIR_Expr::EXPR_BINARY:{
            static const char* BinaryTypeStrings[] = {
                "expr_iadd",
                "expr_isub",
                "expr_imul",
                "expr_idiv",
                "expr_uadd",
                "expr_usub",
                "expr_umul",
                "expr_udiv",
                "expr_fadd",
                "expr_fsub",
                "expr_fmul",
                "expr_fdiv",
                "expr_ibitwise_and",
                "expr_ibitwise_or",
                "expr_ibitwise_xor",
                "expr_logical_and",
                "expr_logical_or",
                "expr_ibitwise_lshift",
                "expr_ibitwise_rshift",
                "expr_icompare_lt",
                "expr_icompare_gt",
                "expr_icompare_le",
                "expr_icompare_ge",
                "expr_icompare_eq",
                "expr_icompare_neq",
                "expr_fcompare_lt",
                "expr_fcompare_gt",
                "expr_fcompare_le",
                "expr_fcompare_ge",
                "expr_fcompare_eq",
                "expr_fcompare_neq",
            };
            printTabs(depth + 1);
            std::cout << "op: " << BinaryTypeStrings[int(enode->binary.op)] << "\n";
            printTabs(depth + 1);
            std::cout << "left: \n";
            printMIRPrimitive(enode->binary.left, depth + 1);
            printTabs(depth + 1);
            std::cout << "right: \n";
            printMIRPrimitive(enode->binary.right, depth + 1);

            break;
        }
        case MIR_Expr::EXPR_UNARY:{
            static const char* UnaryTypeStrings[] = {
                "expr_inegate",
                "expr_fnegate",
                "expr_ibitwise_not",
                "expr_logical_not",
            };
            printTabs(depth + 1);
            std::cout << "op: " << UnaryTypeStrings[int(enode->unary.op)] << "\n";
            printTabs(depth + 1);
            std::cout << "expr: \n";
            printMIRPrimitive(enode->unary.unarySubexpr, depth + 1);
            break;
        }

        case MIR_Expr::EXPR_FUNCTION_CALL:{
            assert(false && "print calls support baka");
            break;
        }
        default:
            assert(false && "something is not supported");
            break;
        }
        
        break;
    }
    case MIR_Primitive::PRIM_SCOPE:{
        
        MIR_Scope* snode = (MIR_Scope*) p;
        
        printTabs(depth);
        std::cout << "scope: \n";
        printTabs(depth);
        std::cout << "statements: \n";
        for (auto &prim : snode->statements){
            printMIRPrimitive(prim, depth + 1);
        }
        printTabs(depth);
        std::cout << "symbols: \n";
        for (auto &symbolName : snode->symbols.order){
            MIR_Datatype type = snode->symbols.getInfo(symbolName).info;
            
            printTabs(depth + 2);
            std::cout << symbolName << ": " << type.name << "\n";
        }



        break;
    }
    default:
        break;
    }
}


static void printMIR(MIR *mir){
    printMIRPrimitive(mir->global, 0);
    for (auto &fooEntry : mir->functions.entries){
        MIR_Function foo = fooEntry.second.info;
        std::cout << "Function: " << foo.funcName << "\n";
        std::cout << "Return type: " << foo.returnType.name << "\n";
        std::cout << "Params: \n";

        for (auto &param : foo.parameters){
            std::cout << "\t" << param.identifier << ": " << param.type.name << "\n";
        }

        std::cout << "Scope: \n";
        printMIRPrimitive(foo.scope, 1);
    }
}



static void printParseTree(Node *const current, int depth = 0){
    if (!current){
        return;
    }


    auto printTabs = [&](int n){
        const int tabSize = 4;
        printf("%*s ", n * tabSize, "");
    };

    printTabs(depth);
    printf("%s : {\n", NODE_TAG_STRINGS[current->tag]);

    switch (current->tag){
    case Node::NODE_SUBEXPR:{
        Subexpr * s = (Subexpr *)current;

        switch (s->subtag){
        case Subexpr::SUBEXPR_BINARY_OP :{
            printParseTree(s->left, depth + 1);
            
            printTabs(depth + 1);
            std::cout<<"OP: " << s->op.string<< "\n";
            
            printParseTree(s->right, depth + 1);   
            break;
        }

        case Subexpr::SUBEXPR_LEAF :{
            printTabs(depth + 1);
            std::cout<<"LEAF: " << s->leaf.string<< "\n";
            break;
        }

        case Subexpr::SUBEXPR_RECURSE_PARENTHESIS :{
            printTabs(depth + 1);
            std::cout<<"(\n";
            printParseTree(s->inside, depth + 1);
            printTabs(depth + 1);
            std::cout<<")\n";
            break;
        }

        case Subexpr::SUBEXPR_UNARY : {
            printTabs(depth + 1);
            std::cout<<"UNARY OP: " <<s->unaryOp.string << "\n";
            printParseTree(s->unarySubexpr, depth + 1);
            break;
        }
        case Subexpr::SUBEXPR_FUNCTION_CALL : {
            printTabs(depth + 1);
            std::cout<<"Function:  " <<s->functionCall->funcName.string << "\n";
            printTabs(depth + 1);
            std::cout<<"Parameters: \n ";

            for (auto &arg : s->functionCall->arguments){
                printParseTree(arg, depth + 1);
            }

            break;
        }
        case Subexpr::SUBEXPR_CAST : {
            printTabs(depth + 1);
            std::cout<<"CAST TO: " <<dataTypePrintf(s->to) << "\n";
            printParseTree(s->expr, depth + 1);
            break;
        }
        default:
            break;
        }
        break;
    }
    case Node::NODE_DECLARATION: {
        Declaration *d = (Declaration*) current;

        for (auto &decl: d->decln){
            printTabs(depth + 1);
            std::cout<< decl.identifier.string<<": " << dataTypePrintf(decl.type)<<"\n";
            
            if (decl.initValue){
                printTabs(depth + 1);
                std::cout<< "initializer value: \n";
                printParseTree(decl.initValue, depth + 1);
            }
        }
        break;
    }
    
    case Node::NODE_STMT_BLOCK: {
        StatementBlock *b = (StatementBlock*) current;

        for (auto &stmt: b->statements){
            printParseTree(stmt, depth + 1);
        }
        
        if (b->symbols.count() > 0){
            printTabs(depth+1);
            std::cout<<"Symbol table:\n";
            for (auto &pair : b->symbols.entries){
                printTabs(depth + 2);
                std::cout<<pair.second.identifier <<": " << dataTypePrintf(pair.second.info)<< "\n";
            }
        }
        
        if (b->structs.count() > 0){
            printTabs(depth+1);
            std::cout<<"Struct table:\n";
            for (auto &pair : b->structs.entries){
                Struct strct = pair.second.info;
                printTabs(depth + 2);
                std::cout<<"\tStruct " << strct.structName.string<< "{\n";

                for (auto &m: strct.members.entries){
                    printTabs(depth + 3);
                    std::cout<<m.second.info.memberName.string <<": " << dataTypePrintf(m.second.info.type)<<"\n"; 
                }
                printTabs(depth + 2);
                std::cout<<"\t}\n"; 

            }
        }
        break;
    }


    case Node::NODE_IF_BLOCK: {
        IfNode *i = (IfNode*) current;
        
        printTabs(depth + 1);
        std::cout<<"{\n";
        

        while (i){
            printTabs(depth + 1);

            switch (i->subtag){
                case IfNode::IF_NODE:{
                    std::cout<< "IF: \n";

                    printTabs(depth + 1);
                    std::cout<< "condition: \n";
                    printParseTree(i->condition, depth + 1);
                    printTabs(depth + 1);
                    std::cout<< "statements: \n";
                    
                    printParseTree(i->block, depth + 1);

                    if (i->nextIf){
                        printTabs(depth + 1);
                        std::cout<< "ELSE ";
                    }
                    break;
                }
                case IfNode::ELSE_NODE:{
                    std::cout<< ": \n";
                    printTabs(depth + 1);
                    std::cout<< "statements: \n";
                    printParseTree(i->block, depth + 1);
                    
                    break;
                }
            }
            i = i->nextIf;
        }
        
        std::cout<<"}\n";
        break;
    }

    case Node::NODE_WHILE: {
        WhileNode *w = (WhileNode*) current;
        
        printTabs(depth + 1);
        std::cout<< "condition: \n";
        printParseTree(w->condition, depth + 1);
        printTabs(depth + 1);
        std::cout<< "statements: \n";
                    
        printParseTree(w->block, depth + 1);

        break;
    }
    
    case Node::NODE_FOR: {
        ForNode *f = (ForNode*) current;
        
        printTabs(depth + 1);
        std::cout<< "init: \n";
        printParseTree(f->init, depth+1);

        printTabs(depth + 1);
        std::cout<< "exit condition: \n";
        printParseTree(f->exitCondition, depth+1);

        printTabs(depth + 1);
        std::cout<< "update: \n";
        printParseTree(f->update, depth+1);
        
        printTabs(depth + 1);
        std::cout<< "statements: \n";
        printParseTree(f->block, depth + 1);

        break;
    }
    
    case Node::NODE_RETURN: {
        ReturnNode *r = (ReturnNode*) current;
        printTabs(depth + 1);        
        std::cout<<"Value: \n";
        printParseTree(r->returnVal, depth + 1);

        break;
    }

    default:
        break;
    }
    printTabs(depth);
    printf("}\n");


} 