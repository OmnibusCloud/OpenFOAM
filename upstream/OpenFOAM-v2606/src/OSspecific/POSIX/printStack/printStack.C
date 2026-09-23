/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2026 OpenCFD Ltd.
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

#include "error.H"
#include "OSspecific.H"

#include <cinttypes>
#include <map>
#include <vector>
#include <sstream>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>

// Experimental - batch process addr2line.
//#undef  Foam_batchPrintStack
//#define Foam_batchPrintStack
//#define Foam_useProcessPipe

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

// Note: demangle requires symbols only - without extra '(' etc.
Foam::word Foam::error::demangle(const char* symbol)
{
    if (!symbol || !*symbol)
    {
        return word();
    }

    int status = 0;
    char* cxx_sname = abi::__cxa_demangle(symbol, nullptr, nullptr, &status);

    if (status == 0 && cxx_sname)
    {
        // Store as a Foam::word since it is useful to print like
        // 'char*' (no surrounding quotes), but the demangled symbol
        // can contain spaces (ie, use no strip).
        //
        // We also change the old style '> >' to '>>' when copying too.

        const size_t nchars = ::strlen(cxx_sname);

        word demangled;
        demangled.reserve(nchars);

        for (size_t i = 0; i < nchars; ++i)
        {
            demangled.push_back(cxx_sname[i]);

            if
            (
                (i+2 < nchars)
             && cxx_sname[i] == '>'
             && cxx_sname[i+1] == ' '
             && cxx_sname[i+2] == '>'
            )
            {
                ++i;  // Eliminate extra space from '> >'
            }
        }

        ::free(cxx_sname);

        return demangled;
    }
    else
    {
        // Fallback is pass-through (no strip)
        return word(symbol, false);
    }
}


// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace
{

// Render stack using backtrace_symbols() and no external programs
//
// For MacOS the results of backtrace_symbols() do not necessarily
// provide much more information than directly querying dladdr()
// does. More importantly, the output is different than what glibc
// produces and it's not really worth parsing different outputs:
//
void render_stack_direct
(
    std::ostream& os,
    void* frames[],   // Raw stack symbols (from backtrace)
    int numFrames     // Number of frames
)
{
    constexpr int firstFrame = 1;

    if (numFrames <= firstFrame)
    {
        // Nothing to do
        return;
    }

    #ifdef __APPLE__
    Dl_info info;

    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        os  << '#' << framei << "  ";

        if (::dladdr(frames[framei], &info) != 0)
        {
            if (info.dli_sname)
            {
                os  << Foam::error::demangle(info.dli_sname);
            }
            else
            {
                // symbol is unknown or unresolved
                os  << '?';
            }

            if (info.dli_fname && *(info.dli_fname))
            {
                os  << " (in ";

                // atos and macos backtrace_symbols just report the
                // name without any path

                if (const char* slash = ::strrchr(info.dli_fname, '/'))
                {
                    os << (slash+1);
                }
                else
                {
                    os << (info.dli_fname);
                }
                os  << ')';
            }
        }
        os  << std::endl;
    }
    #else
    char **strings = ::backtrace_symbols(frames, numFrames);

    if (strings == nullptr)
    {
        // Failed
        return;
    }

    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        std::string str(strings[framei]);

        os  << '#' << framei << "  ";

        // This is glibc-specific
        //
        // Possibly shorten paths that appear to correspond to OpenFOAM
        // locations (platforms).
        //
        // Eg, "/path/openfoam/platforms/linux64Gcc/lib/libxyz.so"
        // --> "platforms/linux64Gcc/lib/libxyz.so"

        auto ldelim = str.find('(');
        auto beg = str.find("/platforms/");

        if (beg == std::string::npos || !beg || beg > ldelim)
        {
            beg = 0;
        }
        else
        {
            ++beg;
        }

        if
        (
            auto rdelim = ldelim;
            (ldelim != std::string::npos)
         && (rdelim = str.find('+', ldelim+1)) != std::string::npos
         && (rdelim > ldelim+1)
        )
        {
            // Found function between () e.g. "(__libc_start_main+0xd0)"
            // - demangle function name (before the '+' offset)
            // - preserve trailing [0xAddr]

            os  << Foam::error::demangle
                   (
                       str.substr(ldelim+1, rdelim-ldelim-1).c_str()
                   )
                << ' '
                << str.substr(beg, ldelim-beg);

            if ((rdelim = str.find('[', rdelim)) != std::string::npos)
            {
                os  << ' ' << str.substr(rdelim);
            }
        }
        else if (beg)
        {
            // With shortened path name
            os  << str.substr(beg);
        }
        else
        {
            // No modification to string
            os  << str;
        }
        os  << std::endl;
    }

    // Cleanup
    ::free(strings);
    #endif
}


