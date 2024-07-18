// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#include <JANA/JVersion.h>
#include <sstream>


constexpr uint64_t JVersion::GetVersionNumber() {
    return 1000000*GetMajorNumber() + 1000*minor + patch;
}

std::string JVersion::GetVersion() {
    std::stringstream ss;
    PrintVersionNumbers(ss);
    return ss.str();
}

void JVersion::PrintVersionNumbers(std::ostream& os) {
    os << major << "." << minor << "." << patch;
}

void JVersion::PrintSplash(std::ostream& os) {
    os << std::endl;
    os << "       |    \\      \\  |     \\    ___ \\   " << std::endl;
    os << "       |   _ \\      \\ |    _ \\      ) |" << std::endl;
    os << "   \\   |  ___ \\   |\\  |   ___ \\    __/" << std::endl;
    os << "  \\___/ _/    _\\ _| \\_| _/    _\\ _____|" << std::endl;
    os << std::endl;
}

void JVersion::PrintVersionDescription(std::ostream& os) {

    os << "JANA2 version:   " << JVersion::GetVersion() << " ";
    if (is_unknown) {
        os << " (unknown git status)";
    }
    else if (is_release) {
        os << " (release)";
    }
    else if (is_modified) {
        os << " (uncommitted changes)";
    }
    else {
        os << " (committed changes)";
    }
    os << std::endl;
    if (!JVersion::is_unknown) {
        os << "Commit hash:     " << JVersion::GetCommitHash() << std::endl;
        os << "Commit date:     " << JVersion::GetCommitDate() << std::endl;
    }
    os << "Install prefix:  " << JVersion::GetInstallDir() << std::endl;
    if (JVersion::HasPodio() || JVersion::HasROOT() || JVersion::HasXerces()) {
        os << "Optional deps:   ";
        if (JVersion::HasPodio()) os << "Podio ";
        if (JVersion::HasROOT()) os << "ROOT ";
        if (JVersion::HasXerces()) os << "Xerces ";
        os << std::endl;
    }
}


