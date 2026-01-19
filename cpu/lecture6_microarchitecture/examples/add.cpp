// add.cpp - Simple addition example
// Compile: g++ -O0 add.cpp -o add
// View assembly: g++ -O0 -S add.cpp -o add.s
// Or: objdump -d add | less
// Try on godbolt.org to see assembly instantly

int main() {
    long a = 5;
    long b = 3;
    long c = a + b;
    return c;
}
