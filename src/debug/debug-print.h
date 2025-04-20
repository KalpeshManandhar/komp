#include <IR/ir.h>

static int mirNodeCounter = 0; 
static std::string generateMIRDotNode(MIR_Primitive* mirNode, std::ostringstream& dotStream) {
    // Modified form of Kelp's printMIRPrimitive()
    if (!mirNode) return "";

    int currentNode = mirNodeCounter++;
    std::string nodeLabel;
    std::string nodeProps;

    // Lambda for recursive child operations
    auto handleChildren = [&](const char* label, MIR_Primitive* child) {
        std::string childNode = generateMIRDotNode(child, dotStream);
        if (!childNode.empty()) {
            dotStream << "    node" << currentNode << " -> " << childNode << " [label=\"" << label << "\"];\n";
        }
    };

    switch (mirNode->ptag) {
        case MIR_Primitive::PRIM_IF: {
            MIR_If* ifNode = static_cast<MIR_If*>(mirNode);
            nodeLabel = "IF";
            MIR_If* currentIf = ifNode;
            while (currentIf) {
                handleChildren("condition", currentIf->condition);
                handleChildren("scope", currentIf->scope);
                currentIf = currentIf->next;
            }
            break;
        }
        case MIR_Primitive::PRIM_LOOP: {
            MIR_Loop* loopNode = static_cast<MIR_Loop*>(mirNode);
            nodeLabel = "LOOP";
            handleChildren("condition", loopNode->condition);
            handleChildren("scope", loopNode->scope);
            break;
        }
        case MIR_Primitive::PRIM_RETURN: {
            MIR_Return* returnNode = static_cast<MIR_Return*>(mirNode);
            nodeLabel = "RETURN";
            handleChildren("value", returnNode->returnValue);
            break;
        }
        case MIR_Primitive::PRIM_JUMP: {
            MIR_Jump* jumpNode = static_cast<MIR_Jump*>(mirNode);
            nodeLabel = "JUMP";
            nodeProps = "to: .L" + std::to_string(jumpNode->jumpLabel);
            break;
        }
        case MIR_Primitive::PRIM_STACK_ALLOC: {
            nodeLabel = "STACK_ALLOC";
            break;
        }
        case MIR_Primitive::PRIM_STACK_FREE: {
            nodeLabel = "STACK_FREE";
            break;
        }
        
        case MIR_Primitive::PRIM_EXPR: {
            MIR_Expr* exprNode = static_cast<MIR_Expr*>(mirNode);
            const char* EXPR_TYPE_STRINGS[] = {
                "expr_addressof", "expr_load", "expr_index", "expr_leaf",
                "expr_load_address", "expr_load_immediate", "expr_store",
                "expr_call", "expr_cast", "expr_binary", "expr_unary", "expr_function_call"
            };
            nodeLabel = EXPR_TYPE_STRINGS[exprNode->tag];

            switch (exprNode->tag) {
                case MIR_Expr::EXPR_ADDRESSOF: {
                    nodeLabel = "EXPR_LEAF: " + std::string(exprNode->addressOf.symbol.data, exprNode->addressOf.symbol.len);
                    break;
                }
                case MIR_Expr::EXPR_LOAD: {
                    nodeProps = "size: " + std::to_string(exprNode->load.size) +
                               "\\noffset: " + std::to_string(exprNode->load.offset);
                    handleChildren("base", exprNode->load.base);
                    break;
                }
                case MIR_Expr::EXPR_INDEX: {
                    nodeProps = "size: " + std::to_string(exprNode->index.size);
                    handleChildren("base", exprNode->index.base);
                    handleChildren("index", exprNode->index.index);
                    break;
                }
                case MIR_Expr::EXPR_LEAF: {
                    nodeLabel = "EXPR_LEAF: " + std::string(exprNode->leaf.val.data, exprNode->leaf.val.len);
                    break;
                }
                case MIR_Expr::EXPR_LOAD_ADDRESS: {
                    nodeProps = "offset: " + std::to_string(exprNode->loadAddress.offset);
                    handleChildren("base", exprNode->loadAddress.base);
                    break;
                }
                case MIR_Expr::EXPR_LOAD_IMMEDIATE: {
                    nodeLabel = "EXPR_LOAD_IMMEDIATE: " + std::string(exprNode->immediate.val.data, exprNode->immediate.val.len);
                    break;
                }
                case MIR_Expr::EXPR_STORE: {
                    nodeProps = "size: " + std::to_string(exprNode->store.size) +
                               "\\noffset: " + std::to_string(exprNode->store.offset);
                    handleChildren("address", exprNode->store.left);
                    handleChildren("value", exprNode->store.right);
                    break;
                }
                case MIR_Expr::EXPR_CALL: {
                    MIR_FunctionCall* callNode = exprNode->functionCall;
                    nodeLabel = "EXPR_CALL: " + std::string(callNode->funcName.data, callNode->funcName.len);
                    for (size_t i = 0; i < callNode->arguments.size(); ++i) {
                        handleChildren(("arg" + std::to_string(i)).c_str(), callNode->arguments[i]);
                    }
                    break;
                }
                case MIR_Expr::EXPR_CAST: {
                    nodeProps = "from: " + std::string(exprNode->cast._from.name) +
                               "\\nto: " + std::string(exprNode->cast._to.name);
                    handleChildren("expr", exprNode->cast.expr);
                    break;
                }
                
                case MIR_Expr::EXPR_BINARY: {
                    static const char* BINARY_OPS[] = {
                        "iadd", "isub", "imul", "idiv", "imod", "uadd", "usub", "umul", "udiv", "umod",
                        "fadd", "fsub", "fmul", "fdiv", "ibitwise_and", "ibitwise_or", "ibitwise_xor",
                        "logical_and", "logical_or", "ibitwise_lshift", "ibitwise_rshift", "expr_arithmetic_rshift",
                        "icompare_lt", "icompare_gt", "icompare_le", "icompare_ge", "icompare_eq", "icompare_neq",
                        "ucompare_lt", "ucompare_gt", "ucompare_le", "ucompare_ge", "ucompare_eq", "ucompare_neq",
                        "fcompare_lt", "fcompare_gt", "fcompare_le", "fcompare_ge", "fcompare_eq", "fcompare_neq"
                    };
                    nodeProps = "op: " + std::string(BINARY_OPS[static_cast<int>(exprNode->binary.op)]);
                    handleChildren("left", exprNode->binary.left);
                    handleChildren("right", exprNode->binary.right);
                    break;
                }
                case MIR_Expr::EXPR_UNARY: {
                    static const char* UNARY_OPS[] = {
                        "inegate", "fnegate", "ibitwise_not", "logical_not"
                    };
                    nodeProps = "op: " + std::string(UNARY_OPS[static_cast<int>(exprNode->unary.op)]);
                    handleChildren("expr", exprNode->unary.expr);
                    break;
                }
                default:
                    nodeLabel = "UNKNOWN_EXPR";
                    break;
            }
            break;
        }
        case MIR_Primitive::PRIM_SCOPE: {
            MIR_Scope* scopeNode = static_cast<MIR_Scope*>(mirNode);
            nodeLabel = "SCOPE";
            for (auto& stmt : scopeNode->statements) {
                handleChildren("stmt", stmt);
            }
            
            for (auto& symbolName : scopeNode->symbols.order) {
                MIR_Datatype type = scopeNode->symbols.getInfo(symbolName).info;
                dotStream << "    node" << currentNode << "_" << symbolName 
                          << " [label=\"" << symbolName << ": " << type.name << "\"];\n";
                dotStream << "    node" << currentNode << " -> node" << currentNode << "_" << symbolName << ";\n";
            }
            break;
        }
        default:
            nodeLabel = "UNKNOWN";
            break;
    }

    dotStream << "    node" << currentNode << " [label=\"" << nodeLabel;
    if (!nodeProps.empty()) dotStream << "\\n" << nodeProps;
    dotStream << "\"];\n";

    return "node" + std::to_string(currentNode);
}

