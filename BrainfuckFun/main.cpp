#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
int main()
{
	bool debug = false;
	std::string input = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.";
	char cells[1 << 15] = { 0 };
	char* data_pointer = &cells[0];

	int instruction_counter = 0;
	
	int max_i = INT_MIN;
	int min_i = INT_MAX;
	for (int i = 0; i < input.size(); i++)
	{
		instruction_counter++;

		if (instruction_counter == 64)
		{
			int br = 1;
		}

		switch (input[i])
		{
		case ',':
			scanf_s("%c", data_pointer, 1);
			break;
		case '.':
			printf("%c", (*data_pointer));
			break;
		case '+':
			(*data_pointer)++;
			break;
		case '-':
			(*data_pointer)--;
			break;
		case '>':
			data_pointer++;
			break;
		case '<':
			data_pointer--;
			break;
		case '[':
			if (*data_pointer == 0)
			{
				i++;
				//jump to command after next ]
				int ignore_points = 0;
				while (1)
				{
					if (input[i] != ']' && ignore_points == 0)
						break;

					if (input[i] == ']')
					{
						ignore_points--;
					}
					if (input[i] == '[')
					{
						ignore_points++;
					}
					i++;
				}
			}
			break;
		case ']':
			if (*data_pointer != 0)
			{
				i--;
				//jump to command after previous [
				int ignore_points = 0;
				while (1)
				{
					if (input[i] == '[' && ignore_points == 0)
						break;

					if (input[i] == ']')
					{
						ignore_points++;
					}
					if (input[i] == '[')
					{
						ignore_points--;
					}
					i--;
				}
			}
			break;
		default:
			std::cerr << "unkown command " << input[i] << "\n";
			break;
		}

		if (data_pointer - cells > max_i)
		{
			max_i = data_pointer - cells;
		}
		if (data_pointer - cells < min_i)
		{
			min_i = data_pointer - cells;
		}

		if (debug)
		{
			std::cout << "i: " << i
				<< " instr: " << input[i]
				<< " [";
			for (int x = min_i; x <= max_i; x++)
			{
				bool is_on_pointer = x == data_pointer - cells;
				if (is_on_pointer)
				{
					std::cout << "\033[1;31m";
				}
				std::cout << (int)cells[x] << " ";
				if (is_on_pointer)
				{
					std::cout << "\033[0m";
				}
			}
			std::cout << "] ";

			for (int x = 0; x < input.size(); x++)
			{
				bool is_on_pointer = x == i;
				if (is_on_pointer)
				{
					std::cout << "\033[1;31m";
				}
				std::cout << input[x];
				if (is_on_pointer)
				{
					std::cout << "\033[0m";
				}
			}
			std::cout << "\n";
		}
	}

	return 0;
}