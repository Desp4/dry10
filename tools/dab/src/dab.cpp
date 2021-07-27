#include <iostream>
#include <span>

#include "dab.hpp"

static constexpr std::array<std::string_view, 4> modes{
    "new", "add", "rem", "upd"
};

static constexpr u32_t mode_new = 0;
static constexpr u32_t mode_add = 1;
static constexpr u32_t mode_rem = 2;
static constexpr u32_t mode_upd = 3;

static constexpr std::array<char, 2> new_opts{ 'o', 'p' };
static constexpr std::array<char, 1> add_opts{ 'p' };
static constexpr std::array<char, 0> rem_opts{};
static constexpr std::array<char, 1> upd_opts{ 'p' };

static constexpr std::array<std::span<const char>, 4> mode_options{
    new_opts, add_opts, rem_opts, upd_opts
};

constexpr std::integer_sequence<char, 'm', 's', 't'> rem_file_flags;
constexpr std::integer_sequence<char, 'f', 'd', 'r'> dab_file_flags;

static void iterate_directory(dab_file_paths& paths, auto dir_iterator) {
    for (const auto& entry : dir_iterator) {
        if (!entry.is_regular_file()) {
            continue;
        }

        paths.files.emplace_back(entry.path());
    }
}

template<char... Opts>
static constexpr bool match_opt_range(char target, std::integer_sequence<char, Opts...>) {
    return ((target == Opts) || ...);
}

static constexpr bool is_option(std::string_view opt) {
    return opt.size() == 2 && opt[0] == '-';
}

template<char... Opts>
static void file_arg_parsing_loop(auto func, char init, std::span<const std::string_view> args, std::integer_sequence<char, Opts...> opts) {
    char file_type = init;
    for (auto i = 0u; i < args.size(); ++i) {
        while (!(i == args.size() || is_option(args[i]))) {
            func(args[i], file_type);
            ++i;
        }

        if (i == args.size()) {
            break;
        }

        if (!match_opt_range(args[i][1], opts)) {
            throw std::runtime_error{ std::string{ "Invalid file flag " } + args[i].data() + '\n' };
        }

        file_type = args[i][1];
    }
}

static dab_file_paths parse_file_args(u32_t mode, std::span<const std::string_view> args) {
    dab_file_paths ret_paths;
    
    if (args.size() == 0) {
        return ret_paths;
    }

    if (mode == mode_rem) {
        if (!is_option(args[0]) || !match_opt_range(args[0][1], rem_file_flags)) {
            throw std::runtime_error{ std::string{ "Invalid rem usage " } + args[0].data() };
        }

        auto rem_l = [&ret_paths](std::string_view file, char rem_type) {
            ret_paths.files.emplace_back(file);
            ret_paths.rem_types.push_back(rem_type);
        };
        file_arg_parsing_loop(rem_l, args[0][1], { args.begin() + 1, args.end() }, rem_file_flags);
    } else {
        auto mode_l = [&ret_paths](std::string_view file, char rem_type) {
            if (rem_type == 'f') {
                if (!fs::is_regular_file(file)) {
                    throw std::runtime_error{ std::string{ file } + " is not a valid file" };
                }
                ret_paths.files.emplace_back(file);
            } else {
                switch (rem_type) {
                case 'd': iterate_directory(ret_paths, fs::directory_iterator{ file }); break;
                case 'r': iterate_directory(ret_paths, fs::recursive_directory_iterator{ file }); break;
                }
            }
        };

        file_arg_parsing_loop(mode_l, 'f', args, dab_file_flags);
    }
    return ret_paths;
}

int main(int argc, char** argv) {
    std::vector<std::string_view> args;
    args.reserve(argc - 1);
    for (auto i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    auto arg_iter = args.begin();

    // command line parsing
    if (args.size() == 1 && args[0] == "-h" || args[0] == "--help") {
        std::cout <<
            "Usage: dab <mode> [options...] <dab_file> [[options] <files...>...]\n" \
            "Modes:\n" \
            "\tnew - create dab file named <dab_file> and add <files> to it\n" \
            "\tadd - add <files> to existing dab file\n" \
            "\trem - remove <files> from existing dab file\n" \
            "\t\t file [options] are instead {-s|-m|-t}, denoting shader/mesh/texture files respectively\n" \
            "\tupd - same as new, but only updates the dab file if it already exists(possibly less work is done)\n" \
            "Options:\n" \
            "\t-o - overwrite dab file if already present; applicable in: new\n" \
            "\t-p - fail command if any added file is not supported; applicable in: new, add, upd\n" \
            "Files options(excluding rem):\n" \
            "\t-f - interpret <files> as filenames, default option\n" \
            "\t-d - interpret <files> as directories and fetch all eligable files from them, non-recursive\n" \
            "\t-r - same as above but recursive\n" \
            "Notes:\n" \
            "\tadding files with the same name(stripped of their extension) and same asset type will result in entiers in dab with the same name, " \
            "which will make it impossible to differentiate those files\n";
        return 0;
    }
    if (args.size() < 2) {
        std::cerr << "Invalid usage, type --help or -h for commands\n";
        return -1;
    }

    // get mode
    const auto mode_it = std::find(modes.begin(), modes.end(), *arg_iter);
    if (mode_it == modes.end()) {
        std::cerr << "Invalid mode " << *arg_iter << '\n';
        return -1;
    }
    const u32_t mode = static_cast<u32_t>(mode_it - modes.begin());
    ++arg_iter;

    // get mode options
    bool mode_overwite = false;
    bool mode_pedantic = false;
    bool mode_noclones = false;
    for (; arg_iter != (args.end() - 1); ++arg_iter) {
        const auto& arg = *arg_iter;
        if (arg[0] != '-') {
            break;
        }
        if (arg.size() != 2) {
            std::cerr << "Invalid option " << arg << '\n';
            return -1;
        }

        if (std::find(mode_options[mode].begin(), mode_options[mode].end(), arg[1]) == mode_options[mode].end()) {
            std::cerr << "Invalid or unsupported option " << arg << " for mode " << modes[mode] << '\n';
            return -1;
        }

        switch (arg[1]) {
        case 'o': mode_overwite = true; break;
        case 'p': mode_pedantic = true; break;
        case 'c': mode_noclones = true; break;
        }
    }

    dab_file_paths parsed_file_args;
    try {
        parsed_file_args = parse_file_args(mode, { arg_iter + 1, args.end() });
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << '\n';
        return -1;
    }

    try {
        switch (mode) {
        case mode_new: create_dab(*arg_iter, parsed_file_args, mode_pedantic, mode_overwite); break;
        case mode_add: add_dab(*arg_iter, parsed_file_args, mode_pedantic); break;
        case mode_upd: update_dab(*arg_iter, parsed_file_args, mode_pedantic); break;
        case mode_rem: remove_dab(*arg_iter, parsed_file_args); break;
        }
    } catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
        return -1;
    }
    return 0;
}