// Forward Foam::Ostream -> std::ostream
inline void render_stack_direct
(
    Foam::Ostream& os,
    void* frames[],   // Raw stack symbols (from backtrace)
    int numFrames     // Number of frames
)
{
    if (auto* oss = dynamic_cast<Foam::OSstream*>(&os))
    {
        render_stack_direct(oss->stdStream(), frames, numFrames);
    }
}

} // End anonymous namespace


// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

#ifdef Foam_useProcessPipe
#include "ProcessPipe.cxx"
#endif

namespace
{

#if 0
// Read lines from piped command, excluding any empty lines
// Return the number of non-empty lines read
int pipeOpen(const std::string& cmd, std::vector<std::string>& strings)
{
    strings.clear();

    FILE* handle = ::popen(cmd.c_str(), "r");
    if (!handle) return 0;

    char* buf = nullptr;
    size_t len = 0;
    ssize_t nread;

    while ((nread = ::getline(&buf, &len, handle)) >= 0)
    {
        if (nread > 0 && buf[nread-1] == '\n')
        {
            // Remove trailing newline
            buf[--nread] == '\0';
        }
        strings.emplace_back(buf, nread);
    }

    ::free(buf);
    ::pclose(handle);

    return static_cast<int>(strings.size());
}
#endif


// Read up to and including lineNum from the piped command
// Return the final line read
std::string pipeOpen(const std::string& cmd, int lineNum = 0)
{
    // std::cerr<< "popen: " << cmd.c_str() << "\n";

    std::string str;

    FILE* handle = ::popen(cmd.c_str(), "r");
    if (!handle) return str;

    char* buf = nullptr;
    size_t len = 0;
    ssize_t nread;

    // Read lineNum number of lines
    for
    (
        int cnt = 0;
        cnt <= lineNum && (nread = ::getline(&buf, &len, handle)) >= 0;
        ++cnt
    )
    {
        if (cnt == lineNum)  // Retain the last line
        {
            if (nread > 0 && buf[nread-1] == '\n')
            {
                // Remove trailing newline
                buf[--nread] = '\0';
            }
            str.assign(buf, nread);
        }
    }

    ::free(buf);
    ::pclose(handle);

    return str;
}


// Possibly shorten paths.
// - replace CWD -> ""
// - replace $HOME -> "~/"
// - replace /path/openfoam/platforms/ -> <platforms>/
//
// Eg, "/path/openfoam/platforms/linux64Gcc/lib/libxyz.so"
// --> "<platforms>/linux64Gcc/lib/libxyz.so"

inline Foam::string& shorterPath(Foam::string& s)
{
    s.replace(Foam::cwd() + '/', "");
    s.replace(Foam::home(), "~");

    if (s[0] == '/')
    {
        auto beg = s.find("/platforms/");
        if
        (
            (beg != std::string::npos && beg > 1)
         && ((beg = s.find('/', beg+1)) != std::string::npos)
        )
        {
            s.replace(0, beg, "<platforms>");
        }
    }

    return s;
}


// Find executable on PATH
// - could also iterate through PATH directly
inline Foam::fileName whichPath(const char* fn)
{
    Foam::fileName fname(fn);

    if (!fname.empty() && fname[0] != '/' && fname[0] != '~')
    {
        std::string s = pipeOpen("command -v " + fname);

        if (s[0] == '/' || s[0] == '~')
        {
            fname = s;
        }
    }

    return fname;
}


#ifndef __APPLE__
uintptr_t resolveFileAddress
(
    const Foam::fileName& filename,
    const Dl_info& info,
    void *addr
)
{
    uintptr_t address = uintptr_t(addr);

    // Can use relative addresses for executables and libraries with the
    // Darwin addr2line implementation.
    // On other systems (Linux), only use relative addresses for libraries.

    #ifndef __APPLE__
    if (filename.has_ext("so"))
    #endif
    {
        // Convert address into offset into dynamic library
        uintptr_t offset = uintptr_t(info.dli_fbase);
        if (address > offset)
        {
            return (address - offset);
        }
    }

    return address;
}
#endif // not (__APPLE__)


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifndef __APPLE__

#ifndef Foam_batchPrintStack
//
// Render stack by calling <addr2line> for each frame
//
void render_stack_legacy
(
    Foam::Ostream& os,
    void* frames[],   // Raw stack symbols (from backtrace)
    int numFrames     // Number of frames
)
{
    constexpr int firstFrame = 1;

    if (numFrames <= firstFrame)
    {
        // Nothing to do
        return;
    }

    using namespace Foam;

    // Has <addr2line> available?
    const std::string addr2linePath
    (
        pipeOpen("command -v addr2line")
    );

    if (addr2linePath.empty())
    {
        os << "##  addr2line not found..." << Foam::endl;
        render_stack_direct(os, frames, numFrames);
        return;
    }

    Dl_info info;

    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        os  << '#' << framei << "  ";

        Foam::fileName fname;

        if (::dladdr(frames[framei], &info) != 0)
        {
            if (info.dli_sname)
            {
                os  << Foam::error::demangle(info.dli_sname);
            }
            else
            {
                // symbol is unknown or unresolved
                os  << '?';
            }

            if (info.dli_fname && *(info.dli_fname))
            {
                fname = whichPath(info.dli_fname);
            }
        }
        else
        {
            // unknown/unresolved
            os  << '?';
        }

        if (fname[0] == '/')
        {
            std::ostringstream command;
            command << addr2linePath << " --exe " << fname;

            uintptr_t address
            (
                resolveFileAddress(fname, info, frames[framei])
            );

            // Append address (hex format)
            command << " 0x" << uint64_t(address);


            // Call for a single address
            Foam::string line = pipeOpen(command.str());

            if (line.empty())
            {
                os  << " addr2line failed";
            }
            else if (line == "??:0" || line == "??:?" )
            {
                line = fname;
                os  << " in " << shorterPath(line).c_str();
            }
            else
            {
                os  << " at " << shorterPath(line).c_str();
            }
        }

        os  << Foam::endl;
    }
}

