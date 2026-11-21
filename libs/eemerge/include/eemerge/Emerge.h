//
// Created by epax on 21.11.26.
//

#pragma once
#include <string>
#include <list>

namespace emgr {
    //central Emerge class for the emerge lib
    class Emerge {
    public:
        Emerge(std::string pw) = default;
        ~Emerge();

        //emerge functions
        bool EmergeInstalled();
        bool EmergeSync();
        bool EmergeUpdateSystem();
        bool EmergeUpdatePackage(std::string const& package);
        bool EmergeInstallPackage(std::string const& package);
        std::list<std::string> EmergeGetPackages();
    private:
        



        //variables
        std::string pw_;
        std::list<std::string> packagesInstalling_;
    };
} // emgr

