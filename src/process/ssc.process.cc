module;

#include "../common.hh"

#include <cstring>
#include <iostream>
#include <stdexcept>

#if defined(_WIN32)
#include <tlhelp32.h>
#else
#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <sstream>
#include <signal.h>
#endif

export module ssc.process;

export namespace ssc {
  ssc::MessageCallback exitCallback;

  struct ExecOutput {
    ssc::String output;
    int exitCode = 0;
  };

  inline ExecOutput exec (ssc::String command) {
    command = command + " 2>&1";

    ExecOutput eo;
    FILE* pipe;
    size_t count;
    int exitCode = 0;
    const int bufsize = 128;
    std::array<char, 128> buffer;

    #if defined(_WIN32)
      //
      // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen?view=msvc-160
      // _popen works fine in a console application... ok fine that's all we need it for... thanks.
      //
      pipe = _popen((const char*) command.c_str(), "rt");
    #else
      pipe = popen((const char*) command.c_str(), "r");
    #endif

    if (pipe == NULL) {
      std::cout << "error: unable to open the command" << std::endl;
      exit(1);
    }

    do {
      if ((count = fread(buffer.data(), 1, bufsize, pipe)) > 0) {
        eo.output.insert(eo.output.end(), std::begin(buffer), std::next(std::begin(buffer), count));
      }
    } while (count > 0);

    #ifdef _WIN32
    exitCode = _pclose(pipe);
    #else
    exitCode = pclose(pipe);
    #endif

    if (!WIFEXITED(exitCode) || exitCode != 0) {
      auto status = WEXITSTATUS(exitCode);
      if (status && exitCode) {
        exitCode = status;
      }
    }

    eo.exitCode = exitCode;

    return eo;
  }

  // Additional parameters to Process constructors.
  struct Config {
    // Buffer size for reading stdout and stderr. Default is 131072 (128 kB).
    std::size_t buffer_size = 131072;
    // Set to true to inherit file descriptors from parent process. Default is false.
    // On Windows: has no effect unless read_stdout==nullptr, read_stderr==nullptr and open_stdin==false.
    bool inherit_file_descriptors = false;

    // On Windows only: controls how the process is started, mimics STARTUPINFO's wShowWindow.
    // See: https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/ns-processthreadsapi-startupinfoa
    // and https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-showwindow
    enum class ShowWindow {
      hide = 0,
      show_normal = 1,
      show_minimized = 2,
      maximize = 3,
      show_maximized = 3,
      show_no_activate = 4,
      show = 5,
      minimize = 6,
      show_min_no_active = 7,
      show_na = 8,
      restore = 9,
      show_default = 10,
      force_minimize = 11
    };
    // On Windows only: controls how the window is shown.
    ShowWindow show_window{ShowWindow::show_default};
  };

  // Platform independent class for creating processes.
  // Note on Windows: it seems not possible to specify which pipes to redirect.
  // Thus, at the moment, if read_stdout==nullptr, read_stderr==nullptr and open_stdin==false,
  // the stdout, stderr and stdin are sent to the parent process instead.
  class Process {
  public:
    ssc::String command;
    ssc::String argv;
    ssc::String path;
#ifdef _WIN32
    typedef unsigned long id_type; // Process id type
    typedef void *fd_type;         // File descriptor type
#else
    typedef pid_t id_type;
    typedef int fd_type;
    typedef ssc::String string_type;
#endif

  private:
    class Data {
    public:
      Data() noexcept;
      id_type id;
#if defined(_WIN32)
      void *handle{nullptr};
#endif
      int exit_status{-1};
    };

