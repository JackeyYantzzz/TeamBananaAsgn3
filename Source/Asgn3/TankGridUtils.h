#pragma once

#include "CoreMinimal.h"

// Helpers for the Battle City style tile grid on the X-Z plane.
namespace TankGrid
{
    // Centre of the grid tile that contains Coord (one axis)
    inline double TileCenter(double Coord, double Origin, double Size)
    {
        return Origin + (FMath::FloorToDouble((Coord - Origin) / Size) + 0.5) * Size;
    }
}
