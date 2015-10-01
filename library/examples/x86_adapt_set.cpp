#include <iostream>
#include <string>
#include <assert.h>

extern "C"
{
#include "x86_adapt.h"
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "usage: " << argv[0] << " KNOB VALUE CPU" << std::endl;
        return 0;
    }

    std::string knob = argv[1];
    auto value = atol(argv[2]);
    auto cpu = atoi(argv[3]);

    std::cout << "Trying to set knob " << knob << " to value " << value << " on cpu " << cpu << std::endl;

    auto ret = x86_adapt_init();
    if (ret)
    {
        std::cerr << "x86_adapt_init failed: " << ret << std::endl;
        return -1;
    }

    int nr_cis = x86_adapt_get_number_cis(X86_ADAPT_CPU);
    assert(nr_cis >= 0);

    int index;
    for (index = 0; index < nr_cis; index++)
    {
        struct x86_adapt_configuration_item* item;
        assert (x86_adapt_get_ci_definition(X86_ADAPT_CPU, index, &item) == 0);

        if (knob == item->name)
        {
            break;
        }
    }
    if (index == nr_cis)
    {
        std::cerr << "Could not find knob." << std::endl;
        return -1;
    }

    auto fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
    if (fd < 0)
    {
        std::cerr << "Could not get device FD: " << fd << std::endl;
        return -1;
    }
    ret = x86_adapt_set_setting(fd, index, value);
    if (ret < 0)
    {
        std::cerr << "Could not set value: " << fd << std::endl;
        return -1;
    }
    ret = x86_adapt_put_device(X86_ADAPT_CPU, cpu);
    if (ret != 0)
    {
        std::cerr << "Could not close device FD: " << ret << std::endl;
        return -1;
    }

    x86_adapt_finalize();
    return 0;
}
