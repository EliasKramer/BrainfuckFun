#include "transpiler.hpp"
#include <vector>
#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>

static void read_file_lines(const std::string& filename, std::vector<std::string>& result) {
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
	}

	std::string line;
	while (std::getline(file, line)) {
		result.push_back(line);
	}

	file.close();
}

static void split_line(const std::string& line, std::vector<std::string>& result, char splitter)
{
	std::string current;

	for (char c : line) {
		if (c == splitter) {
			if (!current.empty()) {
				result.push_back(current);
				current.clear();
			}
		}
		else {
			current += c;
		}
	}

	if (!current.empty()) {
		result.push_back(current);
	}
}

class bx_variable {
public:
	std::string name;
	char value;
	int pointer;
	bx_variable(std::string name, int value, int pointer) : name(name), value(value), pointer(pointer) {}
};

static void move_pointer(int& current_pointer, int target_pointer, std::string& generated_code)
{
	if (current_pointer < target_pointer)
	{
		for (int i = 0; i < target_pointer - current_pointer; i++)
		{
			generated_code += ">";
		}
	}
	else if (current_pointer > target_pointer)
	{
		for (int i = 0; i < current_pointer - target_pointer; i++)
		{
			generated_code += "<";
		}
	}
	current_pointer = target_pointer;
}

static void set_current_to_zero(std::string& generated_code)
{
	generated_code += "[-]";
}
static void set_current_to_value(int value, std::string& generated_code)
{
	if (value > 127 || value < -128)
	{
		std::cerr << "Error: Value out of range" << std::endl;
	}
	if (value == 0)
	{
		return;
	}
	int curr_val = 0;
	if (value > 10)
	{
		int sqrt = std::sqrt(value);

		//go one to right of current pointer
		//this is allowed, since a variable has always 3 spaces occupied
		generated_code += ">";

		//set the value to the square root of the value
		for (int i = 0; i < sqrt; i++)
		{
			generated_code += "+";
		}

		//go back to the current pointer
		generated_code += "[<";
		for (int i = 0; i < sqrt; i++)
		{
			//add the square root to the current pointer
			generated_code += "+";
		}
		generated_code += ">-]<";
		curr_val = sqrt * sqrt;
	}

	while (curr_val != value)
	{
		if (curr_val < value)
		{
			generated_code += "+";
			curr_val++;
		}
		else if (curr_val > value)
		{
			generated_code += "-";
			curr_val--;
		}
	}
}

static void fill_current_copy_space(std::string& generated_code)
{
	//first of all delete the current values of the two right neighbours
	//if they are negative it should still work, because of overflow
	//first loop fills the two right most fields with the value of the current cell
	//this sets the current value to zero tho
	//the second loop moves the second value back to the first
	generated_code += ">[-]>[-]<<[>+>+<<-]>[-<+>]<";
}

static void move_value(int curr_pointer, int to_pointer, std::string& generated_code)
{
	int start_point = curr_pointer;
	move_pointer(curr_pointer, to_pointer, generated_code);
	set_current_to_zero(generated_code); //set the value of the variable we want to copy to, to zero
	move_pointer(curr_pointer, start_point, generated_code);
	generated_code += "[";
	move_pointer(curr_pointer, to_pointer, generated_code);
	generated_code += "+";
	move_pointer(curr_pointer, start_point, generated_code);
	generated_code += "-]";
}

static bx_variable& get_variable_by_name(std::string& name, std::vector<bx_variable>& variables)
{
	for (int i = 0; i < variables.size(); i++)
	{
		if (variables[i].name == name)
		{
			return variables[i];
		}
	}
	throw std::runtime_error("Variable not found");
}

static void move_current_pointer_to_var(std::string& generated_code, int& current_pointer, std::string& var_name, std::vector<bx_variable>& variables)
{
	move_pointer(
		current_pointer,
		get_variable_by_name(var_name, variables).pointer,
		generated_code);
}
static void move_var_to_var(int& curr_pointer, std::string& curr_var, std::string& to_var, std::string& generated_code, std::vector<bx_variable>& variables)
{
	move_current_pointer_to_var(generated_code, curr_pointer, curr_var, variables);
	move_value(curr_pointer, get_variable_by_name(to_var, variables).pointer, generated_code);
}

static void copy_var_to_var(std::string& from_var_name, std::string& to_var_name, int& current_pointer, std::string& generated_code, std::vector<bx_variable>& variables)
{
	move_current_pointer_to_var(generated_code, current_pointer, from_var_name, variables);
	fill_current_copy_space(generated_code);
	current_pointer += 2;
	generated_code += ">>";
	move_value(current_pointer, get_variable_by_name(to_var_name, variables).pointer, generated_code);
}

static void add_var(std::string& name, int value, int& current_pointer, int& new_var_location, int var_space, std::string& generated_code, std::vector<bx_variable>& variables)
{
	move_pointer(current_pointer, new_var_location, generated_code);
	variables.push_back(bx_variable(name, value, current_pointer));
	//generated_code += "[-]";
	set_current_to_value(value, generated_code);
	new_var_location += var_space;
}

