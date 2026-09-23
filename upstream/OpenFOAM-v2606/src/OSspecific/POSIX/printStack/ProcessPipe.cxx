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

\*---------------------------------------------------------------------------*/

#include "error.H"
#include "OSspecific.H"

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

/*---------------------------------------------------------------------------*\
                         Class PipeProcess Declaration
\*---------------------------------------------------------------------------*/

namespace Foam
{

// Wrapping for a bidirectional pipe
class PipeProcess
{
    // Private Data

        //- Input file handle
        FILE* in_ = nullptr;

        //- Output file handle
        FILE* out_ = nullptr;

        //- The child process id
        pid_t pid_ = -1;


    // Private Member Functions

        //- Build an argv vector of strings from variadic arguments.
        //  Caller is responsible for sanity checks
        template<typename... Args>
        static std::vector<std::string> makeArgVector(Args&&... args)
        {
            std::vector<std::string> strings;
            strings.reserve(sizeof...(args));
            (strings.emplace_back(std::forward<Args>(args)), ...);
            return strings;
        }

public:

    //- Default construct
    PipeProcess() = default;

    //- No copy construct
    PipeProcess(const PipeProcess&) = delete;

    //- No copy assignment
    PipeProcess& operator=(const PipeProcess&) = delete;

    //- Move construct
    inline PipeProcess(PipeProcess&& other) noexcept;

    //- Move assignment
    inline PipeProcess& operator=(PipeProcess&& other) noexcept;

    //- Destructor performs a shutdown
    ~PipeProcess() { shutdown(); }


    // Member Functions

        //- The pipe input (its stdin)
        FILE* in() const noexcept { return in_; }

        //- The pipe output (its stdout)
        FILE* out() const noexcept { return out_; }

        //- The child pid
        pid_t pid() const noexcept { return pid_; }

        //- Has good file handles, pid etc
        bool is_open() const noexcept
        {
            return ((in_ || out_) && pid_ > 0);
        }

        //- Can read from pipe
        FILE* is_read() const noexcept
        {
            return (pid_ > 0 ? out_ : static_cast<FILE*>(nullptr));
        }

        //- Can write to pipe
        FILE* is_write() const noexcept
        {
            return (pid_ > 0 ? in_ : static_cast<FILE*>(nullptr));
        }

        //- Flush the write stream
        void flush_write() { if (in_) { ::fflush(in_); } }

        //- Flush and close the write stream
        void close_write()
        {
            if (in_) { ::fflush(in_); ::fclose(in_); in_ = nullptr; }
        }

        //- Shutdown the process. Close file handles etc.
        void shutdown()
        {
            if (in_) { ::fflush(in_); ::fclose(in_); in_ = nullptr; }
            if (out_) { ::fclose(out_); out_ = nullptr; }
            if (pid_ > 0) { ::waitpid(pid_, nullptr, 0); }
            pid_ = -1;
        }


    // Factory methods

        //- Shell command (like popen)
        template<typename Command>
        static PipeProcess run_sh(const char* mode, Command&& first)
        {
            auto argv = makeArgVector
            (
                "sh",
                "-c",
                std::forward<Command>(first)
            );
            return forkExec(mode, argv, nullptr, true);
        }

        //- Run command with absolute path (execve)
        template<typename FirstArg, typename... Args>
        static PipeProcess run_execve
        (
            const std::string& path,
            const char* mode,
            FirstArg&& first,
            Args&&... rest
        )
        {
            auto argv = makeArgVector
            (
                std::forward<FirstArg>(first),
                std::forward<Args>(rest)...
            );
            return forkExec(mode, argv, path.c_str(), false);
        }

        //- Command with searching in PATH (execlp)
        template<typename FirstArg, typename... Args>
        static PipeProcess run_execlp
        (
            const char* mode,
            FirstArg&& first,
            Args&&... rest
        )
        {
            auto argv = makeArgVector
            (
                std::forward<FirstArg>(first),
                std::forward<Args>(rest)...
            );
            return forkExec(mode, argv, nullptr, true);
        }


private:

