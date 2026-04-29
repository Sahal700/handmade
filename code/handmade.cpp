#include "handmade.h"


internal void RenderGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {

        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {
            uint8 BlUE = X + XOffset;
            uint8 Green = Y + YOffset;

            *Pixel++ = (Green << 8 | BlUE);
        }
        Row += Buffer->Pitch;
    }
}

internal void GameUpdateRende(game_offscreen_buffer *Buffer, int XOffset, int YOffset){
    RenderGradient( Buffer, XOffset, YOffset);
}