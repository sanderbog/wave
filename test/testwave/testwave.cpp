/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2005 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// system headers
#include <string>
#include <iostream>
#include <vector>

// include boost
#include <boost/config.hpp>
#include <boost/program_options.hpp>

//  test application related headers
#include "cmd_line_utils.hpp"
#include "testwave_app.hpp"

namespace po = boost::program_options;

///////////////////////////////////////////////////////////////////////////////
//
//  The debuglevel command line parameter is used to control the amount of text 
//  printed by the testwave application. 
//
//  level 0:    prints nothing except serious failures preventing the testwave
//              executable from running, the return value of the executable is 
//              equal to the number of failed tests
//  level 1:    prints a short summary only
//  level 2:    prints the names of the failed tests only
//  level 3:    prints the outcome of every test
//  level 4:    prints the expected and real result for failed tests
//  level 5:    prints the real result for succeeded tests
//
//  The default debug level is 1.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int 
main(int argc, char *argv[])
{
    int error_count = 0;
    try {
    // analyze the command line options and arguments
        po::options_description desc_cmdline ("Options allowed on the command line");
        desc_cmdline.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print the version number")
            ("copyright,c", "print out the copyright statement")
            ("config-file", po::value<std::vector<std::string> >()->composing(), 
                "specify a config file (alternatively: @arg)")
            ("debug,d", po::value<int>(), "set the debug level (0...2)")
        ;

    // Hidden options, will be used in in config file analysis to allow to
    // recognise positional arguments, will not be shown to the user.
        po::options_description desc_hidden("Hidden options");
        desc_hidden.add_options()
            ("input", po::value<std::vector<std::string> >()->composing(), 
                "inputfile")
        ;        

    // this is the test application object
        po::variables_map vm;
        testwave_app app(vm);

    // all command line and config file options
        po::options_description cmdline_options;
        cmdline_options.add(desc_cmdline).add(app.common_options());

        po::options_description cfgfile_options;
        cfgfile_options.add(desc_cmdline).add(desc_hidden);

    // parse command line
        po::parsed_options opts(po::parse_command_line(argc, argv, 
            cmdline_options, 0, cmd_line_utils::at_option_parser));
        
        po::store(opts, vm);
        po::notify(vm);

    // if there is specified at least one config file, parse it and add the 
    // options to the main variables_map
        if (vm.count("config-file")) {
            std::vector<std::string> const &cfg_files = 
                vm["config-file"].as<std::vector<std::string> >();
            std::vector<std::string>::const_iterator end = cfg_files.end();
            for (std::vector<std::string>::const_iterator cit = cfg_files.begin(); 
                 cit != end; ++cit)
            {
            // parse a single config file and store the results
                cmd_line_utils::read_config_file(*cit, cfgfile_options, vm);
            }
        }

    // ... act as required 
        if (vm.count("help")) {
            po::options_description desc_help (
                "Usage: testwave [options] [@config-file(s)] file(s)");
            desc_help.add(desc_cmdline).add(app.common_options());
            std::cout << desc_help << std::endl;
            return 0;
        }
        
    // debug flag
        if (vm.count("debug")) {
            int debug_level = vm["debug"].as<int>();
            if (debug_level < 0 || debug_level > 9) {
                std::cerr 
                    << "testwave: please use an integer in the range [0..9] "
                    << "as the parameter to the debug option!" 
                    << std::endl;
            }
            else {
                app.set_debuglevel(debug_level);
            }
        }
        
        if (vm.count("version")) {
            return app.print_version();
        }

        if (vm.count("copyright")) {
            return app.print_copyright();
        }
        
    // iterate over all given input files 
        int input_count = 0;
        if (vm.count("input")) {
            std::vector<std::string> const &inputs = 
                vm["input"].as<std::vector<std::string> >();
            std::vector<std::string>::const_iterator end = inputs.end();
            for (std::vector<std::string>::const_iterator cit = inputs.begin(); 
                 cit != end; ++cit)
            {
                if (!app.test_a_file((*cit)))
                    ++error_count;
                ++input_count;
            }
        }

    // extract the arguments from the parsed command line
        std::vector<po::option> arguments;
        std::remove_copy_if(opts.options.begin(), opts.options.end(), 
            std::back_inserter(arguments), cmd_line_utils::is_argument());

    // iterate over remaining arguments
        std::vector<po::option>::const_iterator arg_end = arguments.end();
        for (std::vector<po::option>::const_iterator arg = arguments.begin();
             arg != arg_end; ++arg)
        {
            if (!app.test_a_file((*arg).value[0]))
                ++error_count;
            ++input_count;
        }

    // print a message if no input is given
        if (0 == input_count) {
            std::cerr 
                << "testwave: no input file specified, " 
                << "try --help to get a hint." 
                << std::endl;
        }
        else if (app.get_debuglevel() > 0) {
            std::cout 
                << "testwave: " << input_count-error_count 
                << " of " << input_count << " test(s) succeeded";
            if (0 != error_count) {
                std::cout 
                    << " (" << error_count << " test(s) failed)";
            }
            std::cout << "." << std::endl;
        }
    }
    catch (std::exception &e) {
        std::cerr << "testwave: exception caught: " << e.what() << std::endl;
        return (std::numeric_limits<int>::max)() - 1;
    }
    catch (...) {
        std::cerr << "testwave: unexpected exception caught." << std::endl;
        return (std::numeric_limits<int>::max)() - 2;
    }

    return error_count;
}
