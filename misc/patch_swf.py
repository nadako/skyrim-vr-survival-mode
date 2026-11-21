# NOTE: Everything below this line is pure AI slop. It does the job tough. I asked it to comment it so I can figure it out and clean up later.

# Copies symbols (characters) from one SWF into another, working on the XML that
# `ffdec -swf2xml` produces. The result is turned back into a SWF with `ffdec -xml2swf`.
#
# Usage: patch_swf.py <src.xml> <dst.xml> <out.xml> <arg> ...
#   <src_id>:<dst_id>    replace character dst_id in dst with character src_id from src.
#                        The replacement keeps id dst_id, so everything in dst that places
#                        dst_id (same position, name, depth...) now shows the src contents.
#   +<src_export_name>   add the character that src exports under this name as a new
#                        character. Used for AS2 classes, which live in empty sprites
#                        exported as "__Packages.<class name>".
#   *<instance_name>=<factor>
#                        multiply the scale of the dst placement with this instance name.
#
# Background: every SWF object (shape, sprite, bitmap, font, text field, button) is a
# "character" with a 16-bit id that is only unique within its own file. Characters refer
# to each other by id (a sprite places shapes, a shape fills with a bitmap, a text field
# uses a font). So to move a character into another file we have to bring along everything
# it references, give all of it fresh ids that are free in the destination, and rewrite
# every reference to the new ids.

import sys
from xml.etree import ElementTree as ET

# Attribute that holds a defining tag's own character id. ffdec's XML names it differently
# per tag type: DefineBits*/DefineEditText use "characterID", DefineShape* use "shapeId",
# DefineSprite "spriteId", DefineButton* "buttonId", DefineFont* "fontID"/"fontId".
# Order matters: DefineEditText has both "characterID" (its own id) and "fontId" (a
# reference to a font), and the first match must be its own id.
DEFINE_ATTRS = ("characterID", "shapeId", "spriteId", "buttonId", "fontID", "fontId")

# Tags that don't define a character but attach extra data to one, mapped to the attribute
# holding the id they belong to. They must be copied together with that character (with the
# id rewritten), and removed together with a replaced destination character:
#   CSMSettingsTag            - text antialiasing settings for a text field
#   DoInitActionTag           - AS2 init code for a sprite (#initclip, i.e. class definitions)
#   DefineFontAlignZonesTag   - font hinting data
#   DefineFontNameTag         - font name/copyright
#   DefineFontInfo/Info2Tag   - font metadata
#   DefineScalingGridTag      - 9-slice scaling for a sprite/button
#   DefineButtonCxform/Sound  - extra button data
# Some of these start with "Define" but must not be treated as defining a character.
ASSOC_ATTRS = {
    "CSMSettingsTag": "textID",
    "DoInitActionTag": "spriteId",
    "DefineFontAlignZonesTag": "fontID",
    "DefineFontNameTag": "fontId",
    "DefineFontInfoTag": "fontId",
    "DefineFontInfo2Tag": "fontId",
    "DefineScalingGridTag": "characterId",
    "DefineButtonCxformTag": "buttonId",
    "DefineButtonSoundTag": "buttonId",
}

# Tags that bring in characters from another SWF by export name (Skyrim's HUD imports its
# fonts from gfxfontlib.swf). They also give a local character id, but there's no definition
# to copy: an import can only be matched by name or re-created.
IMPORT_TYPES = ("ImportAssetsTag", "ImportAssets2Tag")


# Name of the attribute holding the character id this tag defines, or None if the tag
# doesn't define a character.
def define_attr(tag):
    ty = tag.get("type")
    if not ty.startswith("Define") or ty in ASSOC_ATTRS:
        return None
    for a in DEFINE_ATTRS:
        if a in tag.attrib:
            return a
    return None


# Every (element, attribute) pair inside a defining tag that references another character:
#   characterId - PlaceObject in sprites, BUTTONRECORD in buttons
#   bitmapId    - bitmap fill styles in shapes; 65535 means "no bitmap", so it's not a reference
#   fontId      - the font of a DefineEditText (only when hasFont is set; otherwise the
#                 attribute is a leftover, not a real reference)
# The tag's own id attribute is skipped (only descendants are checked for characterId/bitmapId).
def ref_attrs(tag):
    out = []
    for e in tag.iter():
        if e is not tag:
            for a in ("characterId", "bitmapId"):
                if a in e.attrib and e.get(a) != "65535":
                    out.append((e, a))
    if tag.get("type") == "DefineEditTextTag" and tag.get("hasFont") == "true":
        out.append((tag, "fontId"))
    return out