#else  /* defined(Foam_batchPrintStack) */

//
// Render stack by calling <addr2line> with collection of addresses
// per lib/exe name
//
// In theory by reducing the number of <addr2line> calls it should be
// faster, but that does not seem to be the case.
//
// Algorithm
// - collect frames according to lib/exe name
// - call addr2line for each adddress collection (per lib/exe)
// - report
void render_stack_batch
(
    Foam::Ostream& os,
    void* frames[],   // Raw stack symbols (from backtrace)
    int numFrames     // Number of frames
)
{
    constexpr int firstFrame = 1;

    if (numFrames <= firstFrame)
    {
        // Nothing to do
        return;
    }

    using namespace Foam;

    // Has <addr2line> available?
    const std::string addr2linePath
    (
        pipeOpen("command -v addr2line")
    );

    if (addr2linePath.empty())
    {
        os << "##  addr2line not found..." << Foam::endl;
        render_stack_direct(os, frames, numFrames);
        return;
    }


    // The per-frame library/exename
    std::vector<std::string> frameToName(numFrames);

    // Listing of library name -> string. One per frame
    std::vector<std::string> results(numFrames);

    // Map of library/exename -> (frame/address) tuples
    std::map<std::string, std::vector<std::pair<int, uint64_t>>> queries;

    Dl_info info;

    // Collect addresses per lib/exe name.
    //
    // Note that the 'whichPath()' call probably isn't very costly
    // since most frames come from libraries (absolute paths), not
    // the main executable.

    // Note that the 'whichPath()' call probably isn't very costly
    // since most frames are from libraries (absolute paths), not
    // the main executable.

    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        if
        (
            (0 != ::dladdr(frames[framei], &info))
         && (info.dli_fname && *(info.dli_fname))
        )
        {
            auto fname = whichPath(info.dli_fname);

            if (fname[0] == '/')
            {
                // Frame -> File name
                frameToName[framei] = fname;

                uintptr_t address
                (
                    resolveFileAddress(fname, info, frames[framei])
                );

                // File name -> ((frame, address), ...)
                queries[fname].emplace_back(framei, uint64_t(address));
            }
            else
            {
                // Fallback - not sure where this can happen...
                results[framei] = fname;
            }
        }
    }

    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        os  << '#' << framei << "  ";

        // Report the demangled symbol
        if ((0 != ::dladdr(frames[framei], &info)) && info.dli_sname)
        {
            os  << Foam::error::demangle(info.dli_sname);
        }
        else
        {
            os  << '?';
        }

        auto& fname = frameToName[framei];  // Full lib/exe name
        auto& line  = results[framei];      // Corresponding <addr2line> result

        if (!line.empty())
        {
            // Has previously generated result
        }
        else if (fname.empty())
        {
            // Cannot call <addr2line> without a lib/exe name
        }
        else if
        (
            auto iter = queries.find(fname);
            (iter != queries.end())
        )
        {
            // Call addr2line for all addresses in the query group
            const auto& input = iter->second;

            const auto maxLines = static_cast<int>(input.size());
            std::vector<int> indices;
            indices.reserve(maxLines);

            if (maxLines <= 0)
            {
                // Shouldn't happen
            }
            #ifdef Foam_useProcessPipe
            else if
            (
                auto process
              = PipeProcess::run_execve
                (
                    addr2linePath,
                    "rw",
                    addr2linePath, "--exe", fname
                );
                (process.is_read() && process.is_write())
            )
            {
                FILE* write_handle = process.is_write();
                FILE* read_handle = process.is_read();

                // Bulk send all addresses (hex format)
                for (const auto& [frameId, address] : input)
                {
                    // Remember the frame
                    indices.push_back(frameId);

                    ::fprintf
                    (
                        write_handle,
                        "0x%lx\n",
                        static_cast<unsigned long>(address)
                    );
                }
                process.flush_write();

                // Consume lines from pipe. One per input address
                {
                    char* buf = nullptr;
                    size_t len = 0;
                    ssize_t nread;

                    // Read maxLines number of lines
                    for
                    (
                        int lineCount = 0;
                        (
                            (lineCount < maxLines)
                         && ((nread = ::getline(&buf, &len, read_handle)) >= 0)
                        );
                        ++lineCount
                    )
                    {
                        if (nread > 0 && buf[nread-1] == '\n')
                        {
                            // Remove trailing newline
                            buf[--nread] = '\0';
                        }

                        results[indices[lineCount]].assign(buf, nread);
                    }

                    ::free(buf);
                }

                // process.shutdown(); <- implicit
            }
            #else  /* Foam_useProcessPipe */
            else
            {
                // Pass all addresses on the command line
                std::ostringstream command;
                command.setf(std::ios_base::hex, std::ios_base::basefield);

                command << addr2linePath << " --exe " << fname;

                // Append addresses (hex format)
                for (const auto& [frameId, address] : input)
                {
                    indices.push_back(frameId);
                    command << " 0x" << uint64_t(address);
                }

                if
                (
                    FILE* read_handle = ::popen(command.str().c_str(), "r");
                    read_handle
                )
                {
                    // Call pipe and consume lines. One per input address
                    char* buf = nullptr;
                    size_t len = 0;
                    ssize_t nread;

                    // Read maxLines number of lines
                    for
                    (
                        int lineCount = 0;
                        (
                            (lineCount < maxLines)
                         && ((nread = ::getline(&buf, &len, read_handle)) >= 0)
                        );
                        ++lineCount
                    )
                    {
                        if (nread > 0 && buf[nread-1] == '\n')
                        {
                            // Remove trailing newline
                            buf[--nread] = '\0';
                        }

                        results[indices[lineCount]].assign(buf, nread);
                    }

                    // Cleanup
                    ::free(buf);
                    ::pclose(read_handle);
                }
            }
            #endif  /* Foam_useProcessPipe */
        }

        // Report. Note we are free to discard file/line at this point
        if (line.empty())
        {
            os  << " addr2line failed";
        }
        else if (line == "??:0" || line == "??:?" )
        {
            Foam::string shorten(std::move(fname));
            os  << " in " << shorterPath(shorten).c_str();
        }
        else
        {
            Foam::string shorten(std::move(line));
            os  << " at " << shorterPath(shorten).c_str();
        }

        os  << Foam::endl;
    }
}

