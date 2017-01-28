#include "commandpkg.hpp"

void CommandPkg::init_options(options_description &cmd_desc,
                              positional_options_description &cmd_pos)
{
    cmd_desc.add_options()
    ("file", "Package file to list");
    cmd_pos.add("file", 1);
}

int CommandPkg::run(const variables_map &vm) const {
    if (vm.count("file") == 0)
        throw boost::program_options::required_option("file");
    std::cerr << "Listing file " << vm["file"].as<std::string>() << "\n";
    return 0;
}

Command<>::Register CommandPkg::reg{CommandPkg::cmd_name, CommandPkg::create};
