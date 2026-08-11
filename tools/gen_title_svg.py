#!/usr/bin/env python3
"""Generate data/Assets/UI/Title.svg -- the game's title logo, "VIGIL".

Imported as a traktor.spark.MovieAsset (UI/Title) and instantiated by
Scripts/Startup as the export "MC_Title". Same importer rules as the HUD art, so
see tools/gen_hud_svg.py for the long version; the short version is that every
drawable has to live inside a named child sprite, coordinates end up absolute in
movie space, and numbers never start with a bare '.'.

The letters are drawn as polygons rather than <text>: a font would have to be
listed in the MovieAsset to import at all, and a title wants to be a shape we
control anyway. They are chiselled geometric capitals -- flat faces, one cut
corner size, no curves except the G's octagonal bowl -- which is both what the
importer is good at and what a game about consecrating stone should look like.

Lit from above by the flame, so each letter is drawn three times: a dropped
shadow, a dark body, and a lighter face shifted up, which fakes a bevel without
needing a gradient the importer does not support.
"""

import math
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "data", "Assets", "UI", "Title.svg")

# ---------------------------------------------------------------- geometry --

W, H = 520, 226                 # document / widget box

CAP = 104                       # cap height
STEM = 22                       # stroke weight
GAP = 15                        # letter spacing
TOP = 94                        # top of the capitals

BEVEL = 3                       # how far the lit face is lifted off the body
DROP_X, DROP_Y = 4, 6           # shadow offset

FLAME_X, FLAME_Y = W / 2, 46    # flame centre; clear of the caps below it
FLAME_W, FLAME_H = 21, 38

RULE_Y = TOP + CAP + 14         # the hairline under the word
RULE_W, RULE_H = 300, 2

# Widths of each glyph box. The G is a touch wider than the V so the round bowl
# does not read as smaller than the flat letters beside it.
WIDTH = { "V": 84, "I": 28, "G": 88, "L": 68 }

# ----------------------------------------------------------------- palette --

BLACK = "#000000"
STONE_FACE = "#ded9d0"          # lit top face
STONE_BODY = "#8e8981"          # the side the flame does not reach
FLAME = "#ffb347"               # the amber the compass needle and relics use
FLAME_CORE = "#ffe8bd"
GLOW = "#ff8a2a"
RULE = "#6a7480"


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


def circle(cx, cy, r, fill, opacity=None):
    return ('<circle cx="%s" cy="%s" r="%s" style="%s"/>'
            % (n(cx), n(cy), n(r), style(fill, opacity)))


def path(d, fill, opacity=None):
    return '<path d="%s" style="%s"/>' % (d, style(fill, opacity))


def sprite(sid, body, indent=4):
    pad = " " * indent
    inner = "\n".join(pad + "  " + b for b in body)
    return ('%s<g id="%s" inkscape:label="%s" traktor:sprite="1">\n%s\n%s</g>'
            % (pad, sid, sid, inner, pad))


# ---------------------------------------------------------------- glyphs ----
#
# Each returns a list of point-lists in a box (x, y) .. (x + width, y + CAP).

def glyph_V(x, y, w):
    """Two strokes meeting in a point. The inner apex stops short of the outer
    one so the join has the thickness the rest of the letter has."""
    inner = STEM * 1.18         # the diagonals are wider than the stem at the top
    return [[
        (x, y),
        (x + inner, y),
        (x + w / 2, y + CAP - STEM * 0.95),
        (x + w - inner, y),
        (x + w, y),
        (x + w / 2, y + CAP),
    ]]


def glyph_I(x, y, w):
    cx = x + w / 2
    return [[
        (cx - STEM / 2, y),
        (cx + STEM / 2, y),
        (cx + STEM / 2, y + CAP),
        (cx - STEM / 2, y + CAP),
    ]]


def glyph_L(x, y, w):
    return [[
        (x, y),
        (x + STEM, y),
        (x + STEM, y + CAP - STEM),
        (x + w, y + CAP - STEM),
        (x + w, y + CAP),
        (x, y + CAP),
    ]]


