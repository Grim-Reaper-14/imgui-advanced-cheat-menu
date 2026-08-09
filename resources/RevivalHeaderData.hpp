#pragma once

#include <array>

namespace RevivalHeaderData {
    inline constexpr std::array<const char*, 20> Parts = {{
#include "RevivalHeaderPart00.inc"
#include "RevivalHeaderPart01.inc"
#include "RevivalHeaderPart02.inc"
#include "RevivalHeaderRest00.inc"
#include "RevivalHeaderRest01.inc"
#include "RevivalHeaderRest02.inc"
#include "RevivalHeaderRest03.inc"
#include "RevivalHeaderRest04a.inc"
#include "RevivalHeaderRest04b.inc"
#include "RevivalHeaderRest05a.inc"
#include "RevivalHeaderRest05b.inc"
#include "RevivalHeaderRest06.inc"
#include "RevivalHeaderRest07.inc"
#include "RevivalHeaderRest08.inc"
#include "RevivalHeaderRest09a.inc"
#include "RevivalHeaderRest09b.inc"
#include "RevivalHeaderRest10a.inc"
#include "RevivalHeaderRest10b.inc"
#include "RevivalHeaderRest11a.inc"
#include "RevivalHeaderRest11b.inc"
    }};
}