# Look-up tables for one SWF's top-level tags:
#   defs    - character id -> defining tag
#   imports - character id -> (import tag, imported name)
#   exports - export (linkage) name -> character id
def index_swf(tags):
    defs = {}
    imports = {}
    exports = {}
    for t in tags:
        a = define_attr(t)
        if a:
            defs[int(t.get(a))] = t
        elif t.get("type") in IMPORT_TYPES:
            for i, n in zip(t.find("tags"), t.find("names")):
                imports[int(i.text)] = (t, n.text)
        elif t.get("type") == "ExportAssetsTag":
            for i, n in zip(t.find("tags"), t.find("names")):
                exports[n.text] = int(i.text)
    return defs, imports, exports


def main():
    if len(sys.argv) < 5:
        sys.exit("usage: patch_swf.py <src.xml> <dst.xml> <out.xml> (<src_id>:<dst_id> | +<src_export_name> | *<instance_name>=<factor>) ...")
    src_xml, dst_xml, out_xml = sys.argv[1], sys.argv[2], sys.argv[3]

    src_tree = ET.parse(src_xml)
    dst_tree = ET.parse(dst_xml)
    src_tags = src_tree.getroot().find("tags")
    dst_tags = dst_tree.getroot().find("tags")
    src_defs, src_imports, src_exports = index_swf(src_tags)
    dst_defs, dst_imports, dst_exports = index_swf(dst_tags)

    # roots: src id -> dst id it replaces. added: src id -> export name, copied as new characters.
    roots = {}
    added = {}
    scales = {}
    for arg in sys.argv[4:]:
        if arg.startswith("*"):
            name, factor = arg[1:].split("=")
            scales[name] = float(factor)
        elif arg.startswith("+"):
            if arg[1:] not in src_exports:
                sys.exit("source export '%s' not found" % arg[1:])
            added[src_exports[arg[1:]]] = arg[1:]
        else:
            s, d = arg.split(":")
            roots[int(s)] = int(d)
    if len(set(roots.values())) != len(roots):
        sys.exit("destination ids must be unique")

    for src_id, dst_id in roots.items():
        if src_id not in src_defs:
            sys.exit("source character %d not found" % src_id)
        if dst_id not in dst_defs:
            sys.exit("destination character %d not found" % dst_id)

    # A character can be exported under several names.
    src_export_names = {}
    for n, cid in src_exports.items():
        src_export_names.setdefault(cid, []).append(n)

    # Step 1: collect every src character the requested ones depend on, directly or
    # indirectly (the closure).
    # Exception: a dependency exported under a name that dst also exports is not copied.
    # The dst one is reused instead (`shared`: src id -> dst id), and we don't descend into it.
    # Export names must be unique within a SWF, and code that does attachMovie("name") expects
    # that one symbol. In practice these are the same asset in both files (e.g. the shout meter
    # "Shout_22").
    closure = set()
    shared = {}
    stack = list(roots) + list(added)
    while stack:
        cid = stack.pop()
        if cid in closure or cid in shared:
            continue
        if cid not in roots and cid not in added:
            names = [n for n in src_export_names.get(cid, []) if n in dst_exports]
            if names:
                shared[cid] = dst_exports[names[0]]
                continue
        closure.add(cid)
        if cid in src_defs:
            for e, a in ref_attrs(src_defs[cid]):
                stack.append(int(e.get(a)))
        elif cid not in src_imports:
            sys.exit("source character %d referenced but not defined" % cid)

    # Step 2: decide the dst id of every copied character (mapping: src id -> dst id).
    #   - roots keep the id of the dst character they replace
    #   - shared ones use the existing dst character
    #   - imports (fonts from gfxfontlib.swf) reuse dst's import of the same name if there is
    #     one; otherwise a new import tag is created for them
    #   - everything else gets a fresh id above the highest one dst uses
    next_id = max(list(dst_defs) + list(dst_imports)) + 1
    mapping = dict(roots)
    mapping.update(shared)
    new_imports = []
    dst_import_by_name = {name: cid for cid, (_, name) in dst_imports.items()}
    for cid in sorted(closure):
        if cid in roots:
            continue
        if cid in src_imports:
            itag, name = src_imports[cid]
            if name in dst_import_by_name:
                mapping[cid] = dst_import_by_name[name]
                continue
            nt = ET.Element("item", dict(itag.attrib))
            ET.SubElement(ET.SubElement(nt, "tags"), "item").text = str(next_id)
            ET.SubElement(ET.SubElement(nt, "names"), "item").text = name
            new_imports.append(nt)
        mapping[cid] = next_id
        next_id += 1
    if next_id > 65535:
        sys.exit("out of character ids")

    # Step 3: copy the src tags that belong to the closure, in src order, with ids rewritten.
    # src order matters: a SWF must define a character before anything uses it, and src
    # already satisfies that among these tags.
    # Covered: defining tags, tags attached to a copied character (ASSOC_ATTRS), and export
    # entries of copied characters (exports of roots are dropped: a root takes over the dst
    # character's identity, and the dst character must not be exported, see step 4).
    # Tags of `+` additions go to `tail`, everything else to `copied`; they're inserted at
    # different places in step 4.
    copied = []
    tail = []
    for t in src_tags:
        ty = t.get("type")
        a = define_attr(t)
        if a:
            owner = int(t.get(a))
            if owner not in closure:
                continue
            nt = ET.fromstring(ET.tostring(t))
            nt.set(a, str(mapping[owner]))
        elif ty in ASSOC_ATTRS:
            ref = t.get(ASSOC_ATTRS[ty])
            if ref is None or int(ref) not in closure or int(ref) in src_imports:
                continue
            owner = int(ref)
            nt = ET.fromstring(ET.tostring(t))
            nt.set(ASSOC_ATTRS[ty], str(mapping[owner]))
        elif ty == "ExportAssetsTag":
            pairs = [(int(i.text), n.text) for i, n in zip(t.find("tags"), t.find("names")) if int(i.text) in closure and int(i.text) not in roots]
            if not pairs:
                continue
            for _, name in pairs:
                if name in dst_exports:
                    sys.exit("export name '%s' already exists in destination" % name)
            nt = ET.Element("item", dict(t.attrib))
            ti = ET.SubElement(nt, "tags")
            ni = ET.SubElement(nt, "names")
            for cid, name in pairs:
                ET.SubElement(ti, "item").text = str(mapping[cid])
                ET.SubElement(ni, "item").text = name
            (tail if all(cid in added for cid, _ in pairs) else copied).append(nt)
            continue
        else:
            continue
        for e, ra in ref_attrs(nt):
            e.set(ra, str(mapping[int(e.get(ra))]))
        (tail if owner in added else copied).append(nt)

    # Step 4: edit dst.
    # Remove the replaced characters along with their attached tags. A replaced character
    # that dst exports by name is an error: code could be using it by that name, and this
    # script doesn't handle that.
    dst_root_ids = {str(d) for d in roots.values()}
    for t in list(dst_tags):
        ty = t.get("type")
        if ty in ASSOC_ATTRS and t.get(ASSOC_ATTRS[ty]) in dst_root_ids:
            dst_tags.remove(t)
        elif ty == "ExportAssetsTag":
            for i, n in zip(t.find("tags"), t.find("names")):
                if i.text in dst_root_ids:
                    sys.exit("destination character %s is exported as '%s'" % (i.text, n.text))
    # The copied tags (new imports first, they may be referenced by the rest) go where the
    # earliest replaced character was defined. That's before every dst tag that uses any of
    # the replaced characters, since dst defines a character before its first use.
    # The old characters' own dependencies stay in dst unused; that's harmless.
    old_roots = [dst_defs[d] for d in roots.values()]
    if old_roots:
        pos = min(list(dst_tags).index(t) for t in old_roots)
    else:
        pos = len(dst_tags)
    for t in old_roots:
        dst_tags.remove(t)
    for k, t in enumerate(new_imports + copied):
        dst_tags.insert(pos + k, t)
    # `+` additions (AS2 class packages) go at the end of frame 1, just before its ShowFrame.
    # A subclass's init code runs `extends` against its base class, so it has to come after
    # the base class's DoInitAction, which dst has among its existing __Packages tags. Putting
    # it last on frame 1 satisfies that and still runs before the frame is shown, i.e. before
    # any instance that needs the class is constructed.
    tags_list = list(dst_tags)
    pos = len(tags_list)
    for k, t in enumerate(tags_list):
        if t.get("type") == "ShowFrameTag":
            pos = k
            break
    for k, t in enumerate(tail):
        dst_tags.insert(pos + k, t)

    for name, factor in scales.items():
        places = [t for t in dst_tags.iter("item") if t.get("type", "").startswith("PlaceObject") and t.get("name") == name]
        if len(places) != 1:
            sys.exit("expected one placement named '%s', found %d" % (name, len(places)))
        m = places[0].find("matrix")
        if m is None:
            sys.exit("placement '%s' has no matrix" % name)
        sx = float(m.get("scaleX", "1")) * factor
        sy = float(m.get("scaleY", "1")) * factor
        bits = max(round(abs(sx) * 65536).bit_length(), round(abs(sy) * 65536).bit_length()) + 1
        m.set("hasScale", "true")
        m.set("nScaleBits", str(bits))
        m.set("scaleX", repr(sx))
        m.set("scaleY", repr(sy))

    dst_tree.write(out_xml, encoding="utf-8", xml_declaration=True)
    pairs_str = ", ".join(["%d -> %d" % p for p in roots.items()] + ["+" + n for n in added.values()] + ["*%s=%g" % s for s in scales.items()])
    print("patched %s: %d characters, %d shared by export name, %d new imports, %d tags" % (pairs_str, len(closure), len(shared), len(new_imports), len(new_imports) + len(copied) + len(tail)))


main()
