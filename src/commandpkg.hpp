#ifndef _PKGFS_COMMANDPKG_HPP
#define _PKGFS_COMMANDPKG_HPP

#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "command.hpp"

class CommandPkg: public Command<CommandPkg> {
    using options_description = boost::program_options::options_description;
    using positional_options_description =
    boost::program_options::positional_options_description;
    using variables_map = boost::program_options::variables_map;
    static Command<>::Register reg;
public:
    constexpr static const char *cmd_name = "pkg";
    static void init_options(options_description &cmd_desc,
                             positional_options_description &cmd_pos);
    int run(const variables_map &vm) const override;
};

#endif

