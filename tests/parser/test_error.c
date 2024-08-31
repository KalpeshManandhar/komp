a = (12 + 6; // missing end parenthesis

// missing terminal
a = 12 +; 

// missing terminal inside parenthesis
a = (12 * +
a = (12 * +);
a = (12 * (+12 /));


// missing semi colon
b = 12 + 5 
c = 3 / ; // <- doesnt find the missing terminal here as it skips to the semicolon

// unexpected token
+ 1
