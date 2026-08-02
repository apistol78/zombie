#!/usr/bin/env python3
"""Generate data/Assets/UI/Hud.svg -- the art for the in-game HUD widgets.

The file is a normal SVG and can be edited in Inkscape afterwards; this script
just gets the geometry (chamfered plates, hexagons, the heart and the gem)
exact and repeatable.

Spark's SVG importer (traktor/code/Spark/Editor/ConvertSvg.cpp) turns every
element carrying a "traktor:sprite" attribute into a sprite character:

  * a top level one is *exported* under its id, which is what
    Widget._createResource(parent, "MC_...") instantiates;
  * a nested one is placed on its parent's frame under its id, which is what
    mc:getMember("...") returns -- those are the parts the widget scripts
    animate.

Two rules the importer imposes on the authoring:

  * Shapes that sit directly in a sprite are placed on the parent frame *after*
    its child sprites, so they would cover them. Every drawable therefore lives
    inside a named child sprite.
  * Coordinates end up absolute in movie space (the viewBox is mapped onto
    width x height), so each exported widget is authored at the origin. They
    consequently overlap in the document, exactly as the engine's own
    UiKit2.svg does; the layers are display:none except the first so the file
    stays editable.

Numbers are always written with a leading digit -- the importer's number
scanner skips a leading '.' and would read ".5" as 5.
"""

import math
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "data", "Assets", "UI", "Hud.svg")

# ---------------------------------------------------------------- geometry --

W, H = 460, 56                  # document, and the health bar's widget box

BADGE_X, BADGE_Y = 28, 28       # heart badge centre
BADGE_R = 24

BAR_X, BAR_Y = 58, 12           # bar plate
BAR_W, BAR_H = 396, 32
CUT = 8

TRACK_X, TRACK_Y = 62, 16       # track / fill
TRACK_W, TRACK_H = 388, 24

TC_W = 180                      # treasure counter widget box (fixed width, so
TC_PLATE_Y = 10                 # the plate never has to be redrawn or relaid)
TC_PLATE_H = 36
GEM_X, GEM_Y = 28, 28
GEM_S = 30
SPARK_X, SPARK_Y = 21, 21

# ------------------------------------------------------------------ palette --

EDGE = "#050508"                # outermost dark line of a plate
BEVEL = "#5c5f6b"               # lit rim
FACE = "#1e1e24"                # plate face
TRACK = "#0a0a0e"
BLACK = "#000000"
WHITE = "#ffffff"
WARN = "#ff2a1a"
GHOST = "#b82316"
HEART = "#de2129"
HEART_RIM = "#4c060c"
HEART_HI = "#ff9e99"
GEM = "#f2bf1a"                 # matches the Pickup material's gold
GEM_TABLE = "#ffedb0"
GEM_LEFT = "#c79309"
GEM_RIGHT = "#8c5c07"
GEM_RIM = "#42280a"

# The fill is drawn greyscale and tinted per frame by the widget's colour
# transform, so these are shades, not colours.
FILL_BODY = "#a8a8a8"
FILL_TOP = "#e6e6e6"
FILL_BOTTOM = "#5c5c5c"


def n(v):
    """Number, never with a leading '.' and never in scientific notation."""
    s = "%.2f" % v
    s = s.rstrip("0").rstrip(".")
    return s if s not in ("", "-") else "0"


def pts(points):
    return " ".join("%s,%s" % (n(x), n(y)) for x, y in points)


def style(fill, opacity=None):
    s = "fill:%s" % fill
    if opacity is not None:
        s += ";fill-opacity:%s" % n(opacity)
    return s


def poly(points, fill, opacity=None):
    return '<polygon points="%s" style="%s"/>' % (pts(points), style(fill, opacity))


def rect(x, y, w, h, fill, opacity=None):
    return ('<rect x="%s" y="%s" width="%s" height="%s" style="%s"/>'
            % (n(x), n(y), n(w), n(h), style(fill, opacity)))


def cut_rect(x, y, w, h, c):
    """Rectangle with chamfered corners -- the plated HUD look."""
    return [
        (x + c, y), (x + w - c, y),
        (x + w, y + c), (x + w, y + h - c),
        (x + w - c, y + h), (x + c, y + h),
        (x, y + h - c), (x, y + c),
    ]


def hexagon(cx, cy, r):
    return [(cx + math.cos(math.radians(i * 60 + 30)) * r,
             cy + math.sin(math.radians(i * 60 + 30)) * r) for i in range(6)]


