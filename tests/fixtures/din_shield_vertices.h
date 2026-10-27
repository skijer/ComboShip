// Representative vertices from the existing curved surface and flame rim.
// Keep distinct arrays so a ground-clearance check must include both resources.
static Vtx shieldSurfaceVertices[] = {
    { .v = { .ob = { -1000, 0, 0 } } },
    { .v = { .ob = { 1000, 0, 0 } } },
    { .v = { .ob = { 0, -1000, 0 } } },
    { .v = { .ob = { 0, 1000, -170 } } },
};
static Vtx shieldRimVertices[] = {
    { .v = { .ob = { -1100, 0, -12 } } },
    { .v = { .ob = { 1100, 0, -12 } } },
    { .v = { .ob = { 0, -1100, -12 } } },
    { .v = { .ob = { 0, 1199, -12 } } },
};
