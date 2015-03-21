.PHONY: coppa

coppa:
	clang++ -O0 -std=c++14 -I. -Ivariant/include -Iwebsocketpp/ -Ijeayeson/include/ test.cpp -lboost_system && ./a.out

configure:
	cd jeayeson && ./configure

