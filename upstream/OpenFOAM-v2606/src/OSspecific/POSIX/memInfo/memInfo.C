/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2016-2025 OpenCFD Ltd.
    Copyright (C) 2026 Keysight Technologies
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "memInfo.H"
#include "IOstreams.H"
#include "OSspecific.H"  // For pid()

#include <cstdlib>
#include <fstream>
#include <string>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#include <sys/resource.h>  // For getrusage()

// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

static int is_supported(-1);

bool Foam::memInfo::supported()
{
    if (is_supported < 0)
    {
        #ifdef __linux__
        // This is Linux-specific!
        std::ifstream is("/proc/meminfo");
        is_supported = is.good();
        #else
        // POSIX getrusage is universally available
        struct rusage usage;
        is_supported = (::getrusage(RUSAGE_SELF, &usage) == 0);
        #endif
    }

    return is_supported;
}


// Wrapper to get process memory using POSIX getrusage()
// and normalize the units.
inline static int64_t get_rusage_maxrss()
{
    if
    (
        struct rusage usage;
        ::getrusage(RUSAGE_SELF, &usage) == 0
    )
    {
        // Normalize values to kB for consistency
        #ifdef __linux__
        return (usage.ru_maxrss);         // Linux: ru_maxrss already in kB
        #else
        return (usage.ru_maxrss / 1024);  // BSD/MacOS: ru_maxrss is bytes
        #endif
    }
    else
    {
        return 0;
    }
}


