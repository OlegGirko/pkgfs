#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <map>

#include <boost/program_options.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>


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

class CommandHelp: public Command<> {
    using options_description = boost::program_options::options_description;
    options_description desc_;
public:
    CommandHelp(const options_description &desc): desc_{desc} {}
    int run(const boost::program_options::variables_map &) const override {
        std::cerr << desc_ << "\n";
        return 1;
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
        if (vm.count("help"))
            return new CommandHelp(cmd_desc);
        return new Derived();
    }
};

class CommandPkg: public Command<CommandPkg> {
    using options_description = boost::program_options::options_description;
    using positional_options_description =
    boost::program_options::positional_options_description;
    using variables_map = boost::program_options::variables_map;
    static Command<>::Register reg;
public:
    constexpr static const char *cmd_name = "pkg";
    static void init_options(options_description &cmd_desc,
                             positional_options_description &cmd_pos)
    {
        cmd_desc.add_options()
        ("file", "Package file to list");
        cmd_pos.add("file", 1);
    }
    int run(const variables_map &vm) const override {
        if (vm.count("file") == 0)
            throw boost::program_options::required_option("file");
        std::cerr << "Listing file " << vm["file"].as<std::string>() << "\n";
        return 0;
    }
};

Command<>::Register CommandPkg::reg{CommandPkg::cmd_name, CommandPkg::create};

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
