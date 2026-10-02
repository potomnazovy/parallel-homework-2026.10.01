#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <climits>

size_t recv(int& err, int rd, char* buf, size_t k)
{
  size_t r = 0;
  while (r < k)
  {
    err = read(rd, buf + r, k - r);
    if (err < 0)
    {
      break;
    }
    if (err == 0)
    {
      break;
    }

    r += err;
  }

  return r;
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "usage: child <fd>\n";
    return 1;
  }

  errno = 0;
  int err = 0;

  char* end = nullptr;
  long n = std::strtol(argv[1], &end, 10);

  if (errno != 0 || *end != '\0' || n <= 0 || n > INT_MAX)
  {
    std::cerr << "invalid fd: " << argv[1] << '\n';
    return 1;
  }

  int rd = (int)n;

  char buf[256];

  while (true)
  {
    err = 0;
    size_t got = recv(err, rd, buf, sizeof(buf));

    if (got > 0)
    {
      if (write(STDOUT_FILENO, buf, got) == -1)
      {
        std::cerr << "write: " << strerror(errno) << '\n';

        err = close(rd);
        if (err)
        {
          std::cerr << "close(rd): " << strerror(errno) << '\n';
        }

        return 1;
      }
    }

    if (err < 0)
    {
      std::cerr << "read: " << strerror(errno) << '\n';

      err = close(rd);
      if (err)
      {
        std::cerr << "close(rd): " << strerror(errno) << '\n';
      }

      return 1;
    }

    if (got < sizeof(buf))
    {
      break;
    }
  }

  err = close(rd);
  if (err)
  {
    std::cerr << "close(rd): " << strerror(errno) << '\n';
    return 1;
  }

  return 0;
}
