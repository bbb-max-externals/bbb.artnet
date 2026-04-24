#!/usr/bin/env python3
import json
import os

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECTS_DIR = os.path.join(BASE_DIR, "source", "projects")


class IDGen:
    def __init__(self):
        self.n = 1

    def __call__(self):
        oid = f"obj-{self.n}"
        self.n += 1
        return oid


def box_comment(
    id, text, x, y, w=400.0, h=20.0, bold=False, fontsize=12.0, linecount=None
):
    b = {
        "fontname": "Arial",
        "fontsize": fontsize,
        "id": id,
        "maxclass": "comment",
        "numinlets": 1,
        "numoutlets": 0,
        "patching_rect": [float(x), float(y), float(w), float(h)],
        "style": "",
        "text": text,
    }
    if bold:
        b["fontface"] = 1
    if linecount is not None:
        b["linecount"] = linecount
    return {"box": b}


def box_message(id, text, x, y, w=120.0):
    return {
        "box": {
            "fontname": "Arial",
            "fontsize": 12.0,
            "id": id,
            "maxclass": "message",
            "numinlets": 2,
            "numoutlets": 1,
            "outlettype": [""],
            "patching_rect": [float(x), float(y), float(w), 22.0],
            "style": "",
            "text": text,
        }
    }


def box_attrui(id, attr, x, y, w=200.0):
    return {
        "box": {
            "attr": attr,
            "fontname": "Arial",
            "fontsize": 12.0,
            "id": id,
            "lock": 1,
            "maxclass": "attrui",
            "numinlets": 1,
            "numoutlets": 1,
            "outlettype": [""],
            "patching_rect": [float(x), float(y), float(w), 22.0],
            "style": "",
        }
    }


def box_newobj(id, text, x, y, w=200.0, numinlets=1, numoutlets=1, outlettype=None):
    b = {
        "fontname": "Arial",
        "fontsize": 12.0,
        "id": id,
        "maxclass": "newobj",
        "numinlets": numinlets,
        "numoutlets": numoutlets,
        "patching_rect": [float(x), float(y), float(w), 22.0],
        "style": "",
        "text": text,
    }
    if numoutlets > 0 and outlettype is not None:
        b["outlettype"] = outlettype
    return {"box": b}


def box_button(id, x, y):
    return {
        "box": {
            "id": id,
            "maxclass": "button",
            "numinlets": 1,
            "numoutlets": 1,
            "outlettype": ["bang"],
            "patching_rect": [float(x), float(y), 20.0, 20.0],
            "style": "",
        }
    }


def patchline(src, src_out, dst, dst_in, midpoints=None):
    pl = {
        "patchline": {
            "destination": [dst, dst_in],
            "disabled": 0,
            "hidden": 0,
            "source": [src, src_out],
        }
    }
    if midpoints:
        pl["patchline"]["midpoints"] = [float(v) for v in midpoints]
    return pl


def patcher_meta(w=860.0, h=700.0):
    return {
        "fileversion": 1,
        "appversion": {
            "major": 7,
            "minor": 0,
            "revision": 2,
            "architecture": "x86",
            "modernui": 1,
        },
        "rect": [100.0, 100.0, float(w), float(h)],
        "bglocked": 0,
        "openinpresentation": 0,
        "default_fontsize": 12.0,
        "default_fontface": 0,
        "default_fontname": "Arial",
        "gridonopen": 1,
        "gridsize": [10.0, 10.0],
        "gridsnaponopen": 1,
        "objectsnaponopen": 1,
        "statusbarvisible": 2,
        "toolbarvisible": 1,
        "lefttoolbarpinned": 0,
        "toptoolbarpinned": 0,
        "righttoolbarpinned": 0,
        "bottomtoolbarpinned": 0,
        "toolbars_unpinned_last_save": 0,
        "tallnewobj": 0,
        "boxanimatetime": 200,
        "enablehscroll": 1,
        "enablevscroll": 1,
        "devicewidth": 0.0,
        "description": "",
        "digest": "",
        "tags": "",
        "style": "",
        "subpatcher_template": "",
    }