int64_t Foam::memInfo::get_hwm()
{
    return get_rusage_maxrss();
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::memInfo::memInfo()
:
    hwm_(0),
    peak_(0),
    size_(0),
    rss_(0),
    free_(0)
{
    populate();
}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

bool Foam::memInfo::good() const noexcept
{
    return peak_ > 0;
}


void Foam::memInfo::clear() noexcept
{
    hwm_= peak_ = size_ = rss_ = free_ = 0;
}


void Foam::memInfo::populate()
{
    #ifdef __linux__
    std::string line;

    // This is all Linux-specific!

    // "/proc/meminfo"
    // ===========================
    // MemTotal:       65879268 kB
    // MemFree:        51544256 kB
    // MemAvailable:   58999636 kB
    // Buffers:            2116 kB
    // ...
    // Stop parsing when known keys have been extracted

    if (std::ifstream is("/proc/meminfo"); is.good())
    {
        for
        (
            unsigned nkeys = 1;
            nkeys && is.good() && std::getline(is, line);
            /*nil*/
        )
        {
            const auto delim = line.find(':');
            if (delim == std::string::npos)
            {
                continue;
            }

            // Compare initial part of line to "Key"
            #undef  isKeyEqual
            #define isKeyEqual(Key) (!line.compare(0, delim, Key))

            // std::stol() and std::strtol()
            // both skip whitespace before using as many digits as possible.
            // So just skip over the ':' and let those do the rest

            const char * value = (line.data() + (delim+1));
            char *endptr = nullptr;

            #undef  parseValue
            #define parseValue (std::strtol(value, &endptr, 10))
            // Could also check for 'kB' etc ending

            // ------------------
            // Extract key: value
            // ------------------

            if (isKeyEqual("MemFree"))
            {
                free_ = parseValue;
                --nkeys;
            }

            #undef isKeyEqual
            #undef parseValue
        }
    }

    // "/proc/PID/status"
    // ===========================
    // VmPeak:    15920 kB
    // VmSize:    15916 kB
    // VmLck:         0 kB
    // VmPin:         0 kB
    // VmHWM:      6972 kB
    // VmRSS:      6972 kB
    // ...
    // Stop parsing when known keys have been extracted

    // These units are kibi-btyes (1024)
    if
    (
        std::ifstream is("/proc/" + std::to_string(Foam::pid()) + "/status");
        is.good()
    )
    {
        for
        (
            unsigned nkeys = 4;
            nkeys && is.good() && std::getline(is, line);
            /*nil*/
        )
        {
            const auto delim = line.find(':');
            if (delim == std::string::npos)
            {
                continue;
            }

            // Compare initial part of line to "Key"
            #undef  isKeyEqual
            #define isKeyEqual(Key) (!line.compare(0, delim, Key))

            // std::stol() and std::strtol()
            // both skip whitespace before using as many digits as possible.
            // So just skip over the ':' and let those do the rest

            const char * value = (line.data() + (delim+1));
            char *endptr = nullptr;

            #undef  parseValue
            #define parseValue (std::strtol(value, &endptr, 10))
            // Could also check for 'kB' etc ending


            // ------------------
            // Extract key: value
            // ------------------

            if (isKeyEqual("VmHWM"))
            {
                hwm_ = parseValue;
                --nkeys;
            }
            else if (isKeyEqual("VmPeak"))
            {
                peak_ = parseValue;
                --nkeys;
            }
            else if (isKeyEqual("VmSize"))
            {
                size_ = parseValue;
                --nkeys;
            }
            else if (isKeyEqual("VmRSS"))
            {
                rss_ = parseValue;
                --nkeys;
            }

            #undef isKeyEqual
            #undef parseValue
        }
    }

    #else  /* __linux__ */

    // Get process memory using POSIX getrusage() and also use that
    // for the baseline information
    {
        hwm_ = get_rusage_maxrss();

        peak_ = hwm_;  // ru_maxrss is the peak memory used
        rss_  = hwm_;  //<- getrusage doesn't distinguish current RSS from peak
        size_ = hwm_;  // Virtual size estimate from peak
    }

    // Get system free memory - platform dependent
    #ifdef __APPLE__
    {
        // // MacOS: use mach API to get memory information
        // mach_port_t host = mach_host_self();
        // mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
        // vm_statistics64_data_t vm_stat;

        //- Tends to break with Mach include headers inside c++. Might be
        //- fixed maybe in updates.
        // if
        // (
        //     host_statistics64
        //     (
        //         host,
        //         HOST_VM_INFO64,
        //         reinterpret_cast<host_info64_t>(&vm_stat),
        //        &count
        //     ) == KERN_SUCCESS
        // )
        // {
        //     vm_page_size is in bytes, convert to kB
        //     free_ = (vm_stat.free_count * vm_page_size) / 1024;
        // }

        // Use sysctlbyname() to get the page size and free page count
        int64_t vm_page_size = 0;
        size_t len = sizeof(vm_page_size);
        sysctlbyname("hw.pagesize", &vm_page_size, &len, NULL, 0);
        len = sizeof(free_);
        sysctlbyname("vm.page_free_count", &free_, &len, NULL, 0);
        free_ = (free_ * vm_page_size) / 1024;
    }
    #endif  /* __APPLE__ */
    #endif  /* __linux__ */
}


const Foam::memInfo& Foam::memInfo::update()
{
    clear();
    populate();
    return *this;
}


void Foam::memInfo::writeEntries(Ostream& os) const
{
    os.writeEntry("size", size_);
    os.writeEntry("hwm", hwm_);
    os.writeEntry("peak", peak_);
    os.writeEntry("rss", rss_);
    os.writeEntry("free", free_);
    os.writeEntry("units", "kB");
}


void Foam::memInfo::writeEntry(const word& keyword, Ostream& os) const
{
    os.beginBlock(keyword);
    writeEntries(os);
    os.endBlock();
}


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

// Foam::Istream& Foam::operator>>(Istream& is, memInfo& m)
// {
//     is.readBegin("memInfo");
//     is  >> m.hwm_ >> m.peak_ >> m.size_ >> m.rss_ >> m.free_;
//     is.readEnd("memInfo");
//
//     is.check(FUNCTION_NAME);
//     return is;
// }


Foam::Ostream& Foam::operator<<(Ostream& os, const memInfo& m)
{
    os  << token::BEGIN_LIST
        << m.hwm()  << token::SPACE
        << m.peak() << token::SPACE
        << m.size() << token::SPACE
        << m.rss()  << token::SPACE
        << m.free()
        << token::END_LIST;

    os.check(FUNCTION_NAME);
    return os;
}


// ************************************************************************* //
