#ifndef _PKGFS_COMMANDHELP_HPP
#define _PKGFS_COMMANDHELP_HPP

#include <boost/program_options.hpp>

#include "command.hpp"

class CommandHelp: public Command<> {
    using options_description = boost::program_options::options_description;
    using positional_options_description =
    boost::program_options::positional_options_description;
    using parsed_options = boost::program_options::parsed_options;
    using variables_map = boost::program_options::variables_map;
    options_description desc_;
    static Command<>::Register reg;
public:
    CommandHelp(const options_description &desc): desc_{desc} {}
    static const Command<> *create(parsed_options &parsed, variables_map &vm);   
    int run(const boost::program_options::variables_map &) const override;
};

#endif