def glyph_G(x, y, w):
    """An octagonal bowl, open on the right, with a spur across the opening.

    Built as overlapping quads -- one per side of the ring -- rather than as a
    single clever outline. The importer cannot subtract one shape from another,
    and every piece here is the same colour, so letting them overlap unions them
    for free. Tracing the ring as one closed path is what it looks like it wants,
    and it is how the first attempt lost the whole upper right of the letter."""
    c = CAP * 0.26              # outer corner cut
    ci = c * 0.62               # ...and the inner one
    s = STEM

    # Outer and inner octagons, both clockwise from the top left corner.
    O = [(x + c, y), (x + w - c, y), (x + w, y + c), (x + w, y + CAP - c),
         (x + w - c, y + CAP), (x + c, y + CAP), (x, y + CAP - c), (x, y + c)]
    I = [(x + ci, y + s), (x + w - ci, y + s), (x + w - s, y + ci), (x + w - s, y + CAP - ci),
         (x + w - ci, y + CAP - s), (x + ci, y + CAP - s), (x + s, y + CAP - ci), (x + s, y + ci)]

    mouth = y + CAP * 0.30      # where the upper terminal stops
    barTop = y + CAP * 0.50     # ...and the spur picks up again
    barBot = barTop + s * 0.9
    barIn = x + w * 0.46        # how far the spur reaches back into the bowl

    # Every side of the ring except the right one, which the mouth interrupts.
    quads = [[O[k], O[(k + 1) % 8], I[(k + 1) % 8], I[k]] for k in (0, 1, 3, 4, 5, 6, 7)]

    quads.append([O[2], (x + w, mouth), (x + w - s, mouth), I[2]])          # upper terminal
    quads.append([(x + w, barBot), O[3], I[3], (x + w - s, barBot)])        # below the spur
    quads.append([(barIn, barTop), (x + w, barTop), (x + w, barBot), (barIn, barBot)])
    return quads


GLYPHS = { "V": glyph_V, "I": glyph_I, "G": glyph_G, "L": glyph_L }
WORD = "VIGIL"


def word_polys(dx, dy):
    """Every polygon of the whole word, offset by (dx, dy)."""
    total = sum(WIDTH[ch] for ch in WORD) + GAP * (len(WORD) - 1)
    x = (W - total) / 2
    out = []
    for ch in WORD:
        w = WIDTH[ch]
        for p in GLYPHS[ch](x + dx, TOP + dy, w):
            out.append(p)
        x += w + GAP
    return out


# ----------------------------------------------------------------- flame ----

def flame(cx, cy, fw, fh):
    """A symmetric teardrop: tip at the top, shoulders low, closing on itself."""
    def p(x, y):
        return "%s %s" % (n(x), n(y))
    return ("M %s Q %s %s Q %s %s Q %s %s Q %s %s Z" % (
        p(cx, cy - fh),
        p(cx + fw * 0.95, cy - fh * 0.10), p(cx + fw * 0.72, cy + fh * 0.42),
        p(cx + fw * 0.50, cy + fh), p(cx, cy + fh),
        p(cx - fw * 0.50, cy + fh), p(cx - fw * 0.72, cy + fh * 0.42),
        p(cx - fw * 0.95, cy - fh * 0.10), p(cx, cy - fh)))


# ------------------------------------------------------------------ title ---

def title():
    layers = []

    # Warm haze behind the flame. Flat fills only, so it is a stack of rings at
    # low opacity rather than a gradient -- enough of them that the steps between
    # them stop reading as bands.
    layers.append(sprite("glow", [
        circle(FLAME_X, FLAME_Y + (14 - k) * 0.5, 20 + k * 7, GLOW, 0.014)
        for k in range(14, 0, -1)
    ]))

    layers.append(sprite("flame", [
        path(flame(FLAME_X, FLAME_Y, FLAME_W, FLAME_H), FLAME),
        path(flame(FLAME_X, FLAME_Y + FLAME_H * 0.30, FLAME_W * 0.44, FLAME_H * 0.52),
             FLAME_CORE),
    ]))

    layers.append(sprite("shadow", [
        poly(p, BLACK, 0.55) for p in word_polys(DROP_X, DROP_Y)
    ]))
    layers.append(sprite("body", [
        poly(p, STONE_BODY) for p in word_polys(0, 0)
    ]))
    layers.append(sprite("face", [
        poly(p, STONE_FACE) for p in word_polys(0, -BEVEL)
    ]))

    layers.append(sprite("rule", [
        rect((W - RULE_W) / 2, RULE_Y, RULE_W, RULE_H, RULE, 0.75),
    ]))

    return layers


def main():
    doc = []
    # NOTE: no double hyphen anywhere inside these comments; that is illegal in
    # XML and the importer's parser rejects the whole document over it.
    doc.append('<?xml version="1.0" encoding="UTF-8" standalone="no"?>')
    doc.append('<!-- Title logo, generated by tools/gen_title_svg.py -->')
    doc.append('<svg width="%d" height="%d" viewBox="0 0 %d %d" version="1.1"'
               % (W, H, W, H))
    doc.append('   xmlns="http://www.w3.org/2000/svg"')
    doc.append('   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"')
    doc.append('   xmlns:traktor="http://traktor/svg">')
    doc.append('  <g id="MC_Title" inkscape:label="MC_Title"')
    doc.append('     inkscape:groupmode="layer" traktor:sprite="1"')
    doc.append('     style="display:inline">')
    doc.extend(title())
    doc.append('  </g>')
    doc.append('</svg>')

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(os.path.normpath(OUT), "w") as f:
        f.write("\n".join(doc) + "\n")
    print("wrote", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