def heart(cx, cy, s):
    """Heart centred on (cx, cy), s ~ half width. Six quadratics closing on
    the start point: up the left flank, over both lobes through the centre
    dip, then back down to the tip."""
    def p(dx, dy):
        return "%s %s" % (n(cx + dx * s), n(cy + dy * s))
    return ("M %s Q %s %s Q %s %s Q %s %s Q %s %s Q %s %s Q %s %s Z" % (
        p(0, 0.95),
        p(-1.05, 0.15), p(-1.05, -0.35),
        p(-1.05, -0.95), p(-0.5, -0.95),
        p(-0.18, -0.95), p(0, -0.5),
        p(0.18, -0.95), p(0.5, -0.95),
        p(1.05, -0.95), p(1.05, -0.35),
        p(1.05, 0.15), p(0, 0.95)))


def gem(cx, cy, s):
    """Brilliant cut: table across the top, girdle at the widest point,
    pavilion down to the tip, with three facet tones over the silhouette."""
    hw, top, gird, tip, tw = 0.5 * s, -0.42 * s, -0.15 * s, 0.5 * s, 0.27 * s

    def o(points):
        return [(cx + x, cy + y) for x, y in points]

    return [
        poly(o([(-tw - 1.2, top - 1.2), (tw + 1.2, top - 1.2),
                (hw + 1.2, gird), (0, tip + 1.4), (-hw - 1.2, gird)]), GEM_RIM),
        poly(o([(-tw, top), (tw, top), (hw, gird), (0, tip), (-hw, gird)]), GEM),
        poly(o([(-tw, top), (tw, top), (tw * 1.55, gird), (-tw * 1.55, gird)]), GEM_TABLE),
        poly(o([(-hw, gird), (-tw * 1.55, gird), (0, tip)]), GEM_LEFT),
        poly(o([(tw * 1.55, gird), (hw, gird), (0, tip)]), GEM_RIGHT),
    ]


def star(cx, cy, r):
    k = r * 0.2
    return poly([(cx, cy - r), (cx + k, cy - k), (cx + r, cy), (cx + k, cy + k),
                 (cx, cy + r), (cx - k, cy + k), (cx - r, cy), (cx - k, cy - k)],
                WHITE, 0.9)


def sprite(sid, body, indent=4):
    pad = " " * indent
    inner = "\n".join(pad + "  " + b for b in body)
    return ('%s<g id="%s" inkscape:label="%s" traktor:sprite="1">\n%s\n%s</g>'
            % (pad, sid, sid, inner, pad))


# ------------------------------------------------------------- health bar ---