static void writeToDOTfile(MIR* mir) {
    std::ostringstream dotStream;
    const std::string & outputFile="MIR.dot";

    dotStream << "digraph MIR {\n";
    dotStream << "    rankdir=TB; // Top-to-bottom graph direction\n";

    // Generate DOT nodes for the global scope
    generateMIRDotNode(mir->global, dotStream);

    // Generate DOT nodes for each function
    for (auto& fooEntry : mir->functions.entries) {
        MIR_Function& foo = fooEntry.second.info;
        dotStream << "    subgraph cluster_" << foo.funcName << " {\n";
        dotStream << "        label=\"Function: " << foo.funcName << "\";\n";
        generateMIRDotNode((MIR_Scope*)&foo, dotStream);
        dotStream << "    }\n";
    }

    dotStream << "}\n";

    // Write the DOT content to a file
    std::ofstream dotFile(outputFile);
    dotFile << dotStream.str();
    dotFile.close();

    std::cout << "MIR DOT file generated: " << outputFile << "\n";
}


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
    case MIR_Primitive::PRIM_JUMP: {
        MIR_Jump* jnode = (MIR_Jump*) p;

        printTabs(depth);
        std::cout << "jump: \n";
        printTabs(depth + 1);
        std::cout << "to: .L"<<jnode->jumpLabel << "\n";
        
        break;
    }    
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
            printTabs(depth + 1);
            std::cout << "symbol: " << enode->addressOf.symbol << "\n";
            break;
            break;
        }
        case MIR_Expr::EXPR_LOAD:{
            printTabs(depth + 1);
            std::cout << "size: " << enode->load.size << "\n";
            printTabs(depth + 1);
            std::cout << "offset: " << enode->load.offset << "\n";
            printTabs(depth + 1);
            std::cout << "type: " << int(enode->load.type) << "\n";
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
            printTabs(depth + 1);
            std::cout << "func name: " << enode->functionCall->funcName << "\n";
            printTabs(depth + 1);
            std::cout << "arguments: \n";
            int argNo = 0;
            for (auto &arg : enode->functionCall->arguments){
                printTabs(depth + 1);
                std::cout << "[" << argNo << "]" <<": \n";
                printMIRPrimitive(arg, depth + 2);
                argNo++;
            }
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
                "expr_imod",
                "expr_uadd",
                "expr_usub",
                "expr_umul",
                "expr_udiv",
                "expr_umod",
                "expr_fadd",
                "expr_fsub",
                "expr_fmul",
                "expr_fdiv",
                "expr_ibitwise_and",
                "expr_ibitwise_or",
                "expr_ibitwise_xor",
                "expr_logical_and",
                "expr_logical_or",
                "expr_logical_lshift",
                "expr_logical_rshift",
                "expr_arithmetic_rshift",
                "expr_icompare_lt",
                "expr_icompare_gt",
                "expr_icompare_le",
                "expr_icompare_ge",
                "expr_icompare_eq",
                "expr_icompare_neq",
                "expr_ucompare_lt",
                "expr_ucompare_gt",
                "expr_ucompare_le",
                "expr_ucompare_ge",
                "expr_ucompare_eq",
                "expr_ucompare_neq",
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
            printMIRPrimitive(enode->unary.expr, depth + 1);
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
        
        int i = 0;
        for (auto &param : foo.parameters){
            std::cout << "\t" << "[" << i << "]" << param.identifier << ": " << param.type.name << "\n";
            i++;
        }

        std::cout << "Scope: \n";
        printMIRPrimitive((MIR_Scope*) &foo, 1);
    }
}

