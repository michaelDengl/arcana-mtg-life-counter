# Symbol provenance

The small `mtg_symbols` LVGL font is generated from prepared open fonts, not AI-created imitations. The bitmap subset is checked in so building does not require Node or network access for icons.

| Tracker | Artwork |
| --- | --- |
| Poison | Phyrexian mark, Mana U+E618 |
| Energy | MTG energy glyph, Mana U+E907 |
| Rad | Radiation counter glyph, Mana U+EA50 |
| Shield | Arena shield counter glyph, Mana U+E9C3 |
| Experience | Commander 2015 emblem, Keyrune U+E900 |
| Storm | Original cloud-and-lightning drawing, labeled Storm |
| Treasures | Original chest drawing |

No standardized official Treasure or Storm glyph was verified; those two drawings are not presented as official Wizards assets. Experience uses the recognizable Commander 2015 emblem, not a claim that the rules define a universal experience symbol.

Sources: [Mana](https://github.com/andrewgioia/mana) commit `6ca9e696d3bda2519dcf3eebd96598f84ad9ddd8` and [Keyrune](https://github.com/andrewgioia/keyrune) commit `256add455a043462f7b2bfa224ca71bfc66b62ab`. These are community font reproductions of MTG imagery. The prepared fonts are under SIL OFL 1.1; font metadata and the license text are included in `src/assets/OFL-MTG-Symbols.txt`, with Keyrune's additional license notice alongside it. The generated subset is named `mtg_symbols`.

Regeneration uses `lv_font_conv` 1.5.3 at 26px, 4bpp with the four Mana ranges above and the Commander 2015 glyph from Keyrune. The gear and reset marks use LVGL's built-in symbol font. Other tool icons are native LVGL drawings.

The heart and skull are Font Awesome 5 solid heart (U+F004) and skull (U+F54C), subset at 36px/4bpp from LVGL's bundled `FontAwesome5-Solid+Brands+Regular.woff` using lv_font_conv 1.5.3. Its generated source is `src/assets/heart_symbol.c`; upstream font/icon/code licensing is included in `src/assets/LICENSE-FontAwesome.txt`. This single antialiased glyph replaces the earlier composite heart.
