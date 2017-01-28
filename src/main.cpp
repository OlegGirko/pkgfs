#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include <boost/program_options.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "command.hpp"

int main(int argc, char **argv) try {
    namespace po = boost::program_options;
    po::options_description global_opt("Global options");
    global_opt.add_options()
    ("help", "produce help message")
    ("command", po::value<std::string>(), "command to execute")
    ("subargs", po::value<std::vector<std::string> >(),
     "Arguments for command");
    po::positional_options_description pos;
    pos.add("command", 1).add("subargs", -1);
    po::variables_map vm;
    po::parsed_options parsed = po::command_line_parser(argc, argv).
    options(global_opt).
    positional(pos).
    allow_unregistered().
    run();
    po::store(parsed, vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cerr << global_opt << "\n";
        return 1;
    }
    std::unique_ptr<const Command<>> cmd{Command<>::find(parsed, vm)};
    return cmd->run(vm);
} catch (const boost::exception &e) {
    std::cerr << boost::diagnostic_information(e) << std::endl;
    return 1;
} catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
} catch (...) {
    std::cerr << "Unknown exception!" << std::endl;
    return 1;
}
