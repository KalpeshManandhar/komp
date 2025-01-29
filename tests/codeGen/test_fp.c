int main() {
    float a = 10.265;   // a = 10.0f
    float b = 2.0f;    // b = 2.0f
    float c = -5;   // c = -5.0f
    float d = 0;    // d = 0.0f

    float sum = a + b;           // 10.0 + 2.0
    float difference = a - b;    // 10.0 - 2.0
    float product = a * c;       // 10.0 * -5.0
    float quotient = a / b;      // 10.0 / 2.0

    int result = 0;
    if (sum == (b + a)) {
        result = result + 1;
    }
    if (difference == -(b - a)) {
        result = result + 2;
    }
    if (product == (c * a)) {
        result = result + 4;
    }
    if (quotient * b == a) {
        result = result + 8;
    }

    if (a > b) {
        result = result + 16;
    }
    if (b < a) {
        result = result + 32;
    }
    if (d == d) {
        result = result + 64;
    }
    if (d != c) {
        result = result + 128;
    }

    // 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 = 255
    return result;
}
