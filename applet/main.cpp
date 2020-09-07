#include "hekate.hpp"

#include <cstdio>
#include <cstdlib>
#include <switch.h>
#include <vector>

namespace {

    typedef void (*TuiCallback)(void *user);

    struct TuiItem {
        std::string text;
        TuiCallback cb;
        void *user;
        bool selectable;
    };

    void ConfigCallback(void *user) {
        auto config = reinterpret_cast<Hekate::BootConfig *>(user);

        Hekate::RebootToConfig(*config);
    }

    void UmsCallback(void *user) {
        Hekate::RebootToUMS(Hekate::UmsTarget_Sd);

        (void)user;
    }

}

extern "C" void userAppInit(void) {
    splInitialize();
}

extern "C" void userAppExit(void) {
    splExit();
}

int main(int argc, char **argv) {
    auto config_list = Hekate::LoadBootConfigList();
    std::vector<TuiItem> items;

    items.reserve(config_list.size() + 3);

    items.emplace_back("Configs", nullptr, nullptr, false);
    for (auto &entry : config_list)
        items.emplace_back(entry.name, ConfigCallback, &entry, true);

    items.emplace_back("Miscellaneous", nullptr, nullptr, false);
    items.emplace_back("Reboot to UMS", UmsCallback, nullptr, true);

    std::size_t index = 0;

    for (auto &item : items) {
        if (item.selectable)
            break;

        index++;
    }

    PrintConsole *console = consoleInit(nullptr);

    while (appletMainLoop()) {
        {
            u64 kDown = 0;

            hidScanInput();

            for (int controller = 0; controller < 10; controller++)
                kDown |= hidKeysDown(static_cast<HidControllerID>(controller));

            if ((kDown & (KEY_PLUS | KEY_B | KEY_L)))
                break;

            if ((kDown & KEY_A)) {
                auto &item = items[index];

                if (item.selectable && item.cb)
                    item.cb(item.user);

                break;
            }

            if ((kDown & KEY_MINUS)) {
                Hekate::RebootDefault();
            }

            if ((kDown & KEY_DOWN) && (index + 1) < items.size()) {
                for (std::size_t i = index; i < items.size(); i++) {
                    if (!items[i + 1].selectable)
                        continue;

                    index = i + 1;
                    break;
                }
            }

            if ((kDown & KEY_UP) && index > 0) {
                for (std::size_t i = index; i > 0; i--) {
                    if (!items[i - 1].selectable)
                        continue;

                    index = i - 1;
                    break;
                }
            }
        }

        consoleClear();

        printf("Studious Pancake\n----------------\n");

        for (std::size_t i = 0; i < items.size(); i++) {
            auto &item    = items[i];
            bool selected = (i == index);

            if (!item.selectable)
                console->flags |= CONSOLE_COLOR_FAINT;

            printf("%c %s\n", selected ? '>' : ' ', item.text.c_str());

            if (!item.selectable)
                console->flags &= ~CONSOLE_COLOR_FAINT;
        }

        consoleUpdate(nullptr);
    }

    consoleExit(nullptr);

    (void)argc;
    (void)argv;

    return EXIT_SUCCESS;
}