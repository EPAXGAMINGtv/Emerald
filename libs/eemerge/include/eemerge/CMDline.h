//
// Created by epax on 21.11.26.
//

#pragma once
#include <string>

//utility file for emerge libary

namespace emrg {
    class CMDline {
    public:
        CMDline(std::string & workingDirectory);
    private:
        std::string & workingDir_;
    };
} // emrg

