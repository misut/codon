import std;
import commands;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::print("{}", commands::usage_text());
        return 1;
    }

    std::string_view cmd{argv[1]};

    if (cmd == "version" || cmd == "--version" || cmd == "-v")
        return commands::cmd_version();
    if (cmd == "help" || cmd == "--help" || cmd == "-h")
        return commands::cmd_help();
    if (cmd == "build")  return commands::cmd_build(argc, argv);
    if (cmd == "check")  return commands::cmd_check(argc, argv);
    if (cmd == "init")   return commands::cmd_init(argc, argv);

    std::println(std::cerr, "codon: unknown command '{}'", cmd);
    std::print(std::cerr, "{}", commands::usage_text());
    return 1;
}
