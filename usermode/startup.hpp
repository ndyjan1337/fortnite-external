#include "render.hpp"

class startup1
{
public:

    void Startup()
    {

        //source passed by hus to me @ndyjan1337 the source was reworked... kinda well still its unleaked yet well now it is. Also props to that one guy that still uses this as 
        //his slotted cheat (reworked) :))))

        //i can tell this source does its job yes fine if you just test smth privat but not for selling since its just a eth repaste and most of features outdated

        //Input.init(); -> Old find your own one -> helper.hpp


        auto result = mem::find_driver(); //props dvl -> @mmcopymemory for driver: https://github.com/MmCopyMemory/fud-eac-driver

        screen_width = GetSystemMetrics(SM_CXSCREEN);
        screen_height = GetSystemMetrics(SM_CYSCREEN);

        while (windows == NULL)
        {
            XorS(wind, "Fortnite  ");
            windows = FindWindowA(0, wind.decrypt());
        }

        auto pid = mem::find_process(L"FortniteClient-Win64-Shipping.exe"); //props dvl -> @mmcopymemory for driver: https://github.com/MmCopyMemory/fud-eac-driver

        auto base_address = mem::find_image(); //props dvl -> @mmcopymemory for driver: https://github.com/MmCopyMemory/fud-eac-driver


        mem::CR3(); //props dvl -> @mmcopymemory for driver: https://github.com/MmCopyMemory/fud-eac-driver
        std::printf("base address -> %I64d\n", base_address); //props dvl -> @mmcopymemory for driver: https://github.com/MmCopyMemory/fud-eac-driver

        __int64 va_text = 0;
        for (int i = 0; i < 25; i++) {
            if ((read<__int32>(base_address + (i * 0x1000) + 0x250)) == 0x260E020B) {
                va_text = base_address + ((i + 1) * 0x1000);
                dynamic_uworld = offset::uworld + va_text;
            }
        }

        std::thread([&]() { for (;;) { g_cache->actors(); } }).detach();

        g_render->Overlay();
    }

}; static startup1* startup = new startup1;