  public:
    Process(
      const ssc::String &command,
      const ssc::String &argv,
      const ssc::String &path = ssc::String(""),
      ssc::MessageCallback read_stdout = nullptr,
      ssc::MessageCallback read_stderr = nullptr,
      ssc::MessageCallback on_exit = nullptr,
      bool open_stdin = true,
      const Config &config = {}) noexcept;

#if !defined(_WIN32)
    // Starts a process with the environment of the calling process.
    // Supported on Unix-like systems only.
    Process(
      const std::function<int()> &function,
      ssc::MessageCallback read_stdout = nullptr,
      ssc::MessageCallback read_stderr = nullptr,
      ssc::MessageCallback on_exit = nullptr,
      bool open_stdin = true,
      const Config &config = {}) noexcept;
#endif

    ~Process() noexcept {
      close_fds();
    };

    // Get the process id of the started process.
    id_type getPID() const noexcept {
      return data.id;
    }

    // Write to stdin.
    bool write(const char *bytes, size_t n);
    // Write to stdin. Convenience function using write(const char *, size_t).
    bool write(const ssc::String &str);
    // Close stdin. If the process takes parameters from stdin, use this to
    // notify that all parameters have been sent.
    void close_stdin() noexcept;
    void open() noexcept;

    // Kill a given process id. Use kill(bool force) instead if possible.
    // force=true is only supported on Unix-like systems.
    void kill(id_type id) noexcept;

  private:
    Data data;
    bool closed;
    std::mutex close_mutex;
    ssc::MessageCallback read_stdout;
    ssc::MessageCallback read_stderr;
    ssc::MessageCallback on_exit;
#if !defined(_WIN32)
    std::thread stdout_stderr_thread;
#else
    std::thread stdout_thread, stderr_thread;
#endif
    bool open_stdin;
    std::mutex stdin_mutex;

    Config config;

    std::unique_ptr<fd_type> stdout_fd, stderr_fd, stdin_fd;

    id_type open(const ssc::String &command, const ssc::String &path) noexcept;
    id_type open(const std::function<int()> &function) noexcept;

    void read() noexcept;
    void close_fds() noexcept;
  };

  inline bool Process::write (const ssc::String &s) {
    return Process::write(s.c_str(), s.size());
  }

  inline void Process::open () noexcept {
    if (this->command.size() == 0) return;
    open(this->command + this->argv, this->path);
    read();
  }

  inline Process::Process(
    const ssc::String &command,
    const ssc::String &argv,
    const ssc::String &path,
    ssc::MessageCallback read_stdout,
    ssc::MessageCallback read_stderr,
    ssc::MessageCallback on_exit,
    bool open_stdin,
    const Config &config) noexcept
      : closed(true),
        open_stdin(true),
        read_stdout(std::move(read_stdout)),
        read_stderr(std::move(read_stderr)),
        on_exit(std::move(on_exit)) {
    this->command = command;
    this->argv = argv;
    this->path = path;
  }

  ssc::StringStream initial;

#if defined(_WIN32)
  //
  // Windows processes
  //

  Process::Data::Data() noexcept : id(0) {}

  // Simple HANDLE wrapper to close it automatically from the destructor.
  class Handle {
    public:
      Handle() noexcept : handle(INVALID_HANDLE_VALUE) {}

      ~Handle() noexcept {
        close();
      }

      void close() noexcept {
        if (handle != INVALID_HANDLE_VALUE) {
          CloseHandle(handle);
        }
      }

      HANDLE detach() noexcept {
        HANDLE old_handle = handle;
        handle = INVALID_HANDLE_VALUE;
        return old_handle;
      }

      operator HANDLE() const noexcept { return handle; }
      HANDLE *operator&() noexcept { return &handle; }

    private:
      HANDLE handle;
  };

  std::mutex create_process_mutex;

