#include "parser.h"
#include <debug/debug-print.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <c file to parse> [p]\n \t p: for parse program proper", argv[0]);
        return EXIT_FAILURE;
    }

    Tokenizer t;
    t.init();
    t.loadFileToBuffer(argv[1]);

    Arena a;
    a.init(PAGE_SIZE * 2);
    a.createFrame();

    Parser p;
    p.init(&t, &a);

    AST *ir = NULL;
    if (argc == 3 && strcmp(argv[2], "p") == 0) {
        ir = p.parseProgram();
    } else {
        ir = p.parse();
    }

    if (ir) {
        fprintf(stdout, "Parse succeeded :)\n");

        std::ostringstream dotStream;
        dotStream << "digraph AST {\n";

        for (auto &stmt : ir->global.statements) {
            printParseTree(stmt, 0, &dotStream);
        }

        for (auto &pair : ir->functions.entries) {
            std::cout << "Function: " << pair.second.identifier << "{\n";
        
            // Extract function name
            std::string functionName = std::string(pair.second.identifier.data, pair.second.identifier.len);
            int functionNode = nodeCounter++;
        
            Function *foo = &pair.second.info;
            
            std::cout << "\tReturn type: " << dataTypePrintf(foo->returnType) << "\n";
            std::cout << "\tParameters: {\n";
        
            // Buffer to store the parameters strings
            std::string paramBuffer;
        
            for (auto &param : foo->parameters) {
                std::cout << "\t\t" << param.identifier.string << " : " << dataTypePrintf(param.type) << "\n";
        
                // Append each parameter with a newline
                paramBuffer += std::string(dataTypePrintf(param.type)) + " " +
                               std::string(param.identifier.string.data, param.identifier.string.len) +
                               ", ";
            }
            // Remove the trailing comma and space
            if (!paramBuffer.empty()) {
                paramBuffer.pop_back();
                paramBuffer.pop_back();
            }
        
            std::cout << "\t}\n";
            std::cout << "\tVariadic: " << ((foo->isVariadic)? "true":"false") << "\n";

        
            // Generate DOT node using HTML label
            dotStream << "    node" << functionNode << " [shape=rect, label=<"
                      << "<B>Function:</B> " << functionName << "<BR/>"
                      << "<B>Params:</B> " << paramBuffer << "<BR/>"
                      << "<B>Return:</B> " << dataTypePrintf(foo->returnType)
                      << ">];\n";
        
            // Connect to the next node
            dotStream << "    node" << functionNode << " -> " << "node" + std::to_string(functionNode + 1) << ";\n";
        
            printParseTree(foo->block, 1, &dotStream);
            std::cout << "}\n";
        }
        

        if (ir->global.composites.count() > 0) {
            std::cout << "Struct table:\n";
            for (auto &pair : ir->global.composites.entries) {
                Composite strct = pair.second.info;
                std::cout << "\tStruct " << strct.compositeName.string << "{\n";

                for (auto &m : strct.members.entries) {
                    std::cout << "\t\t" << m.second.info.memberName.string << ": " << dataTypePrintf(m.second.info.type) << "\n";
                }
                std::cout << "\t}\n";
            }
        }

        dotStream << "}\n";

        // Write the DOT content to a file
        std::ofstream dotFile("ast.dot");
        dotFile << dotStream.str();
        dotFile.close();
    }

    a.destroyFrame();
    a.destroy();

    return p.errors;
}