static void destructive_add(std::string& generated_code, int var_space, int& current_pointer)
{
	generated_code += "[";
	for (int i = 0; i < var_space * 2; i++)
	{
		generated_code += ">";
	}
	generated_code += "+";
	for (int i = 0; i < var_space * 2; i++)
	{
		generated_code += "<";
	}
	generated_code += "-]";

	for (int i = 0; i < var_space; i++)
	{
		generated_code += ">";
	}
	generated_code += "[";
	for (int i = 0; i < var_space; i++)
	{
		generated_code += ">";
	}
	generated_code += "+";
	for (int i = 0; i < var_space; i++)
	{
		generated_code += "<";
	}
	generated_code += "-]";

	current_pointer += var_space;
}

static void remove_variable_from_list(std::string& var_name, std::vector<bx_variable> variables)
{
	for (int i = 0; i < variables.size(); i++)
	{
		if (variables[i].name == var_name)
		{
			variables.erase(variables.begin() + i);
			return;
		}
	}
	throw std::runtime_error("Variable not found");

}

std::string transpiler::bx_file_to_bf(std::string bx_filename)
{
	std::vector<std::string> raw_lines;
	read_file_lines(bx_filename, raw_lines);

	std::string generated_code = "";
	//read from file

	std::vector<bx_variable> variables;

	int current_pointer = 0;
	int new_var_location = 0;
	int var_space = 3;
	for (std::string& line : raw_lines)
	{
		std::vector<std::string> tokens;
		split_line(line, tokens, ' ');

		//num var_name value
		if (tokens[0] == "num")
		{
			if (tokens.size() != 3)
			{
				std::cerr << "Error: Invalid variable declaration\n";
				continue;
			}

			add_var(tokens[1], std::stoi(tokens[2]), current_pointer, new_var_location, var_space, generated_code, variables);
		}
		//print var_name
		else if (tokens[0] == "print")
		{
			if (tokens.size() != 2)
			{
				std::cerr << "Error: Invalid print statement\n";
				continue;
			}

			move_current_pointer_to_var(generated_code, current_pointer, tokens[1], variables);
			generated_code += ".";
		}
		//copy var_name_from to var_name_to
		else if (tokens[0] == "copy")
		{
			if (tokens.size() != 4)
			{
				std::cerr << "Error: Invalid copy statement\n";
				continue;
			}

			copy_var_to_var(tokens[1], tokens[3], current_pointer, generated_code, variables);
		}
		//set var_name value
		else if (tokens[0] == "set")
		{
			if (tokens.size() != 3)
			{
				std::cerr << "Error: Invalid set statement\n";
				continue;
			}

			move_current_pointer_to_var(generated_code, current_pointer, tokens[1], variables);
			set_current_to_zero(generated_code);
			set_current_to_value(std::stoi(tokens[2]), generated_code);
		}
		//add x y to z
		else if (tokens[0] == "add")
		{
			if (tokens.size() != 5)
			{
				std::cerr << "Error: Invalid add statement\n";
				continue;
			}

			std::string register1 = "bx_register1";
			std::string register2 = "bx_register2";
			std::string register3 = "bx_register3";

			int new_var_location_before = new_var_location;
			//generated_code += "addreg1";
			add_var(register1, 0, current_pointer, new_var_location, var_space, generated_code, variables);
			//generated_code += "addreg2";
			add_var(register2, 0, current_pointer, new_var_location, var_space, generated_code, variables);
			//generated_code += "addreg3";
			add_var(register3, 0, current_pointer, new_var_location, var_space, generated_code, variables);
			//generated_code += "cpy arg 1 to reg 1";
			copy_var_to_var(tokens[1], register1, current_pointer, generated_code, variables);
			//generated_code += "cpy arg 2 to reg 2";
			copy_var_to_var(tokens[2], register2, current_pointer, generated_code, variables);
			//generated_code += "move pointer to reg 1";
			move_current_pointer_to_var(generated_code, current_pointer, register1, variables);
			//generated_code += "destructive add";
			destructive_add(generated_code, var_space, current_pointer);
			//generated_code += "move to result var";
			move_current_pointer_to_var(generated_code, current_pointer, tokens[4], variables);
			//generated_code += "result set 0";
			set_current_to_zero(generated_code);
			//generated_code += "copy res reg to res var";
			move_var_to_var(current_pointer, register3, tokens[4], generated_code, variables);

			//just removing these variables is fine
			//because all are set to 0 after this operation
			remove_variable_from_list(register1, variables);
			remove_variable_from_list(register2, variables);
			remove_variable_from_list(register3, variables);
			new_var_location = new_var_location_before;
		}
	}
	std::cout << generated_code << std::endl;
	return generated_code;
}