#endif  /* (Foam_batchPrintStack) */

#endif  // __APPLE__

} // End anonymous namespace


// MacOS render_stack version
#ifdef __APPLE__
#include "printStack_macos.cxx"
#endif


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::error::safePrintStack(std::ostream& os, int size)
{
    // Get raw stack symbols
    void *callstack[100];
    size = ::backtrace(callstack, (size > 0 && size < 100) ? size + 1 : 100);

    // Frame 0 is 'printStack()' - report something more meaningful
    os << "[stack trace]\n"
          "=============" << std::endl;

    if (size > 1)
    {
        render_stack_direct(os, callstack, size);
    }

    os << "=============" << std::endl;
}


void Foam::error::printStack(Foam::Ostream& os, int size)
{
    // Get raw stack symbols
    void *callstack[100];
    size = ::backtrace(callstack, (size > 0 && size < 100) ? size + 1 : 100);

    // Frame 0 is 'printStack()' - report something more meaningful
    os << "[stack trace]\n"
          "=============" << Foam::endl;

    const bool usePipe
    (
        !Foam::isAdministrator()
        // FUTURE?
        // && Foam::dynamicCode::allowSystemOperations
    );

    if (size <= 1)
    {
        // Empty stack
    }
    else if (usePipe)
    {
        #ifdef __APPLE__
        render_stack_macos(os, callstack, size);
        #elif defined(Foam_batchPrintStack)
        render_stack_batch(os, callstack, size);
        #else
        render_stack_legacy(os, callstack, size);
        #endif
    }
    else
    {
        render_stack_direct(os, callstack, size);
    }

    os << "=============" << Foam::endl;
}


// ************************************************************************* //
