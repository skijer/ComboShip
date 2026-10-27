"""Keep legacy equipment fixtures on their explicit enhancement-off path.

The chooser itself is exercised with real game headers and production helpers by
scripts/diagnostics/sword_element_kaleido_tests.py. These narrower ownership,
preview and description fixtures continue executing the unmodified equipment
cursor/draw bodies with the optional chooser unavailable.
"""

import re


def enhancement_off_boundary(pause_source):
    if "static SwordElementMenu sSwordElementMenu;" not in pause_source:
        return ""  # Historical --source-ref fixtures predate the chooser.

    declarations = re.search(
        r"^static SwordElementMenu sSwordElementMenu;\n"
        r"(?:static [^;\n]+ sSwordElement\w+;\n)+",
        pause_source,
        re.M,
    )
    if not declarations:
        raise ValueError("missing production sword-element menu declarations")

    return (
        '#include "mm/src/overlays/kaleido_scope/ovl_kaleido_scope/sword_element_kaleido.h"\n'
        + declarations.group(0)
        + r'''
// This fixture models disabled sword enhancements; an A press follows native equipment logic.
static u8 KaleidoEquip_OpenSwordElement(PlayState*) {
    check(sSwordElementMenu.phase == SWORD_ELEMENT_MENU_CLOSED,
          "disabled sword enhancement cannot own the equipment cursor");
    return false;
}
void SwordElementKaleido_Draw(PlayState*, const SwordElementMenu*, void*) {
    check(false, "disabled sword enhancement must not submit chooser graphics");
}
'''
    )
