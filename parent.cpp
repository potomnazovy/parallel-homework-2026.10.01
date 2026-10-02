#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdio>

size_t send(int& err, int wr, const char* buf, size_t k)
{
  size_t r = 0;
  while (r < k)
  {
    err = write(wr, buf + r, k - r);
    if (err < 0)
    {
      break;
    }

    r += err;
  }

  return r;
}

int cleanup(pid_t pid, int wr)
{
  int rc = 0;
  int err = close(wr);

  if (err)
  {
    std::cerr << "close(wr): " << strerror(errno) << '\n';
    rc = 1;
  }

  pid_t p = waitpid(pid, 0, 0);
  if (p == -1)
  {
    std::cerr << "waitpid: " << strerror(errno) << '\n';
    rc = 1;
  }
  else if (p != pid)
  {
    std::cerr << "waitpid: unexpected pid\n";
    rc = 1;
  }

  return rc;
}

int main()
{
  int pps[2] = {};
  int err = pipe(pps);

  if (err)
  {
    std::cerr << "pipe: " << strerror(errno) << '\n';
    return 1;
  }

  int rd = pps[0];
  int wr = pps[1];

  pid_t pid = fork();
  if (pid < 0)
  {
    std::cerr << "fork: " << strerror(errno) << '\n';
    return 1;
  }

  if (pid == 0)
  {
    err = close(wr);
    if (err)
    {
      std::cerr << "close(wr): " << strerror(errno) << '\n';
      _exit(1);
    }

    char p[100] = {};
    int n = snprintf(p, sizeof(p), "%d", rd);

    if (n < 0)
    {
      std::cerr << "snprintf: " << strerror(errno) << '\n';
      _exit(1);
    }
    if (n >= (int)sizeof(p))
    {
      std::cerr << "snprintf: truncated (need " << n << ")\n";
      _exit(1);
    }

    execl("child", "child", p, NULL);

    std::cerr << "execl: " << strerror(errno) << '\n';
    _exit(1);
  }

  err = close(rd);
  if (err)
  {
    std::cerr << "close(rd): " << strerror(errno) << '\n';
    return cleanup(pid, wr);
  }

  std::string str;
  if (!std::getline(std::cin, str))
  {
    std::cerr << "stdin: failed to read\n";
    return cleanup(pid, wr);
  }

  str += "\n";

  err = 0;
  errno = 0;

  bool send_failed = false;

  size_t sent = send(err, wr, str.c_str(), str.size());
  if (sent != str.size())
  {
    std::cerr << "send: wrote " << sent << " of " << str.size() << " bytes";
    if (errno != 0)
    {
      std::cerr << ": " << strerror(errno);
    }

    std::cerr << '\n';
    send_failed = true;
  }

  int rc = cleanup(pid, wr);
  return (send_failed || rc) ? 1 : 0;
}
