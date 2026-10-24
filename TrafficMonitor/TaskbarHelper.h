#pragma once
#include <vector>
class CTaskbarHelper
{
public:
    //Get the taskbars on all secondary monitors and make sure they are in monitor order
    static void GetAllSecondaryDisplayTaskbar(std::vector<HWND>& secondary_taskbars);

    //Get the number of monitors
    static int GetDisplayNum();

    //Get the number of taskbars on the secondary monitor
    static int GetSecondaryTaskbarNum();
};

