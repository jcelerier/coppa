configure:
	cd jeayeson && ./configure

coppa:
	clang++ -O0 -std=c++14 -Ivariant/include -Iwebsocketpp/ -Ijeayeson/include/ test.cpp && ./a.out
