#include "ResourceLoader.hpp"

#include <Windows.h>

namespace ResourceLoader {
    std::pair<const void*, std::size_t> getBinary(int resourceId) {
        HMODULE module = GetModuleHandleW(nullptr);
        if (!module)
            return { nullptr, 0 };

        HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        if (!resource)
            return { nullptr, 0 };

        HGLOBAL loaded = LoadResource(module, resource);
        if (!loaded)
            return { nullptr, 0 };

        const DWORD size = SizeofResource(module, resource);
        const void* data = LockResource(loaded);
        if (!data || size == 0)
            return { nullptr, 0 };

        return { data, static_cast<std::size_t>(size) };
    }
}