  Process::id_type Process::open(const ssc::String &command, const ssc::String &path) noexcept {
    if (open_stdin) {
      stdin_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
    }

    if (read_stdout) {
      stdout_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
    }

    if (read_stderr) {
      stderr_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
    }

    Handle stdin_rd_p;
    Handle stdin_wr_p;
    Handle stdout_rd_p;
    Handle stdout_wr_p;
    Handle stderr_rd_p;
    Handle stderr_wr_p;

    SECURITY_ATTRIBUTES security_attributes;

    security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attributes.bInheritHandle = TRUE;
    security_attributes.lpSecurityDescriptor = nullptr;

    std::lock_guard<std::mutex> lock(create_process_mutex);

    if (stdin_fd) {
      if (!CreatePipe(&stdin_rd_p, &stdin_wr_p, &security_attributes, 0) ||
         !SetHandleInformation(stdin_wr_p, HANDLE_FLAG_INHERIT, 0))
        return 0;
    }

    if (stdout_fd) {
      if (!CreatePipe(&stdout_rd_p, &stdout_wr_p, &security_attributes, 0) ||
         !SetHandleInformation(stdout_rd_p, HANDLE_FLAG_INHERIT, 0)) {
        return 0;
      }
    }

    if (stderr_fd) {
      if (!CreatePipe(&stderr_rd_p, &stderr_wr_p, &security_attributes, 0) ||
         !SetHandleInformation(stderr_rd_p, HANDLE_FLAG_INHERIT, 0)) {
        return 0;
      }
    }

    PROCESS_INFORMATION process_info;
    STARTUPINFO startup_info;

    ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));

    ZeroMemory(&startup_info, sizeof(STARTUPINFO));
    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdInput = stdin_rd_p;
    startup_info.hStdOutput = stdout_wr_p;
    startup_info.hStdError = stderr_wr_p;

    if (stdin_fd || stdout_fd || stderr_fd)
      startup_info.dwFlags |= STARTF_USESTDHANDLES;

    if (config.show_window != Config::ShowWindow::show_default) {
      startup_info.dwFlags |= STARTF_USESHOWWINDOW;
      startup_info.wShowWindow = static_cast<WORD>(config.show_window);
    }

    auto process_command = command;
#ifdef MSYS_PROCESS_USE_SH
    size_t pos = 0;
    while((pos = process_command.find('\\', pos)) != string_type::npos) {
      process_command.replace(pos, 1, "\\\\\\\\");
      pos += 4;
    }
    pos = 0;
    while((pos = process_command.find('\"', pos)) != string_type::npos) {
      process_command.replace(pos, 1, "\\\"");
      pos += 2;
    }
    process_command.insert(0, "sh -c \"");
    process_command += "\"";
