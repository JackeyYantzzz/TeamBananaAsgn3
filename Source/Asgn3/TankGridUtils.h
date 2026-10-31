#pragma once

#include "CoreMinimal.h"

namespace TankGrid
{
    inline double TileCenter(double Coord, double Origin, double Size)
    {
        return Origin + (FMath::FloorToDouble((Coord - Origin) / Size) + 0.5) * Size;
    }
}
