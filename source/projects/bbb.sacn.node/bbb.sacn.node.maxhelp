{
	"patcher": {
		"fileversion": 1,
		"appversion": {
			"major": 7,
			"minor": 0,
			"revision": 2,
			"architecture": "x86",
			"modernui": 1
		},
		"rect": [
			100.0,
			100.0,
			860.0,
			532.0
		],
		"bglocked": 0,
		"openinpresentation": 0,
		"default_fontsize": 12.0,
		"default_fontface": 0,
		"default_fontname": "Arial",
		"gridonopen": 1,
		"gridsize": [
			10.0,
			10.0
		],
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
		"boxes": [
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 18.0,
					"id": "obj-1",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						20.0,
						600.0,
						28.0
					],
					"style": "",
					"text": "bbb.sacn.node",
					"fontface": 1
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-2",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						55.0,
						800.0,
						34.0
					],
					"style": "",
					"text": "bbb.sacn.node receives DMX values using the sACN (E1.31) protocol.\nListens for sACN data and outputs received channel values as lists.",
					"linecount": 2
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-3",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						20.0,
						104.0,
						100.0,
						20.0
					],
					"style": "",
					"text": "Messages:",
					"fontface": 1
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-4",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						104.0,
						100.0,
						20.0
					],
					"style": "",
					"text": "Attributes:",
					"fontface": 1
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-5",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						132.0,
						80.0,
						22.0
					],
					"style": "",
					"text": "bang"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-6",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						110.0,
						132.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Request current DMX values"
				}
			},
			{
				"box": {
					"attr": "universe",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-7",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						132.0,
						180.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-8",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						155.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "sACN universe (1-63999)"
				}
			},
			{
				"box": {
					"attr": "num_universes",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-9",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						180.0,
						160.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-10",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						203.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Number of universes to receive"
				}
			},
			{
				"box": {
					"attr": "num_channels",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-11",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						228.0,
						160.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-12",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						251.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Number of DMX channels"
				}
			},
			{
				"box": {
					"attr": "sync_universes",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-13",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						276.0,
						160.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-14",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						299.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Sync all universes"
				}
			},
			{
				"box": {
					"attr": "mode",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-15",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						324.0,
						220.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-16",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						347.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "update/bang/automatic/change/forced"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-17",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20.0,
						412.0,
						250.0,
						22.0
					],
					"style": "",
					"text": "bbb.sacn.node @universe 1",
					"outlettype": [
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-18",
					"maxclass": "button",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"patching_rect": [
						20.0,
						452.0,
						20.0,
						20.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-19",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						50.0,
						452.0,
						180.0,
						22.0
					],
					"style": "",
					"text": "print bbb.sacn.node"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-20",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						50.0,
						477.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "list: DMX values"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-5",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-7",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-9",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-11",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-13",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-17",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-15",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-18",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-17",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-19",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-17",
						0
					]
				}
			}
		],
		"embedsnapshot": 0
	}
}
