#include "srcs/ext_f.hpp"

// ./ircserv <Port> <Password>
// Port: Listening port
// Password: Server password
int main(int ac, char **av)
{
    if (ac != 3)
    {
        std::cerr << "Usage: " << av[0] << " <Port> <Password>" << std::endl;
        return 1;
    }
    // run_server(av[1], av[2]);
    return 0;
}