#endif

    BOOL bSuccess = CreateProcess(
      nullptr,
      process_command.empty() ? nullptr : &process_command[0],
      nullptr,
      nullptr,
      stdin_fd || stdout_fd || stderr_fd || config.inherit_file_descriptors, // Cannot be false when stdout, stderr or stdin is used
      stdin_fd || stdout_fd || stderr_fd ? CREATE_NO_WINDOW : 0,             // CREATE_NO_WINDOW cannot be used when stdout or stderr is redirected to parent process
      nullptr,
      path.empty() ? nullptr : path.c_str(),
      &startup_info,
      &process_info
    );

    if (!bSuccess) {
      auto msg = ssc::String("Unable to execute: " + process_command);
      MessageBoxA(nullptr, &msg[0], "Alert", MB_OK | MB_ICONSTOP);
      return 0;
    } else {
      CloseHandle(process_info.hThread);
    }

    if (stdin_fd) {
      *stdin_fd = stdin_wr_p.detach();
    }

    if (stdout_fd) {
      *stdout_fd = stdout_rd_p.detach();
    }

    if (stderr_fd) {
      *stderr_fd = stderr_rd_p.detach();
    }

    auto t = std::thread([this, &process_info]() {
      WaitForSingleObject(process_info.hProcess, INFINITE);
      DWORD exitCode;
      GetExitCodeProcess(process_info.hProcess, &exitCode);
      on_exit(std::to_string(exitCode));
    });

    t.detach();

    closed = false;
    data.id = process_info.dwProcessId;
    data.handle = process_info.hProcess;

    return process_info.dwProcessId;
  }

  void Process::read() noexcept {
    if (data.id == 0) {
      return;
    }

    if (stdout_fd) {
      stdout_thread = std::thread([this]() {
        DWORD n;

        std::unique_ptr<char[]> buffer(new char[config.buffer_size]);
        ssc::StringStream ss;

        for (;;) {
          memset(buffer.get(), 0, config.buffer_size);
          BOOL bSuccess = ReadFile(*stdout_fd, static_cast<CHAR *>(buffer.get()), static_cast<DWORD>(config.buffer_size), &n, nullptr);

          if (!bSuccess || n == 0) {
            break;
          }

          auto b = ssc::String(buffer.get());
          auto parts = splitc(b, '\n');

          if (parts.size() > 1) {
            for (int i = 0; i < parts.size() - 1; i++) {
              ss << parts[i];
              ssc::String s(ss.str());
              read_stdout(s);
              ss.str(ssc::String());
              ss.clear();
              ss.copyfmt(initial);
            }
            ss << parts[parts.size() - 1];
          } else {
            ss << b;
          }
        }
      });
    }

    if (stderr_fd) {
      stderr_thread = std::thread([this]() {
        DWORD n;
        std::unique_ptr<char[]> buffer(new char[config.buffer_size]);

        for (;;) {
          BOOL bSuccess = ReadFile(*stderr_fd, static_cast<CHAR *>(buffer.get()), static_cast<DWORD>(config.buffer_size), &n, nullptr);
          if (!bSuccess || n == 0) break;
          read_stderr(ssc::String(buffer.get()));
        }
      });
    }
  }

  void Process::close_fds() noexcept {
    if (stdout_thread.joinable()) {
      stdout_thread.join();
    }

    if (stderr_thread.joinable()) {
      stderr_thread.join();
    }

    if (stdin_fd) {
      close_stdin();
    }

    if (stdout_fd) {
      if (*stdout_fd != nullptr) {
        CloseHandle(*stdout_fd);
      }

      stdout_fd.reset();
    }

    if (stderr_fd) {
      if (*stderr_fd != nullptr) {
        CloseHandle(*stderr_fd);
      }

      stderr_fd.reset();
    }
  }

  bool Process::write(const char *bytes, size_t n) {
    if (!open_stdin) {
      throw std::invalid_argument("Can't write to an unopened stdin pipe. Please set open_stdin=true when constructing the process.");
    }

    std::lock_guard<std::mutex> lock(stdin_mutex);
    if (stdin_fd) {
      ssc::String b(bytes);

      while (true && (b.size() > 0)) {
        DWORD bytesWritten;
        DWORD size = static_cast<DWORD>(b.size());
        BOOL bSuccess = WriteFile(*stdin_fd, b.c_str(), size, &bytesWritten, nullptr);

        if (bytesWritten >= size || bSuccess) {
          break;
        }

        b = b.substr(bytesWritten / 2, b.size());
      }

      DWORD bytesWritten;
      BOOL bSuccess = WriteFile(*stdin_fd, L"\n", static_cast<DWORD>(2), &bytesWritten, nullptr);

      if (!bSuccess || bytesWritten == 0) {
        return false;
      } else {
        return true;
      }
    }

    return false;
  }

  void Process::close_stdin() noexcept {
    std::lock_guard<std::mutex> lock(stdin_mutex);

    if (stdin_fd) {
      if (*stdin_fd != nullptr) {
        CloseHandle(*stdin_fd);
      }

      stdin_fd.reset();
    }
  }

  void Process::kill(id_type id) noexcept {
    if (id == 0) {
      return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot) {
      PROCESSENTRY32 process;
      ZeroMemory(&process, sizeof(process));
      process.dwSize = sizeof(process);

      if (Process32First(snapshot, &process)) {
        do {
          if (process.th32ParentProcessID == id) {
            HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process.th32ProcessID);

            if (process_handle) {
              TerminateProcess(process_handle, 2);
              CloseHandle(process_handle);
            }
          }
        } while (Process32Next(snapshot, &process));
      }

      CloseHandle(snapshot);
    }

    HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, id);

    if (process_handle) {
      TerminateProcess(process_handle, 2);
    }
  }