    // --------------------------
    // Internal: fork/dup2/pipe/exec
    // --------------------------
    inline static PipeProcess forkExec
    (
        // The "r", "w" or "rw" mode
        const char* mode,
        const std::vector<std::string>& argv,
        // Absolute path for command
        const char* path = nullptr,
        // Use 'path' versions of exec
        bool searchPath = false
    );
};

} // End namespace Foam


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::PipeProcess::PipeProcess(PipeProcess&& other) noexcept
:
    in_(other.in_),
    out_(other.out_),
    pid_(other.pid_)
{
    other.in_ = nullptr;
    other.out_ = nullptr;
    other.pid_ = -1;
}


Foam::PipeProcess& Foam::PipeProcess::operator=(PipeProcess&& other) noexcept
{
    if (this != &other)
    {
        shutdown();
        in_ = other.in_;
        out_ = other.out_;
        pid_ = other.pid_;
        other.in_ = nullptr;
        other.out_ = nullptr;
        other.pid_ = -1;
    }
    return *this;
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

// fork/dup2/pipe/exec
Foam::PipeProcess Foam::PipeProcess::forkExec
(
    const char* mode,
    const std::vector<std::string>& argv,
    const char* path,
    bool searchPath
)
{
    bool read_mode  = (mode && ::strchr(mode, 'r'));
    bool write_mode = (mode && ::strchr(mode, 'w'));

    if (!read_mode && !write_mode)
    {
        std::cerr<< "PipeProcess: did not specify a known mode. mode='";
        if (mode) std::cerr<< mode;
        std::cerr<< "'\n";
        return PipeProcess{};
    }

    // Create write pipe
    int to_child[2] = {-1, -1};
    if (write_mode && (::pipe(to_child) != 0))
    {
        std::cerr<< "PipeProcess: failed to create <to child> file pipe\n";
        return PipeProcess{};
    }

    // Create read pipe
    int from_child[2] = {-1, -1};
    if (read_mode && (::pipe(from_child) != 0))
    {
        std::cerr<< "PipeProcess: failed to create <from child> file pipe\n";
        if (write_mode) { ::close(to_child[0]); ::close(to_child[1]); }
        return PipeProcess{};
    }

    pid_t child_pid = ::fork();

    if (child_pid == -1)
    {
        std::cerr << "PipeProcess: failed to fork process\n";
        if (write_mode) { ::close(to_child[0]); ::close(to_child[1]); }
        if (read_mode)  { ::close(from_child[0]); ::close(from_child[1]); }
        return PipeProcess{};
    }
    else if (child_pid == 0)
    {
        // In child

        // Redirect <stdin> to be to_child[0] and close local descriptors
        if (write_mode)
        {
            ::dup2(to_child[0], STDIN_FILENO);
            ::close(to_child[0]);
            ::close(to_child[1]);
        }

        // Redirect <stdout> to use from_child[1] and close local descriptors
        if (read_mode)
        {
            ::dup2(from_child[1], STDOUT_FILENO);
            ::close(from_child[0]);
            ::close(from_child[1]);
        }

        // Create a NULL terminated list of pointer to string content
        std::vector<char*> cargv;
        cargv.reserve(argv.size()+1);
        for (const auto& s : argv)
        {
            cargv.push_back(const_cast<char*>(s.data()));
            // cargv.push_back(s.data());
        }
        cargv.push_back(nullptr);

        if (searchPath)
        {
            ::execvp(cargv[0], cargv.data());
        }
        else if (path)
        {
            // Use enclosing environment
            ::execve(path, cargv.data(), environ);
        }
        else
        {
            ::execvp(cargv[0], cargv.data());
        }

        // Obviously failed, since exec should not return
        _exit(127);
    }

    // In parent

    PipeProcess handles;
    handles.pid_ = child_pid;

    if (write_mode)
    {
        handles.in_ = ::fdopen(to_child[1], "w");
        ::close(to_child[0]);
    }
    if (read_mode)
    {
        handles.out_ = ::fdopen(from_child[0], "r");
        ::close(from_child[1]);
    }

    if ((write_mode && !handles.in_) || (read_mode && !handles.out_))
    {
        std::cerr<< "PipeProcess: failed to open handles to file descriptors\n";
        handles.shutdown();
        return PipeProcess{};
    }
    else
    {
        return handles;
    }
}


// ************************************************************************* //
