flex src/tokens.l
bison src/grammar.y

g++ --std=c++11 -Wall src/parser.cc src/scanner.cc src/expression.cc -o dusk