def generate_help(name, cfg):
    gid = IDGen()
    boxes = []
    lines = []

    # Title
    boxes.append(box_comment(gid(), name, 20, 20, 600, 28, bold=True, fontsize=18.0))

    # Description
    desc = cfg["description"]
    lc = desc.count("\n") + 1
    boxes.append(box_comment(gid(), desc, 20, 55, 800, 17.0 * lc, linecount=lc))

    desc_bottom = 55 + 17.0 * lc + 15

    # Section headers
    boxes.append(box_comment(gid(), "Messages:", 20, desc_bottom, 100, 20, bold=True))
    boxes.append(
        box_comment(gid(), "Attributes:", 520, desc_bottom, 100, 20, bold=True)
    )

    section_y = desc_bottom + 28

    # --- Messages (left column) ---
    msg_y = section_y
    msg_ids = []
    for msg_text, msg_desc in cfg["messages"]:
        mid = gid()
        msg_w = max(80.0, len(msg_text) * 8.0 + 20.0)
        boxes.append(box_message(mid, msg_text, 20, msg_y, msg_w))
        boxes.append(box_comment(gid(), msg_desc, 20 + msg_w + 10, msg_y, 300, 20))
        msg_ids.append(mid)
        msg_y += 28

    # --- Attributes (right column) ---
    attr_y = section_y
    attr_ids = []
    for attr_name, attr_desc, attr_w in cfg["attributes"]:
        aid = gid()
        boxes.append(box_attrui(aid, attr_name, 520, attr_y, attr_w))
        if attr_desc:
            boxes.append(box_comment(gid(), attr_desc, 520, attr_y + 23, 300, 18))
            attr_y += 20
        attr_ids.append(aid)
        attr_y += 28

    # --- Main external ---
    ext_y = max(msg_y, attr_y) + 40
    ext_text = cfg["external_text"]
    ext_w = max(250.0, len(ext_text) * 7.5 + 20.0)
    ext_id = gid()
    boxes.append(
        box_newobj(
            ext_id,
            ext_text,
            20,
            ext_y,
            ext_w,
            numinlets=1,
            numoutlets=cfg["numoutlets"],
            outlettype=cfg["outlettype"],
        )
    )

    # Connect messages -> external
    for mid in msg_ids:
        lines.append(patchline(mid, 0, ext_id, 0))

    # Connect attrui -> external
    for aid in attr_ids:
        lines.append(patchline(aid, 0, ext_id, 0))

    # --- Output section ---
    out_y = ext_y + 40
    outlet_labels = cfg.get("outlet_labels", [])

    for i in range(cfg["numoutlets"]):
        x_off = i * 350
        otype = cfg["outlettype"][i] if i < len(cfg["outlettype"]) else ""

        # button for visual feedback
        btn_id = gid()
        boxes.append(box_button(btn_id, 20 + x_off, out_y))
        lines.append(patchline(ext_id, i, btn_id, 0))

        # print object
        pid = gid()
        boxes.append(
            box_newobj(
                pid, f"print {name}", 50 + x_off, out_y, 180, numinlets=1, numoutlets=0
            )
        )
        lines.append(patchline(ext_id, i, pid, 0))

        # label comment
        if i < len(outlet_labels):
            boxes.append(
                box_comment(gid(), outlet_labels[i], 50 + x_off, out_y + 25, 300, 20)
            )

    # Assemble
    meta = patcher_meta(860.0, out_y + 80)
    meta["boxes"] = boxes
    meta["lines"] = lines
    meta["embedsnapshot"] = 0
    return {"patcher": meta}


