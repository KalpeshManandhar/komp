# C Grammar supported.

The grammar *currently* used in the parser. 

***This also may be out of date due to not being timely documented.***




```
NOTE: 
    - Literals are surrounded with double quotes "" 
    - (x)? denotes 0 or 1 occurence of x
    - (x|y) denotes either x or y
    - (x)* denotes 0 or more occurences of x
    - (x)+ denotes 1 or more occurences of x


program: 
        (declaration
        | function_definition)+
        


statement: 
        subexpr ";" 
        | declaration ";" 
        | WHILE 
        | IF 
        | FOR


subexpr : 
        primary binary_op subexpr 
        | primary

primary : 
        "("subexpr")" 
        | unary_op primary
        | identifier 
        | literal 
        | func_call

func_call:
        func_identifier "(" subexpr ( "," subexpr )* ")"

unary_op :  
        "-" | "+" | "*" | "!" | "~" | "++" | "--" | "&"

binary_op : 
        "+" | "-" | "/" | "*" | "%" | "&" | "|" | "<<" | "=" | "+=" 
        | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "^=" | "!=" 
        | ">>" | "&&" | "||" | "==" | "!=" | ">" | "<" | ">=" | "<="

declaration: 
        data_type identifier ( "=" subexpr )? ("," identifier ( "=" subexpr )? )*  

statement_block:
        "{"
            ( statement )*
        "}"

IF : 
        "if" "("condition")" statement_block
        ( "else" ( IF | statement_block ) )?

WHILE : 
        "while" "("subexpr")" statement_block

FOR : 
        "for" "("assignment";" subexpr";" assignment")" statement_block

function_definition:
        data_type identifier "("data_type identifier ( "," data_type identifier )* ) statement_block

data_type:
        type_modifiers ("int" | "float" | "double" | "char" | "struct") ("*")* 


type_modifiers:
        ("long" | "short" | "signed" | "unsigned")*


```