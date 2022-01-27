#ifndef _PKGFS_COMMAND_HPP
#define _PKGFS_COMMAND_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/program_options.hpp>

template <typename Derived = void> class Command;

template<> class Command<void> {
    using parsed_options = boost::program_options::parsed_options;
    using variables_map = boost::program_options::variables_map;
    using create_proc = const Command *(&)(parsed_options &, variables_map &);
    using cmd_map = std::map<std::string, create_proc>;
    static cmd_map &registry() {
        static cmd_map reg;
        return reg;
    }
public:
    class Register {
    public:
        Register(const std::string &name, create_proc proc) {
            registry().insert(cmd_map::value_type(name, proc));
        }
    };
    virtual int run(const variables_map &vm) const = 0;
    static const Command *find(parsed_options &parsed,
                               variables_map &vm)
    {
        if (vm.count("command") == 0)
            throw boost::program_options::required_option("command");
        std::string cmdname = vm["command"].as<std::string>();
        cmd_map::const_iterator p = registry().find(cmdname);
        if (p == registry().end())
            throw boost::program_options::invalid_option_value(cmdname);
        return (p->second)(parsed, vm);
    }
};

template <typename Derived> class Command: public Command<void> {
private:
    using parsed_options = boost::program_options::parsed_options;
    using variables_map = boost::program_options::variables_map;
protected:
    static const Command<> *create(parsed_options &parsed, variables_map &vm) {
        namespace po = boost::program_options;
        po::options_description cmd_desc(std::string(Derived::cmd_name) +
                                         " options");
        cmd_desc.add_options()("help", "produce help message");
        po::positional_options_description cmd_pos;
        Derived::init_options(cmd_desc, cmd_pos);
        std::vector<std::string> opts =
            po::collect_unrecognized(parsed.options, po::include_positional);
        opts.erase(opts.begin());
        auto cmd_parsed = po::command_line_parser(opts);
        po::store(cmd_parsed.options(cmd_desc).positional(cmd_pos).run(), vm);
        return new Derived();
    }
};

#endif
