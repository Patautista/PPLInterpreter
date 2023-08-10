#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

std::map<std::string, int> variables;
std::map<std::string, size_t> labels;

void executePPL(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open the file." << std::endl;
        return;
    }

    std::string line;
    std::vector<std::string> program;

    // Read the file and store labels and program lines
    while (std::getline(inputFile, line)) {
        if (!line.empty() && line[0] == '[' && line.back() == ']') {
            labels[line.substr(1, line.size() - 2)] = program.size();
        }
        else {
            program.push_back(line);
        }
    }

    size_t programCounter = 0;
    while (programCounter < program.size()) {
        std::istringstream iss(program[programCounter]);
        std::string command;
        iss >> command;
        // v < - ...
        if (std::isalpha(command[0]) && command != "if") {
            std::string variable_name = command;
            std::string right_side;
            std::string op;
            iss >> op;
            if (op == "<-") {
                iss >> right_side;
                // initialize variable
                if (right_side != variable_name) {
                    if (right_side == "0") {
                        variables.insert(std::make_pair(variable_name, 0));
                    }
                    else {
                        std::cerr << "Invalid operation." << std::endl;
                    }
                }
                iss >> op;
                if (op == "+") {
                    if (variables.find(variable_name) != variables.end()) {
                        variables[variable_name]++;
                    }
                    else {
                        std::cerr << "Error: Variable " << variable_name << " not found." << std::endl;
                    }
                }
                else if (op == "-") {
                    if (variables.find(variable_name) != variables.end()) {
                        variables[variable_name]--;
                    }
                    else {
                        std::cerr << "Error: Variable " << variable_name << " not found." << std::endl;
                    }
                }
            }
        }

        else if (command == "if") {
            std::string varName;
            iss >> varName;
            if (variables[varName] != 0) {
                std::string label;
                iss >> label;
                iss >> label;
                iss >> label;
                iss >> label;
                if (labels.find(label) != labels.end()) {
                    programCounter = labels[label];
                    continue;
                }
            }
        }

        programCounter++;
    }
}

int main() {
    std::string filename = "program.ppl"; // Change this to your .ppl file's name
    executePPL(filename);

    // Print variable values
    for (const auto& variable : variables) {
        std::cout << variable.first << ": " << variable.second << std::endl;
    }
    system("pause");
    return 0;
}
