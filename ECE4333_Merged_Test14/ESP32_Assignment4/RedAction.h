#pragma once
// Area is the existing colour detector's reported pixel count, not box width.
static bool redActionTriggered(bool seen,int pixels,int minimum){return seen&&pixels>=minimum;}
