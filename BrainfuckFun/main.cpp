#include <iostream>
#include <string>
#include <cstdio>

#include "bf_compiler.hpp"
#include "transpiler.hpp"
int main()
{
	std::string code = transpiler::bx_file_to_bf("src.bx");
	bf::run(code, false);

	return 0;
}