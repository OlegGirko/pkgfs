#include <string>
#include <vector>

#include "commandpkg.hpp"

void CommandPkg::init_options(options_description &cmd_desc,
                              positional_options_description &cmd_pos)
{
    namespace po = boost::program_options;
    cmd_desc.add_options()
    ("subcommand", po::value<std::string>(), "Subcommand")
    ("subargs", po::value<std::vector<std::string>>(), "Subcommand arguments");
    cmd_pos.add("subcommand", 1).add("subargs", -1);
}

int CommandPkg::run(const variables_map &vm) const {
    namespace po = boost::program_options;
    if (vm.count("subcommand") == 0)
        throw boost::program_options::required_option("subcommand");
    std::cerr << "Running subcommand " << vm["subcommand"].as<std::string>();
    std::vector<std::string> subargs =
        vm["subargs"].as<std::vector<std::string>>();
    subargs.erase(subargs.begin());
    if (!subargs.empty()) {
        std::cerr << " with args:";
        for (auto p: subargs)
            std::cerr << " " << p;
        std::cerr << "\n";
    } else {
        std::cerr << " without args\n";
    }
    return 0;
}

Command<>::Register CommandPkg::reg{CommandPkg::cmd_name, CommandPkg::create};
