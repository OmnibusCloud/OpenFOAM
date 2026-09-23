/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2026 OpenCFD Ltd.
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

Description
    MacOS-specific handling of printStack. Unlike Linux etc, it uses the
    "atos" for symbolization. However, atos is not a drop-in replacement
    but needed additional information about the image "slide" etc.

See
    man(3) dyld

    https://developer.apple.com/documentation/xcode/\
    adding-identifiable-symbol-names-to-a-crash-report

\*---------------------------------------------------------------------------*/

#include "error.H"
#include "OSspecific.H"

#include <execinfo.h>

// Bit of a hack to avoid for compiling under g++ on MacOS
#include "dylib_macos.h"

#include <mach/port.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <vector>
#include <string>
#include <sstream>

// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace
{

//
// Render stack using "atos"
//
// For each stack frame, need to identify which images it came from
// and obtain the corresponding "slide" (start address) for all subsequent
// addressing.
//
// For each frame, located the corresponding image. Process all frames
// on that image and cache the "atos" results.
//
void render_stack_macos
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

    // Expect <xcrun> to be available, and can use it to find <atos>
    const std::string atosPath
    (
        pipeOpen("xcrun --find atos 2>/dev/null")
    );

    if (atosPath.empty())
    {
        os << "##  xcrun and/or atos not found..." << Foam::endl;
        render_stack_direct(os, frames, numFrames);
        return;
    }

    // Expect mach_header_64 types (no 32-bit support)
    #if defined(__arm64__) || defined(__x86_64__)
    struct ImageInfo
    {
        std::string path;
        uintptr_t slide;
        std::vector<std::pair<uintptr_t, uintptr_t>> ranges;
    };

    // Step 1: build Mach-O image cache
    std::vector<ImageInfo> images;
    {
        const uint32_t nImages = _dyld_image_count();

        images.reserve(nImages);

        for (uint32_t imageIdx = 0; imageIdx < nImages; ++imageIdx)
        {
            const auto* header = reinterpret_cast<const mach_header_64*>
            (
                _dyld_get_image_header(imageIdx)
            );

            if (!header) continue;

            // Already restricted to mach_header_64 with
            // pre-processor guards...
            // if (header->magic != MH_MAGIC_64) continue;

            auto& img = images.emplace_back();

            img.path  = _dyld_get_image_name(imageIdx);
            img.slide = static_cast<uintptr_t>
            (
                _dyld_get_image_vmaddr_slide(imageIdx)
            );

            auto* lc = reinterpret_cast<const load_command*>
            (
                reinterpret_cast<const char*>(header)
              + sizeof(mach_header_64)
            );

            for (uint32_t cmdi = 0; cmdi < header->ncmds; ++cmdi)
            {
                if (lc->cmd == LC_SEGMENT_64)
                {
                    const auto* seg =
                        reinterpret_cast<const segment_command_64*>(lc);

                    uintptr_t start =
                    (
                        static_cast<uintptr_t>(seg->vmaddr) + img.slide
                    );

                    img.ranges.emplace_back
                    (
                        start,
                        start + static_cast<uintptr_t>(seg->vmsize)
                    );
                }

                // The next load command
                lc = reinterpret_cast<const load_command*>
                (
                    reinterpret_cast<const char*>(lc) + lc->cmdsize
                );
            }
        }
    }

    // Step 2: Map frames to images, maintaining order
    std::vector<const ImageInfo*> frame_info
    (
        numFrames,
        static_cast<const ImageInfo*>(nullptr)
    );

    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        auto addr = reinterpret_cast<uintptr_t>(frames[framei]);
        const ImageInfo* owner = nullptr;

        for (auto& img : images)
        {
            for (const auto& r : img.ranges)
            {
                if (addr >= r.first && addr < r.second)
                {
                    owner = &img;
                    break;
                }
            }
            if (owner) break;
        }

        frame_info[framei] = owner;
    }


    // Finally.
    // Walk the frames in stack order, using the "results"
    // vector to cache the values from atos.
    std::vector<std::string> results(numFrames);

    // Helper routine.
    // Stringify frame address and file name via dladdr()
    // formatted as "symbol (in library)"
    const auto eval_frameAddress = [](void* frameAddr) -> std::string
    {
        std::ostringstream oss;

        // No image - get basic information from dladdr()
        if
        (
            Dl_info info;
            (::dladdr(frameAddr, &info) != 0)
         && (info.dli_sname && *(info.dli_sname))
        )
        {
            oss << Foam::error::demangle(info.dli_sname);

            if (info.dli_fname && *(info.dli_fname))
            {
                oss  << " (in ";
                // atos and macos backtrace_symbols just report the
                // name without any path

                if (const char* slash = ::strrchr(info.dli_fname, '/'))
                {
                    oss << (slash+1);
                }
                else
                {
                    oss << (info.dli_fname);
                }
                oss  << ')';
            }
        }
        else
        {
            oss.setf(std::ios_base::hex, std::ios_base::basefield);
            oss << "0x " << uint64_t(frameAddr) << " (no image)";
        }

        return oss.str();
    };


    for (int framei = firstFrame; framei < numFrames; ++framei)
    {
        if (results[framei].empty())
        {
            if (const ImageInfo* img = frame_info[framei]; img)
            {
                std::vector<int> indices;

                std::ostringstream command;
                command.setf(std::ios_base::hex, std::ios_base::basefield);

                // Call <atos>, which was found by xcrun.
                // Alternative: command << "xcrun atos";
                command << atosPath;
                #if defined(__arm64__)
                command << " -arch arm64";
                #elif defined(__x86_64__)
                command << " -arch x86_64";
                #endif
                command
                    << " -o '" << img->path
                    << "' -l 0x" << uint64_t(img->slide);

                // Collect frames + build command in one pass
                for (int scan = framei; scan < numFrames; ++scan)
                {
                    if ((frame_info[scan] == img) && results[scan].empty())
                    {
                        indices.push_back(scan);
                        command << " 0x" << uint64_t(frames[scan]);
                    }
                }

                const auto maxLines = static_cast<int>(indices.size());

                if (maxLines <= 0)
                {
                    // Shouldn't happen
                }
                else if
                (
                    FILE* handle = ::popen(command.str().c_str(), "r");
                    handle
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
                            lineCount < maxLines
                         && (nread = ::getline(&buf, &len, handle)) >= 0
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
                    ::pclose(handle);
                }
            }
            else
            {
                // No image - get basic information from dladdr()
                results[framei] = eval_frameAddress(frames[framei]);
            }
        }

        // Output

        os  << '#' << framei << "  ";

        if (results[framei].empty())
        {
            os << "? resolve: " << uint64_t(frames[framei]);
        }
        else
        {
            // Catch the odd corner case where querying the
            // information for the executable itself just return the
            // address (likely if it is stripped?).
            // In that case, use dladdr() to provide something better...

            if
            (
                (results[framei].size() > 2)
             && (results[framei].compare(0, 2, "0x")) == 0
            )
            {
                results[framei] = eval_frameAddress(frames[framei]);
            }

            os << results[framei].c_str();
        }

        os  << Foam::endl;
    }
    #else
    {
        os << "##  no print stack (atos) support on this architecture\n";
        render_stack_direct(os, frames, numFrames);
    }
    #endif  /* defined(__arm64__) || defined(__x86_64__) */
}


} // End anonymous namespace


// ************************************************************************* //
