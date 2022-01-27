#include "commandhelp.hpp"

const Command<> *CommandHelp::create(parsed_options &parsed, variables_map &vm)
{
    return new CommandHelp(*parsed.description);
}

int CommandHelp::run(const variables_map &) const {
    std::cerr << desc_ << "\n";
    return 1;
}

Command<>::Register CommandHelp::reg{"help", CommandHelp::create};