#else
  //
  // UNIX-LIKE processes
  //

  Process::Data::Data() noexcept : id(-1) {}

  Process::Process(
    const std::function<int()> &function,
    MessageCallback read_stdout,
    MessageCallback read_stderr,
    MessageCallback on_exit,
    bool open_stdin, const Config &config
  ) noexcept:
    closed(true),
    read_stdout(std::move(read_stdout)),
    read_stderr(std::move(read_stderr)),
    on_exit(std::move(on_exit)),
    open_stdin(open_stdin),
    config(config) {

    open(function);
    read();

    exitCallback = on_exit;

    signal(SIGCHLD, [](int code) {
      exitCallback(std::to_string(code));
    });
  }

  Process::id_type Process::open(const std::function<int()> &function) noexcept {
    if (open_stdin) {
      stdin_fd = std::unique_ptr<fd_type>(new fd_type);
    }

    if (read_stdout) {
      stdout_fd = std::unique_ptr<fd_type>(new fd_type);
    }

    if (read_stderr) {
      stderr_fd = std::unique_ptr<fd_type>(new fd_type);
    }

    int stdin_p[2];
    int stdout_p[2];
    int stderr_p[2];

    if (stdin_fd && pipe(stdin_p) != 0) {
      return -1;
    }

    if (stdout_fd && pipe(stdout_p) != 0) {
      if (stdin_fd) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }
      return -1;
    }

    if (stderr_fd && pipe(stderr_p) != 0) {
      if (stdin_fd) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }
      if (stdout_fd) {
        close(stdout_p[0]);
        close(stdout_p[1]);
      }
      return -1;
    }

    id_type pid = fork();

    if (pid < 0) {
      if (stdin_fd) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }
      if (stdout_fd) {
        close(stdout_p[0]);
        close(stdout_p[1]);
      }
      if (stderr_fd) {
        close(stderr_p[0]);
        close(stderr_p[1]);
      }
      return pid;
    } else if (pid == 0) {
      if (stdin_fd) {
        dup2(stdin_p[0], 0);
      }

      if (stdout_fd) {
        dup2(stdout_p[1], 1);
      }

      if (stderr_fd) {
        dup2(stderr_p[1], 2);
      }

      if (stdin_fd) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }

      if (stdout_fd) {
        close(stdout_p[0]);
        close(stdout_p[1]);
      }

      if (stderr_fd) {
        close(stderr_p[0]);
        close(stderr_p[1]);
      }

      setpgid(0, 0);

      if (function) {
        function();
      }

      _exit(EXIT_FAILURE);
    }

    if (stdin_fd) {
      close(stdin_p[0]);
    }

    if (stdout_fd) {
      close(stdout_p[1]);
    }

    if (stderr_fd) {
      close(stderr_p[1]);
    }

    if (stdin_fd) {
      *stdin_fd = stdin_p[1];
    }

    if (stdout_fd) {
      *stdout_fd = stdout_p[0];
    }

    if (stderr_fd) {
      *stderr_fd = stderr_p[0];
    }

    closed = false;
    data.id = pid;
    return pid;
  }

  Process::id_type Process::open(const ssc::String &command, const ssc::String &path) noexcept {
    return open([&command, &path] {
      auto command_c_str = command.c_str();
      ssc::String cd_path_and_command;

      if (!path.empty()) {
        auto path_escaped = path;
        size_t pos = 0;

        while((pos = path_escaped.find('\'', pos)) != ssc::String::npos) {
          path_escaped.replace(pos, 1, "'\\''");
          pos += 4;
        }

        cd_path_and_command = "cd '" + path_escaped + "' && " + command; // To avoid resolving symbolic links
        command_c_str = cd_path_and_command.c_str();
      }

      return execl("/bin/sh", "/bin/sh", "-c", command_c_str, nullptr);
    });
  }

  void Process::read() noexcept {
    if (data.id <= 0 || (!stdout_fd && !stderr_fd)) {
      return;
    }

    stdout_stderr_thread = std::thread([this] {
      std::vector<pollfd> pollfds;
      std::bitset<2> fd_is_stdout;

      if (stdout_fd) {
        fd_is_stdout.set(pollfds.size());
        pollfds.emplace_back();
        pollfds.back().fd = fcntl(*stdout_fd, F_SETFL, fcntl(*stdout_fd, F_GETFL) | O_NONBLOCK) == 0 ? *stdout_fd : -1;
        pollfds.back().events = POLLIN;
      }

      if (stderr_fd) {
        pollfds.emplace_back();
        pollfds.back().fd = fcntl(*stderr_fd, F_SETFL, fcntl(*stderr_fd, F_GETFL) | O_NONBLOCK) == 0 ? *stderr_fd : -1;
        pollfds.back().events = POLLIN;
      }

      auto buffer = std::unique_ptr<char[]>(new char[config.buffer_size]);
      bool any_open = !pollfds.empty();
      ssc::StringStream ss;

      while (any_open && (poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), -1) > 0 || errno == EINTR)) {
        any_open = false;

        for (size_t i = 0; i < pollfds.size(); ++i) {
          if (!(pollfds[i].fd >= 0)) continue;

          if (pollfds[i].revents & POLLIN) {
            memset(buffer.get(), 0, config.buffer_size);
            const ssize_t n = ::read(pollfds[i].fd, buffer.get(), config.buffer_size);

            if (n > 0) {
              if (fd_is_stdout[i]) {
                auto b = ssc::String(buffer.get());
                auto parts = splitc(b, '\n');

                if (parts.size() > 1) {
                  for (int i = 0; i < parts.size() - 1; i++) {
                    ss << parts[i];

                    ssc::String s(ss.str());
                    read_stdout(s);

                    ss.str(ssc::String());
                    ss.clear();
                    ss.copyfmt(initial);
                  }
                  ss << parts[parts.size() - 1];
                } else {
                  ss << b;
                }
              } else {
                read_stderr(ssc::String(buffer.get()));
              }
            } else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
              pollfds[i].fd = -1;
              continue;
            }
          }

          if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            pollfds[i].fd = -1;
            continue;
          }

          any_open = true;
        }
      }
    });
  }

  void Process::close_fds() noexcept {
    if (stdout_stderr_thread.joinable()) {
      stdout_stderr_thread.join();
    }

    if (stdin_fd) {
      close_stdin();
    }

    if (stdout_fd) {
      if (data.id > 0) {
        close(*stdout_fd);
      }

      stdout_fd.reset();
    }

    if (stderr_fd) {
      if (data.id > 0) {
        close(*stderr_fd);
      }

      stderr_fd.reset();
    }
  }

  bool Process::write(const char *bytes, size_t n) {
    std::lock_guard<std::mutex> lock(stdin_mutex);

    if (stdin_fd) {
      ssc::String b(bytes);

      while (true && (b.size() > 0)) {
        int bytesWritten = ::write(*stdin_fd, b.c_str(), b.size());

        if (bytesWritten >= b.size()) {
          break;
        }

        b = b.substr(bytesWritten, b.size());
      }

      int bytesWritten = ::write(*stdin_fd, "\n", 1);
    }

    return false;
  }

  void Process::close_stdin() noexcept {
    std::lock_guard<std::mutex> lock(stdin_mutex);

    if (stdin_fd) {
      if (data.id > 0) {
        close(*stdin_fd);
      }

      stdin_fd.reset();
    }
  }

  void Process::kill(id_type id) noexcept {
    if (id <= 0) {
      return;
    }

    auto r = ::kill(-id, SIGINT);

    if (r != 0) {
      r = ::kill(-id, SIGTERM);

      if (r != 0) {
        r = ::kill(-id, SIGKILL);

        if (r != 0) {
          // @TODO: print warning
        }
      }
    }
  }
#endif
}
