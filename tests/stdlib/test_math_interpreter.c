#include <string.h>
#include <io.h>

enum TokenType {
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_END,
    TOK_INVALID
};

struct Token {
    int type;   // Store enum as int for compiler compatibility
    int value;  // Only valid if type == TOK_NUMBER
};

struct ParserState {
    char *input;
    int pos;
    struct Token current_token;
};

// Lexer helpers

char peek(struct ParserState *ps) {
    return ps->input[ps->pos];
}

char advance(struct ParserState *ps) {
    return ps->input[ps->pos++];
}

void skip_whitespace(struct ParserState *ps) {
    while (ps->input[ps->pos] == ' ' || ps->input[ps->pos] == '\t') {
        ps->pos++;
    }
}

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int parse_number(struct ParserState *ps) {
    int val = 0;
    while (is_digit(peek(ps))) {
        val = val * 10 + (advance(ps) - '0');
    }
    return val;
}

struct Token get_next_token(struct ParserState *ps) {
    skip_whitespace(ps);
    char c = peek(ps);

    if (is_digit(c)) {
        struct Token t;
        t.type = TOK_NUMBER;
        t.value = parse_number(ps);
        return t;
    }

    struct Token t;
    t.value = 0;

    if (c == '+') { advance(ps); t.type = TOK_PLUS; }
    else if (c == '-') { advance(ps); t.type = TOK_MINUS; }
    else if (c == '*') { advance(ps); t.type = TOK_MUL; }
    else if (c == '/') { advance(ps); t.type = TOK_DIV; }
    else if (c == '(') { advance(ps); t.type = TOK_LPAREN; }
    else if (c == ')') { advance(ps); t.type = TOK_RPAREN; }
    else if (c == 0)   { t.type = TOK_END; }
    else { t.type = TOK_INVALID; }

    return t;
}

void eat(struct ParserState *ps, int expected_type) {
    if (ps->current_token.type == expected_type) {
        ps->current_token = get_next_token(ps);
    }
}

// Forward declaration
int expr(struct ParserState *ps);

int factor(struct ParserState *ps) {
    if (ps->current_token.type == TOK_NUMBER) {
        int val = ps->current_token.value;
        eat(ps, TOK_NUMBER);
        return val;
    } else if (ps->current_token.type == TOK_LPAREN) {
        eat(ps, TOK_LPAREN);
        int val = expr(ps);
        eat(ps, TOK_RPAREN);
        return val;
    }
    return 0;
}

int term(struct ParserState *ps) {
    int result = factor(ps);
    while (ps->current_token.type == TOK_MUL || ps->current_token.type == TOK_DIV) {
        int op = ps->current_token.type;
        eat(ps, op);
        int right = factor(ps);
        if (op == TOK_MUL) result *= right;
        if (op == TOK_DIV) result /= right;
    }
    return result;
}

int expr(struct ParserState *ps) {
    int result = term(ps);
    while (ps->current_token.type == TOK_PLUS || ps->current_token.type == TOK_MINUS) {
        int op = ps->current_token.type;
        eat(ps, op);
        int right = term(ps);
        if (op == TOK_PLUS) result += right;
        if (op == TOK_MINUS) result -= right;
    }
    return result;
}

void write_int(int n) {
    char buf[32];
    int len = 0;
    if (n == 0) {
        buf[0] = '0';
        len = 1;
    } else {
        int temp = n;
        while (temp > 0) {
            buf[len++] = '0' + (temp % 10);
            temp /= 10;
        }
        // Reverse digits
        int i;
        for (i = 0; i < len / 2; i++) {
            char t = buf[i];
            buf[i] = buf[len - 1 - i];
            buf[len - 1 - i] = t;
        }
    }
    write(1, buf, len);
    write(1, "\n", 1);
}

int main() {
    struct ParserState ps;
    char buf[1024];
    ps.input = &buf[0];
    while (1){
        int n = read(0, ps.input, 1024);
        ps.input[n] = 0;
        ps.pos = 0;
        ps.current_token = get_next_token(&ps);
        
        int result = expr(&ps);
        write_int(result);
    }

    return 0;
}