// FOR Parser output in tree structure
static int nodeCounter = 0;
static std::string generateDotNode(Node *node, std::ostringstream &dotStream) {
    if (!node) return "";

    int currentNode = nodeCounter++;
    std::string nodeLabel;

    switch (node->tag) {
        case Node::NODE_SUBEXPR: {
            Subexpr *s = (Subexpr *)node;
            switch (s->subtag) {
                case Subexpr::SUBEXPR_BINARY_OP:
                    nodeLabel = std::string(s->binary.op.string.data, s->binary.op.string.len); // Direct conversion
                    break;
                case Subexpr::SUBEXPR_LEAF:
                    nodeLabel = std::string(s->leaf.string.data, s->leaf.string.len); // Direct conversion
                    break;
                case Subexpr::SUBEXPR_UNARY:
                    nodeLabel = std::string(s->unary.op.string.data, s->unary.op.string.len); // Direct conversion
                    break;
                case Subexpr::SUBEXPR_FUNCTION_CALL:
                    nodeLabel = std::string(s->functionCall->funcName.string.data, s->functionCall->funcName.string.len); // Direct conversion
                    break;
                case Subexpr::SUBEXPR_CAST:
                    nodeLabel = "cast";
                    break;
                case Subexpr::SUBEXPR_INITIALIZER_LIST:
                    nodeLabel = "init_list";
                    break;
                case Subexpr::SUBEXPR_RECURSE_PARENTHESIS:
                    return generateDotNode(s->inside, dotStream);
                default:
                    nodeLabel = "subexpr";
                    break;
            }
            break;
        }
        case Node::NODE_DECLARATION: {
            Declaration *d = (Declaration *)node;
            nodeLabel = "decl";
            break;
        }
        case Node::NODE_STMT_BLOCK: {
            StatementBlock *b = (StatementBlock *)node;
            nodeLabel = "block";
            break;
        }
        case Node::NODE_IF_BLOCK: {
            IfNode *i = (IfNode *)node;
            nodeLabel = "if";
            break;
        }
        case Node::NODE_WHILE: {
            WhileNode *w = (WhileNode *)node;
            nodeLabel = "while";
            break;
        }
        case Node::NODE_FOR: {
            ForNode *f = (ForNode *)node;
            nodeLabel = "for";
            break;
        }
        case Node::NODE_RETURN: {
            ReturnNode *r = (ReturnNode *)node;
            nodeLabel = "return";
            break;
        }

        default:
            nodeLabel = "node";
            break;
    }

    // Initializer for Dot File
    dotStream << "    node" << currentNode << " [label=\"" << nodeLabel << "\"];\n";

    // Yoinked from Kelp printParse
    switch (node->tag) {
        case Node::NODE_SUBEXPR: {
            Subexpr *s = (Subexpr *)node;
            switch (s->subtag) {
                case Subexpr::SUBEXPR_BINARY_OP: {
                    std::string leftNode = generateDotNode(s->binary.left, dotStream);
                    std::string rightNode = generateDotNode(s->binary.right, dotStream);
                    dotStream << "    node" << currentNode << " -> " << leftNode << ";\n";
                    dotStream << "    node" << currentNode << " -> " << rightNode << ";\n";
                    break;
                }
                case Subexpr::SUBEXPR_UNARY: {
                    std::string exprNode = generateDotNode(s->unary.expr, dotStream);
                    dotStream << "    node" << currentNode << " -> " << exprNode << ";\n";
                    break;
                }
                case Subexpr::SUBEXPR_FUNCTION_CALL: {
                    for (auto &arg : s->functionCall->arguments) {
                        std::string argNode = generateDotNode(arg, dotStream);
                        dotStream << "    node" << currentNode << " -> " << argNode << ";\n";
                    }
                    break;
                }
                case Subexpr::SUBEXPR_INITIALIZER_LIST: {
                    for (auto &val : s->initList->values) {
                        std::string valNode = generateDotNode(val, dotStream);
                        dotStream << "    node" << currentNode << " -> " << valNode << ";\n";
                    }
                    break;
                }
                case Subexpr::SUBEXPR_RECURSE_PARENTHESIS: {
                    std::string insideNode = generateDotNode(s->inside, dotStream);
                    dotStream << "    node" << currentNode << " -> " << insideNode << ";\n";
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case Node::NODE_DECLARATION: {
            Declaration *d = (Declaration *)node;
            for (auto &decl : d->decln) {
                std::string varNode = "node" + std::to_string(nodeCounter++);
                dotStream << "    " << varNode << " [label=\"" << std::string(decl.identifier.string.data, decl.identifier.string.len) << "\"];\n";
                dotStream << "    node" << currentNode << " -> " << varNode << ";\n";

                if (decl.initValue) {
                    std::string initNode = generateDotNode(decl.initValue, dotStream);
                    dotStream << "    node" << currentNode << " -> " << initNode << ";\n";
                }
            }
            break;
        }
        case Node::NODE_STMT_BLOCK: {
            StatementBlock *b = (StatementBlock *)node;
            for (auto &stmt : b->statements) {
                std::string stmtNode = generateDotNode(stmt, dotStream);
                dotStream << "    node" << currentNode << " -> " << stmtNode << ";\n";
            }
            break;
        }
        case Node::NODE_IF_BLOCK: {
            IfNode *i = (IfNode *)node;
            std::string condNode = generateDotNode(i->condition, dotStream);
            dotStream << "    node" << currentNode << " -> " << condNode << ";\n";
            std::string blockNode = generateDotNode(i->block, dotStream);
            dotStream << "    node" << currentNode << " -> " << blockNode << ";\n";
            break;
        }
        case Node::NODE_WHILE: {
            WhileNode *w = (WhileNode *)node;
            std::string condNode = generateDotNode(w->condition, dotStream);
            dotStream << "    node" << currentNode << " -> " << condNode << ";\n";
            std::string blockNode = generateDotNode(w->block, dotStream);
            dotStream << "    node" << currentNode << " -> " << blockNode << ";\n";
            break;
        }
        case Node::NODE_FOR: {
            ForNode *f = (ForNode *)node;
            std::string initNode = generateDotNode(f->init, dotStream);
            dotStream << "    node" << currentNode << " -> " << initNode << ";\n";
            std::string condNode = generateDotNode(f->exitCondition, dotStream);
            dotStream << "    node" << currentNode << " -> " << condNode << ";\n";
            std::string updateNode = generateDotNode(f->update, dotStream);
            dotStream << "    node" << currentNode << " -> " << updateNode << ";\n";
            std::string blockNode = generateDotNode(f->block, dotStream);
            dotStream << "    node" << currentNode << " -> " << blockNode << ";\n";
            break;
        }
        case Node::NODE_RETURN: {
            ReturnNode *r = (ReturnNode *)node;
            if (r->returnVal) {
                std::string retNode = generateDotNode(r->returnVal, dotStream);
                dotStream << "    node" << currentNode << " -> " << retNode << ";\n";
            }
            break;
        }
        default:
            break;
    }

    return "node" + std::to_string(currentNode);

}

static void printParseTree(Node *const current, int depth = 0,std::ostringstream *dotStream = nullptr){
    if (!current){
        return;
    }


    auto printTabs = [&](int n){
        const int tabSize = 4;
        printf("%*s ", n * tabSize, "");
    };

    printTabs(depth);
    printf("%s : {\n", NODE_TAG_STRINGS[current->tag]);

    if (dotStream) {
        generateDotNode(current, *dotStream);
    }

    switch (current->tag){
    case Node::NODE_SUBEXPR:{
        Subexpr * s = (Subexpr *)current;

        switch (s->subtag){
        case Subexpr::SUBEXPR_BINARY_OP :{
            printParseTree(s->binary.left, depth + 1);
            
            printTabs(depth + 1);
            std::cout<<"OP: " << s->binary.op.string<< "\n";
            
            printParseTree(s->binary.right, depth + 1);   
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
            std::cout<<"UNARY OP: " <<s->unary.op.string << "\n";
            printParseTree(s->unary.expr, depth + 1);
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
            std::cout<<"CAST TO: " <<dataTypePrintf(s->cast.to) << "\n";
            printParseTree(s->cast.expr, depth + 1);
            break;
        }
        case Subexpr::SUBEXPR_INITIALIZER_LIST : {
            int i = 0;
            for (auto &val : s->initList->values){
                printTabs(depth + 1);
                std::cout<< "["<< i <<"]" <<":\n";
                printParseTree(val, depth + 2);
                i++;
            }
            
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
        
        if (b->composites.count() > 0){
            printTabs(depth+1);
            std::cout<<"Struct table:\n";
            for (auto &pair : b->composites.entries){
                Composite strct = pair.second.info;
                printTabs(depth + 2);
                std::cout<<"\tStruct " << strct.compositeName.string<< "{\n";

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
