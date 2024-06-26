#include <getopt.h>
#include <omp.h>

#include <cmath>
#include <string>
#include <vector>

#include "huffman_coding.hpp"
#include "test_file.hpp"
#include "utils/bench.hpp"
#include "utils/print.hpp"

constexpr std::string file_extension = ".sloth";

void print_usage(std::string error = "") {
    print("\n{}\n{}{}\n", "Usage: sloth [COMMAND] [OPTIONS...]",
          "    It's a sloth... what did you expect?",
          error == "" ? "" : "\n    " + error);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }
    std::string command(argv[1]);
    if (command == "test") {
        if (argc < 4) {
            print_usage(
                "test requires a file name and at least 1 factor in 0.5GB");
            return EXIT_FAILURE;
        }
        std::string pathname = argv[2];
#pragma omp parallel for
        for (int i = 3; i < argc; ++i) {
            std::size_t factor = std::stoul(argv[i]);
            Bench bench;
            TestFile::generate(pathname + std::to_string(factor), 5e8 * factor);
            print("Generated {} in {}\n", factor, bench.format());
        }
    } else if (command == "zip") {
        if (argc < 3) {
            print_usage("zip requires at least 1 file name");
            return EXIT_FAILURE;
        }

        std::vector<std::string> pathnames;
        bool parallel = false;

        static struct option long_options[] = {
            {"parallel", no_argument, 0, 'p'}, {0, 0, 0, 0}};
        char c;
        optind = 2;
        while (optind < argc) {
            if ((c = getopt_long(argc, argv, "p", long_options, 0)) != -1) {
                switch (c) {
                    case 'p': {
                        parallel = true;
                        break;
                    }
                    case '?': {
                        return EXIT_FAILURE;
                    }
                }
            } else {
                pathnames.emplace_back(argv[optind]);
                ++optind;
            }
        }
        if (parallel) {
            for (const std::string& pathname : pathnames) {
                Bench bench;
                HuffmanCoding::Parallel::Processor::encode(
                    pathname, pathname + file_extension);
                print("Zipped {} in {}\n", pathname, bench.format());
            }
        } else {
            for (const std::string& pathname : pathnames) {
                Bench bench;
                HuffmanCoding::Serial::Processor::encode(
                    pathname, pathname + file_extension);
                print("Zipped {} in {}\n", pathname, bench.format());
            }
        }
    } else if (command == "unzip") {
        if (argc < 3) {
            print_usage("unzip requires at least 1 file name");
            return EXIT_FAILURE;
        }

        std::vector<std::string> pathnames;
        for (std::string& pathname : pathnames) {
            if (pathname.size() < file_extension.size() ||
                pathname.substr(pathname.size() - file_extension.size()) !=
                    file_extension) {
                print_usage("unzip requires a file with the extension " +
                            file_extension);
                return EXIT_FAILURE;
            }
        }
        bool parallel = false;

        static struct option long_options[] = {
            {"parallel", no_argument, 0, 'p'}, {0, 0, 0, 0}};
        char c;
        optind = 2;
        while (optind < argc) {
            if ((c = getopt_long(argc, argv, "p", long_options, 0)) != -1) {
                switch (c) {
                    case 'p': {
                        parallel = true;
                        break;
                    }
                    case '?': {
                        return EXIT_FAILURE;
                    }
                }
            } else {
                pathnames.emplace_back(argv[optind]);
                ++optind;
            }
        }
        if (parallel) {
            for (const std::string& pathname : pathnames) {
                Bench bench;
                HuffmanCoding::Parallel::Processor::decode(pathname, pathname);
                print("Unzipped {} in {}\n", pathname, bench.format());
            }
        } else {
            for (const std::string& pathname : pathnames) {
                Bench bench;
                HuffmanCoding::Serial::Processor::decode(pathname, pathname);
                print("Unzipped {} in {}\n", pathname, bench.format());
            }
        }
    } else {
        print_usage();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
