#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

using namespace std;

class Macro {
public:
	enum class Type { INFIX, FUNCTION };

	Macro(Type macroType, const std::string& macroSource) : type(macroType), source(macroSource) {}
	Macro() : type(Macro::Type::FUNCTION), source("") {}
	Type getType() const {
		return type;
	}
	const std::string& getSource() const {
		return source;
	}

private:
	Type type;
	std::string content;
	std::string source;
};

// Macros are global
std::map<std::string, Macro> macroFunctions; // Store macro functions

std::map<std::string, Macro> importMacrosFromFile(const std::string& filename, std::map<std::string, Macro> macros) {
	std::ifstream macroFile(filename);
	if (!macroFile.is_open()) {
		std::cerr << "Error: Unable to open macro definition file." << std::endl;
		return macros;
	}

	std::string line;
	while (std::getline(macroFile, line) ) {
		// parse macro
		istringstream iss(line);
		string macroName;
		string macroType;
		string macroSourceFilename;

		iss >> macroName >> macroType >> macroSourceFilename;
		if (macroName == "program") {
			break;
		}

		ifstream sourceFile(macroSourceFilename);
		if (!sourceFile.is_open()) {
			//std::cerr << "Error: Unable to open macro source file." << std::endl;
			return macros;
		}
		if (macroType == "infix") {
			macros[macroName] = Macro(Macro::Type::INFIX, macroSourceFilename);
			macros = importMacrosFromFile(macroSourceFilename, macros);
		}
		else if (macroType == "function") {
			macros[macroName] = Macro(Macro::Type::FUNCTION, macroSourceFilename);
			macros = importMacrosFromFile(macroSourceFilename, macros);
		}
		else {
			std::cerr << "Error: Unknown macro type." << std::endl;
		}
	}
	return macros;
}
std::vector<std::string> extractFunctionArguments(string input) {
	std::istringstream ss(input);
	ss.ignore(input.find("(")+1, '(');
	std::vector<std::string> arguments;

	// Read and extract arguments
	std::string argument;
	while (ss >> argument ) {
		arguments.push_back(argument);
	}
	// remove parenthesis from last argument
	if (arguments[arguments.size() - 1].find(")") != string::npos) {
		arguments[arguments.size() - 1] = arguments[arguments.size() - 1].substr(0, arguments[arguments.size() - 1].find(")"));
	}

	return arguments;
}
int runInstance(const std::string& filename, std::map<std::string, int> variables) {

	// Read macros
	std::map<std::string, Macro> macros;
	macros = importMacrosFromFile(filename, macros);

	std::map<std::string, size_t> labels;
	// Initialize outuput variable
	variables["y"] = 0;

	std::ifstream inputFile(filename);
	if (!inputFile.is_open()) {
		std::cerr << "Error: Unable to open the file." << std::endl;
		return 0;
	}

	int inputs = 0;

	std::string line;
	std::vector<std::string> program;

	bool program_start = false;
	// Read the file and store labels and program lines
	while (std::getline(inputFile, line)) {
		if (program_start) {
			if (!line.empty() && line[0] == '[' && line.back() == ']') {
				labels[line.substr(1, line.size() - 2)] = program.size();
			}
			else {
				program.push_back(line);
			}
		}
		if (line.find("program") != string::npos) {
			istringstream iss(line);
			string syntax;
			iss >> syntax >> syntax;
			try {
				inputs = stoi(syntax.substr(1, syntax.size() - 2));
			}
			catch (exception e) {}
			program_start = true;
		}
	}

	size_t programCounter = 0;

	// main loop
	while (programCounter < program.size()) {
		istringstream iss(program[programCounter]);
		string command;
		std::vector<std::string> tokens(std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>());
		if (tokens.size() < 1) {
			programCounter++;
			continue;
		}
		else if (std::isalpha(tokens[0][0]) && tokens[0] != "if" && tokens[0] != "goto") {
			// Handle variable operations
			std::string variable_name = tokens[0];
			std::string operand;
			std::string op;
			op = tokens[1];
			// assignment
			if (op == "<-") {
				operand = tokens[2];
				// initialize variable
				if (operand != variable_name) {
					if (operand == "0") {
						variables.insert(std::make_pair(variable_name, 0));
						programCounter++;
						continue;
					}
				}
				// expressions
				std::string rightSide;
				iss >> op >> rightSide;
				op = tokens[3];
				rightSide = tokens[4];
				if (op == "+" && rightSide == "1") {
					if (variables.find(variable_name) != variables.end()) {
						variables[variable_name]++;
					}
					else {
						std::cerr << "Error: Variable " << variable_name << " not found." << std::endl;
					}
					programCounter++;
					continue;
				}
				else if (op == "-" && rightSide == "1") {
					if (variables.find(variable_name) != variables.end()) {
						variables[variable_name]--;
					}
					else {
						std::cerr << "Error: Variable " << variable_name << " not found." << std::endl;
					}
					programCounter++;
					continue;
				}
				// Handle macros and variable operations
				iss >> command >> command >> command;
				//infix
				if (macros.find(op) != macros.end() && macros[op].getSource() != filename) {
					Macro macro = macros[op];
					if (macro.getType() == Macro::Type::INFIX) {
						// Process infix macro
						// Example: v + v
						// Infix processing
						int result = 0;
						std::map<std::string, int> variables2;
						variables2["x1"] = variables[tokens[2]];
						variables2["x2"] = variables[tokens[4]];
						result = runInstance(macro.getSource(), variables2);
						variables[tokens[0]] = result;
					}
					else if (macro.getType() == Macro::Type::FUNCTION) {
					}
				}
				else {}
			}
		}
		// if goto or P(...)
		else if (tokens[0] == "if") {
			int predicateNameEndIndex = tokens[1].find("(");
			if (predicateNameEndIndex != string::npos) {
				// parse predicate
				string predicateName = tokens[1].substr(0, predicateNameEndIndex);
				// check if predicate is callable
				if (macros.find(predicateName) != macros.end() && macros[predicateName].getSource() != filename) {
					vector<string> arguments = {};
					arguments = extractFunctionArguments(program[programCounter]);
					map<string, int> variables2;
					for (int i = 0; i < arguments.size(); i++) {
						// argument 0 is assigned to x1 and so forth
						variables2["x" + to_string(i+1)] = variables[arguments[i]];
					}
					auto result = runInstance(macros[predicateName].getSource(), variables2);
					variables["y"] = result;
				}
			}
			else if (variables[tokens[1]] != 0) {
				std::string label = tokens[5];
				if (label == "e") {
					return variables["y"];
				}
				if (labels.find(label) != labels.end()) {
					programCounter = labels[label];
					continue;
				}
			}
		}
		// return goto
		else if (tokens[0] == "goto" && tokens[1] == "e") {
			return variables["y"];
		}
		else if (macros.find(tokens[0]) != macros.end() && macros[tokens[0]].getSource() != filename) {
			Macro macro = macros[tokens[0]];
			if (macro.getType() == Macro::Type::INFIX) {
				// Process infix macro
				// Example: v + v
				// Infix processing
				int result = 0;
				std::map<std::string, int> variables2;
				variables2["x1"] = variables[tokens[2]];
				variables2["x2"] = variables[tokens[4]];
				result = runInstance(macro.getSource(), variables2);
				variables[tokens[0]] = result;
			}
			else if (macro.getType() == Macro::Type::FUNCTION) {
			}
		}
		else {}
		programCounter++;
	}
	if (filename == "program.ppl") {
		// Print variable values
		for (const auto& variable : variables) {
			std::cout << variable.first << ": " << variable.second << std::endl;
		}
	}
	return variables["y"];
}

int main() {
	std::string filename = "program.ppl"; // Change this to your .ppl file's name
	std::map<std::string, int> variables;
	variables["x1"] = 0;
	variables["x2"] = 0;
	runInstance(filename, variables);
	system("pause");
	return 0;
}