EXTERNALS = {
    "bbb.artnet.controller": {
        "description": "bbb.artnet.controller sends DMX values using the Art-Net protocol.\nSupports multiple universes, unicast/broadcast modes, and configurable send modes.",
        "messages": [
            ("255 128 0 255 128 0", "Send a list of DMX values (ints 0-255)"),
            ("bang", "Trigger send (in bang/forced mode)"),
            ("channel 1 255", "Set single channel: channel <ch> <val>"),
            ("setchannel 2 $1", "Set channel without sending"),
            ("set 255 128 0", "Store values without sending"),
            ("set_offset 10 255", "Set value at offset position"),
        ],
        "attributes": [
            ("net", "Art-Net net (0-127)", 160.0),
            ("subnet", "Art-Net subnet (0-15)", 160.0),
            ("universe", "Art-Net universe (0-15)", 160.0),
            ("num_universes", "Number of universes to span", 160.0),
            ("num_channels", "Number of DMX channels", 160.0),
            ("sync_universes", "Sync all universes on change", 160.0),
            ("blackout", "Send all zeros", 130.0),
            ("mode", "automatic/bang/update/change/forced", 220.0),
            ("framerate", "Target framerate (0.01-44)", 160.0),
            ("unicast", "Enable unicast mode (default: 1)", 160.0),
            ("unicast_ip", "Destination IP for unicast", 200.0),
            ("alt_broadcast_ip", "Use 10.x.x.x broadcast range", 200.0),
            ("osc_port", "OSC feedback port", 160.0),
        ],
        "external_text": "bbb.artnet.controller @unicast_ip 127.0.0.1 @universe 1",
        "numoutlets": 1,
        "outlettype": ["bang"],
        "outlet_labels": ["bang on send"],
    },
    "bbb.artnet.node": {
        "description": "bbb.artnet.node receives DMX values using the Art-Net protocol.\nListens for Art-Net data and outputs received channel values as lists.",
        "messages": [
            ("bang", "Request current DMX values"),
        ],
        "attributes": [
            ("net", "Art-Net net (0-127)", 160.0),
            ("subnet", "Art-Net subnet (0-15)", 160.0),
            ("universe", "Art-Net universe (0-15)", 160.0),
            ("num_universes", "Number of universes to receive", 160.0),
            ("num_channels", "Number of DMX channels", 160.0),
            ("sync_universes", "Sync all universes", 160.0),
            ("mode", "update/bang/automatic/change/forced", 220.0),
            ("framerate", "Output framerate (0.01-44)", 160.0),
            ("osc_port", "OSC port for listening", 160.0),
        ],
        "external_text": "bbb.artnet.node @universe 1",
        "numoutlets": 1,
        "outlettype": [""],
        "outlet_labels": ["list: DMX values"],
    },
    "bbb.artnet.rdm": {
        "description": "bbb.artnet.rdm is an RDM controller over Art-Net.\nDiscovers RDM devices, queries and sets device parameters, manages responders.",
        "messages": [
            ("discover", "Discover RDM devices on the network"),
            ("tod", "Request Table of Devices"),
            ("identify 1", "Identify device: identify <uid> <on/off>"),
            ("start_address 1", "Get DMX start address: start_address <uid>"),
            ("label My Device", "Set device label: label <uid> <text>"),
            ("device_info", "Request device info: device_info <uid>"),
            ("manufacturer_label", "Get manufacturer label: manufacturer_label <uid>"),
            ("software_version", "Get software version: software_version <uid>"),
            ("get 1 device_info", "Get RDM parameter: get <uid> <pid>"),
            ("set 1 start_address 1", "Set RDM parameter: set <uid> <pid> <val>"),
            ("mute", "Mute device: mute <uid>"),
            ("unmute", "Unmute device: unmute <uid>"),
        ],
        "attributes": [
            ("net", "Art-Net net (0-127)", 160.0),
            ("subnet", "Art-Net subnet (0-15)", 160.0),
            ("universe", "Art-Net universe (0-15)", 160.0),
            ("unicast", "Enable unicast mode", 160.0),
            ("unicast_ip", "Destination IP for unicast", 200.0),
            ("source_uid", "RDM controller source UID", 200.0),
            ("timeout", "RDM response timeout (ms)", 160.0),
        ],
        "external_text": "bbb.artnet.rdm @unicast_ip 127.0.0.1 @universe 1",
        "numoutlets": 2,
        "outlettype": ["", ""],
        "outlet_labels": [
            "data_out: response/uids/ack",
            "status_out: nack/timeout/error",
        ],
    },
    "bbb.sacn.controller": {
        "description": "bbb.sacn.controller sends DMX values using the sACN (E1.31) protocol.\nSupports multiple universes, priority control, and configurable send modes.",
        "messages": [
            ("255 128 0 255 128 0", "Send a list of DMX values (ints 0-255)"),
            ("bang", "Trigger send (in bang/forced mode)"),
            ("channel 1 255", "Set single channel: channel <ch> <val>"),
            ("setchannel 2 $1", "Set channel without sending"),
            ("set 255 128 0", "Store values without sending"),
            ("set_offset 10 255", "Set value at offset position"),
        ],
        "attributes": [
            ("universe", "sACN universe (1-63999)", 180.0),
            ("num_universes", "Number of universes to span", 160.0),
            ("num_channels", "Number of DMX channels", 160.0),
            ("sync_universes", "Sync all universes on change", 160.0),
            ("blackout", "Send all zeros", 130.0),
            ("mode", "automatic/bang/update/change/forced", 220.0),
            ("framerate", "Target framerate", 160.0),
            ("priority", "sACN priority (0-200)", 160.0),
            ("source_name", "sACN source name", 200.0),
            ("unicast", "Enable unicast mode", 160.0),
            ("unicast_ip", "Destination IP for unicast", 200.0),
        ],
        "external_text": "bbb.sacn.controller @universe 1",
        "numoutlets": 1,
        "outlettype": ["bang"],
        "outlet_labels": ["bang on send"],
    },
    "bbb.sacn.node": {
        "description": "bbb.sacn.node receives DMX values using the sACN (E1.31) protocol.\nListens for sACN data and outputs received channel values as lists.",
        "messages": [
            ("bang", "Request current DMX values"),
        ],
        "attributes": [
            ("universe", "sACN universe (1-63999)", 180.0),
            ("num_universes", "Number of universes to receive", 160.0),
            ("num_channels", "Number of DMX channels", 160.0),
            ("sync_universes", "Sync all universes", 160.0),
            ("mode", "update/bang/automatic/change/forced", 220.0),
        ],
        "external_text": "bbb.sacn.node @universe 1",
        "numoutlets": 1,
        "outlettype": [""],
        "outlet_labels": ["list: DMX values"],
    },
}


def main():
    for name, cfg in EXTERNALS.items():
        data = generate_help(name, cfg)
        out_dir = os.path.join(PROJECTS_DIR, name)
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, f"{name}.maxhelp")
        with open(out_path, "w") as f:
            json.dump(data, f, indent="\t")
            f.write("\n")
        print(f"  {out_path}")


if __name__ == "__main__":
    main()