def health_bar():
    """Layers back to front. Only 'ghost', 'fill' and 'edge' change length;
    'warn', 'flash' and 'heart' are animated through alpha/scale alone."""
    plate = cut_rect(BAR_X, BAR_Y, BAR_W, BAR_H, CUT)

    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(BAR_X, BAR_Y + 3, BAR_W, BAR_H, CUT), BLACK, 0.45),
        poly(hexagon(BADGE_X, BADGE_Y + 3, BADGE_R), BLACK, 0.45),
    ]))

    # Sits under the plate, a few pixels proud of it, so only a rim shows.
    layers.append(sprite("warn", [
        poly(cut_rect(BAR_X - 4, BAR_Y - 4, BAR_W + 8, BAR_H + 8, CUT + 3), WARN, 0.35),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R + 3.5), WARN, 0.35),
        poly(cut_rect(BAR_X - 2, BAR_Y - 2, BAR_W + 4, BAR_H + 4, CUT + 1), WARN),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R + 1.5), WARN),
    ]))

    # Dark outline, lit bevel, then a face inset further from the top than the
    # bottom so the bevel reads as lit from above.
    layers.append(sprite("plate", [
        poly(plate, EDGE),
        poly(cut_rect(BAR_X + 1, BAR_Y + 1, BAR_W - 2, BAR_H - 2, CUT - 1), BEVEL),
        poly(cut_rect(BAR_X + 2, BAR_Y + 3, BAR_W - 4, BAR_H - 4, CUT - 2), FACE),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R), EDGE),
        poly(hexagon(BADGE_X, BADGE_Y, BADGE_R - 1.5), BEVEL),
        poly(hexagon(BADGE_X, BADGE_Y + 0.5, BADGE_R - 3.5), FACE),
    ]))

    layers.append(sprite("track", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, TRACK),
        rect(TRACK_X, TRACK_Y, TRACK_W, 2, BLACK, 0.55),
    ]))

    layers.append(sprite("ghost", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, GHOST, 0.92),
    ]))

    layers.append(sprite("fill", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, FILL_BODY),
        rect(TRACK_X, TRACK_Y + 1, TRACK_W, TRACK_H * 0.38, FILL_TOP),
        rect(TRACK_X, TRACK_Y + TRACK_H * 0.74, TRACK_W, TRACK_H * 0.26, FILL_BOTTOM),
    ]))

    # Rides the head of the fill; translated, never scaled, so it stays crisp.
    layers.append(sprite("edge", [
        rect(TRACK_X, TRACK_Y, 3, TRACK_H, WHITE),
    ]))

    pips = [rect(TRACK_X + round(TRACK_W * i / 10) - 1, TRACK_Y, 2, TRACK_H, BLACK, 0.45)
            for i in range(1, 10)]
    pips.append(rect(TRACK_X, TRACK_Y, TRACK_W, 1, BLACK, 0.55))
    pips.append(rect(TRACK_X, TRACK_Y + TRACK_H - 1, TRACK_W, 1, BLACK, 0.55))
    layers.append(sprite("pips", pips))

    layers.append(sprite("flash", [
        rect(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, WHITE),
    ]))

    layers.append(sprite("heart", [
        '<path d="%s" style="%s"/>' % (heart(BADGE_X, BADGE_Y, 14.5), style(HEART_RIM)),
        '<path d="%s" style="%s"/>' % (heart(BADGE_X, BADGE_Y, 13), style(HEART)),
        '<ellipse cx="%s" cy="%s" rx="3" ry="2.2" style="%s"/>'
        % (n(BADGE_X - 4.6), n(BADGE_Y - 5.4), style(HEART_HI, 0.8)),
    ]))

    return layers


# -------------------------------------------------------- treasure counter --

def treasure_counter():
    layers = []

    layers.append(sprite("shadow", [
        poly(cut_rect(2, TC_PLATE_Y + 3, TC_W - 4, TC_PLATE_H, 7), BLACK, 0.45),
    ]))

    layers.append(sprite("plate", [
        poly(cut_rect(2, TC_PLATE_Y, TC_W - 4, TC_PLATE_H, 7), EDGE),
        poly(cut_rect(3, TC_PLATE_Y + 1, TC_W - 6, TC_PLATE_H - 2, 6), BEVEL),
        poly(cut_rect(4, TC_PLATE_Y + 3, TC_W - 8, TC_PLATE_H - 4, 5), FACE),
    ]))

    layers.append(sprite("gem", gem(GEM_X, GEM_Y, GEM_S)))
    layers.append(sprite("sparkle", [star(SPARK_X, SPARK_Y, 7)]))

    layers.append(sprite("glow", [
        poly(cut_rect(3, TC_PLATE_Y + 1, TC_W - 6, TC_PLATE_H - 2, 6), WHITE),
    ]))

    return layers


def main():
    doc = []
    # NOTE: no double hyphen anywhere inside these comments; that is illegal in
    # XML and the importer's parser rejects the whole document over it.
    doc.append('<?xml version="1.0" encoding="UTF-8" standalone="no"?>')
    doc.append('<!-- HUD widget art, generated by tools/gen_hud_svg.py -->')
    doc.append('<!-- Each top level layer is one Spark export: the widget')
    doc.append('     scripts instantiate it by id and animate the named child')
    doc.append('     sprites inside it. Both are authored at the origin and so')
    doc.append('     overlap; toggle the layers to edit them. -->')
    doc.append('<svg width="%d" height="%d" viewBox="0 0 %d %d" version="1.1"'
               % (W, H, W, H))
    doc.append('   xmlns="http://www.w3.org/2000/svg"')
    doc.append('   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"')
    doc.append('   xmlns:traktor="http://traktor/svg">')

    doc.append('  <g id="MC_HealthBar" inkscape:label="MC_HealthBar"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:inline">')
    doc.extend(health_bar())
    doc.append('  </g>')

    doc.append('  <g id="MC_TreasureCounter" inkscape:label="MC_TreasureCounter"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:none">')
    doc.extend(treasure_counter())
    doc.append('  </g>')

    doc.append('</svg>')

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(os.path.normpath(OUT), "w") as f:
        f.write("\n".join(doc) + "\n")
    print("wrote", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
