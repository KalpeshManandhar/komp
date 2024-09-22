int a, b, c;

// missing end parenthesis
a = (12 + 6; 

// missing terminal
a = 12 +; 

// missing terminal inside parenthesis
a = (12 * +
a = (12 * +); // <- skip here
a = (12 * )
a = (12 * (+12 /)); // <- skip here


// missing semi colon
b = 12 + 5 
c = 3 / ; // <- skips to semicolon

// missing semicolon
+ 1