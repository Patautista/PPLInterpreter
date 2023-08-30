#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "Interpreter.h"

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
map<string, Macro> macroFunctions; // Store macro functions

map<string, Macro> importMacrosFromFile(const string& filename, map<string, Macro> macros) {
	std::ifstream macroFile(filename);
	if (!macroFile.is_open()) {
		std::cerr << "Error: Unable to open macro definition file." << std::endl;
		return macros;
	}

	std::string line;
	while (std::getline(macroFile, line)) {
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
vector<string> extractFunctionArguments(string input) {
	std::istringstream ss(input);
	ss.ignore(input.find("(") + 1, '(');
	std::vector<std::string> arguments;

	// Read and extract arguments
	std::string argument;
	while (ss >> argument) {
		if (argument.find(")") != string::npos) {
			argument = argument.substr(0, argument.find(")"));
			arguments.push_back(argument);
			break;
		}
		arguments.push_back(argument);
	}
	return arguments;
}
class Instance {

private:
	string _filename;
	map<string, int> _variables;
	map<string, Macro> _macros;
	vector<string> _program;
	std::map<std::string, size_t> _labels;
	size_t _programCounter;
	int _inputSize;
	void _initialize() {
		_programCounter = 0;
		_inputSize = 0;
		// Read macros
		_macros = importMacrosFromFile(_filename, _macros);
		// Initialize outuput variable
		_variables["y"] = 0;

		std::ifstream inputFile(_filename);
		if (!inputFile.is_open()) {
			std::cerr << "Error: Unable to open the file." << std::endl;
		}

		std::string line;

		for (int i = 0; i < _inputSize; i++) {
			// initialize variables
			if (_variables.find("x" + to_string(i + 1)) != _variables.end()) {
				_variables["x" + to_string(i + 1)] = 0;
			}
		}

		bool program_start = false;
		// Read the file and store labels and program lines
		while (std::getline(inputFile, line)) {
			if (program_start) {
				if (!line.empty() && line[0] == '[' && line.back() == ']') {
					_labels[line.substr(1, line.size() - 2)] = _program.size();
				}
				else {
					_program.push_back(line);
				}
			}
			if (line.find("program") != string::npos) {
				istringstream iss(line);
				string syntax;
				iss >> syntax >> syntax;
				try {
					_inputSize = stoi(syntax.substr(1, syntax.size() - 2));
				}
				catch (exception e) {}
				program_start = true;
			}
		}
	}
	string _jumpToLabel(string label) {
		if (_labels.find(label) != _labels.end()) {
			_programCounter = _labels[label];
			return label;
		}
		else if (label == "e") {
			return label;
		}
		else return "";
	};
	map<string, int> _extractFunctionArguments(string input) {
		std::istringstream ss(input);
		ss.ignore(input.find("(") + 1, '(');
		std::vector<std::string> arguments;

		// Read and extract arguments
		std::string argument;
		while (ss >> argument) {
			if (argument.find(")") != string::npos) {
				argument = argument.substr(0, argument.find(")"));
				arguments.push_back(argument);
				break;
			}
			arguments.push_back(argument);
		}
		map<string, int> variables2;
		for (int i = 0; i < arguments.size(); i++) {
			// argument 0 is assigned to x1 and so forth
			variables2["x" + to_string(i + 1)] = _variables[arguments[i]];
		}
		return variables2;
	}
public:
	Instance(const string& filename, const map<string,
		int> variables, map<string, Macro> macros) : _filename(filename),
		_variables(variables), _macros(macros) {
		_initialize();
	}
	int run() {
		while (_programCounter < _program.size()) {
			istringstream iss(_program[_programCounter]);
			string command;
			std::vector<std::string> tokens(std::istream_iterator<std::string>{iss},
				std::istream_iterator<std::string>());
			if (tokens.size() < 1) {
				_programCounter++;
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
					// v <- 0
					// initialize variable
					if (operand != variable_name) {
						if (operand == "0") {
							_variables.insert(std::make_pair(variable_name, 0));
							_programCounter++;
							continue;
						}
					}
					// expressions
					std::string rightSide;
					op = tokens[3];
					if(tokens.size()>4) rightSide = tokens[4];
					if (op == "+" && rightSide == "1") {
						if (_variables.find(variable_name) != _variables.end()) {
							_variables[variable_name]++;
						}
						else {
							std::cerr << "Error: Variable " << variable_name << " not found." << std::endl;
						}
						_programCounter++;
						continue;
					}
					else if (op == "-" && rightSide == "1") {
						if (_variables.find(variable_name) != _variables.end()) {
							_variables[variable_name]--;
						}
						else {
							std::cerr << "Error: Variable " << variable_name << " not found." << std::endl;
						}
						_programCounter++;
						continue;
					}
					//infix
					else if (_macros.find(op) != _macros.end() && _macros[op].getSource() != _filename) {
						Macro macro = _macros[op];
						if (macro.getType() == Macro::Type::INFIX) {
							// Process infix macro
							// Example: v + v
							// Infix processing
							int result = 0;
							std::map<std::string, int> variables2;
							variables2["x1"] = _variables[tokens[2]];
							variables2["x2"] = _variables[tokens[4]];
							_variables[tokens[0]] = (new Instance(macro.getSource(), variables2, _macros))->run();
						}
					}
					// function
					else {
						size_t predicateNameEndIndex = tokens[2].find("(");
						// P(...)
						if (predicateNameEndIndex != string::npos) {
							// parse predicate name
							string predicateName = tokens[2].substr(0, predicateNameEndIndex);
							// check if predicate is callable
							if (_macros.find(predicateName) != _macros.end() && _macros[predicateName].getSource() != _filename && _macros[predicateName].getType() == Macro::Type::FUNCTION) {
								map<string, int> variables2 = _extractFunctionArguments(_program[_programCounter]);
								_variables[variable_name] = (new Instance(_macros[predicateName].getSource(), variables2, _macros))->run();
							}
						}
					}
				}
			}
			// if goto or P(...)
			else if (tokens[0] == "if") {
				size_t predicateNameEndIndex = tokens[1].find("(");
				// P(...)
				if (predicateNameEndIndex != string::npos) {
					// parse predicate name
					string predicateName = tokens[1].substr(0, predicateNameEndIndex);
					// check if predicate is callable
					if (_macros.find(predicateName) != _macros.end() && _macros[predicateName].getSource() != _filename) {
						map<string, int> variables2 = _extractFunctionArguments(_program[_programCounter]);
						if ((new Instance(_macros[predicateName].getSource(), variables2, _macros))->run() == 0) {
							string label = tokens[tokens.size() - 1];
							if (_jumpToLabel(label) == "e") {
								break;
							}
							continue;
						}
					}
				}
				// x != 0 predicate
				else if (_variables[tokens[1]] != 0) {
					std::string label = tokens[5];
					if (_jumpToLabel(label) == "e") {
						return _variables["y"];
					}
					continue;
				}
			}
			// return (goto e)
			else if (tokens[0] == "goto" && tokens[1] == "e") {
				break;
			}
			// applied macros
			else if (_macros.find(tokens[0]) != _macros.end() && _macros[tokens[0]].getSource() != _filename) {
				Macro macro = _macros[tokens[0]];
				if (macro.getType() == Macro::Type::INFIX) {
					// Process infix macro
					// Example: v + v
					// Infix processing
					std::map<std::string, int> variables2;
					variables2["x1"] = _variables[tokens[2]];
					variables2["x2"] = _variables[tokens[4]];
					_variables[tokens[0]] = (new Instance(macro.getSource(), variables2, _macros))->run();
				}
				else if (macro.getType() == Macro::Type::FUNCTION) {
				}
			}
			else {}
			_programCounter++;
		}
		// print variables
		if (_filename == "program.ppl") {
			// Print variable values
			for (const auto& variable : _variables) {
				std::cout << variable.first << ": " << variable.second << std::endl;
			}
		}
		return _variables["y"];
	}
};
int main() {
	std::string filename = "program.ppl"; // Change this to your .ppl file's name
	std::map<std::string, int> variables;
	std::map<std::string, Macro> macros;
	variables["x1"] = 0;
	variables["x2"] = 0;
	(new Instance(filename, variables, macros))->run();
	system("pause");
	return 0;
}