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
			964.0
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
					"text": "bbb.sacn.controller",
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
					"text": "bbb.sacn.controller sends DMX values using the sACN (E1.31) protocol.\nSupports multiple universes, priority control, and configurable send modes.",
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
						172.0,
						22.0
					],
					"style": "",
					"text": "255 128 0 255 128 0"
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
						202.0,
						132.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Send a list of DMX values (ints 0-255)"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-7",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						160.0,
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
					"id": "obj-8",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						110.0,
						160.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Trigger send (in bang/forced mode)"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-9",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						188.0,
						124.0,
						22.0
					],
					"style": "",
					"text": "channel 1 255"
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
						154.0,
						188.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Set single channel: channel <ch> <val>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-11",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						216.0,
						140.0,
						22.0
					],
					"style": "",
					"text": "setchannel 2 $1"
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
						170.0,
						216.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Set channel without sending"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-13",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						244.0,
						124.0,
						22.0
					],
					"style": "",
					"text": "set 255 128 0"
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
						154.0,
						244.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Store values without sending"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-15",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						272.0,
						156.0,
						22.0
					],
					"style": "",
					"text": "set_offset 10 255"
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
						186.0,
						272.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Set value at offset position"
				}
			},
			{
				"box": {
					"attr": "universe",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-17",
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
					"id": "obj-18",
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
					"id": "obj-19",
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
					"id": "obj-20",
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
					"text": "Number of universes to span"
				}
			},
			{
				"box": {
					"attr": "num_channels",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-21",
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
					"id": "obj-22",
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
					"id": "obj-23",
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
					"id": "obj-24",
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
					"text": "Sync all universes on change"
				}
			},
			{
				"box": {
					"attr": "blackout",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-25",
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
						130.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-26",
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
					"text": "Send all zeros"
				}
			},
			{
				"box": {
					"attr": "mode",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-27",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						372.0,
						270.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-28",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						395.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "0=automatic 1=bang 2=update 3=change 4=forced"
				}
			},
			{
				"box": {
					"attr": "framerate",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-29",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						420.0,
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
					"id": "obj-30",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						443.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Target framerate"
				}
			},
			{
				"box": {
					"attr": "priority",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-31",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						468.0,
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
					"id": "obj-32",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						491.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "sACN priority (0-200)"
				}
			},
			{
				"box": {
					"attr": "source_name",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-33",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						516.0,
						200.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-34",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						539.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "sACN source name"
				}
			},
			{
				"box": {
					"attr": "target_ip",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-35",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						564.0,
						200.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-36",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						587.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Destination IP (empty=multicast)"
				}
			},
			{
				"box": {
					"attr": "bind_ip",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-37",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						612.0,
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
					"id": "obj-38",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						635.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Local interface IP"
				}
			},
			{
				"box": {
					"attr": "origin",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-39",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						660.0,
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
					"id": "obj-40",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						683.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Channel index origin"
				}
			},
			{
				"box": {
					"attr": "unicast",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-41",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						708.0,
						150.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-42",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						731.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Legacy unicast mode"
				}
			},
			{
				"box": {
					"attr": "unicast_ip",
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-43",
					"lock": 1,
					"maxclass": "attrui",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						520.0,
						756.0,
						210.0,
						22.0
					],
					"style": ""
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-44",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						520.0,
						779.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Legacy unicast destination"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-45",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 1,
					"patching_rect": [
						20.0,
						844.0,
						432.5,
						22.0
					],
					"style": "",
					"text": "bbb.sacn.controller @universe 1 @target_ip 192.168.1.50",
					"outlettype": [
						"bang"
					]
				}
			},
			{
				"box": {
					"id": "obj-46",
					"maxclass": "button",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"patching_rect": [
						20.0,
						884.0,
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
					"id": "obj-47",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						50.0,
						884.0,
						180.0,
						22.0
					],
					"style": "",
					"text": "print bbb.sacn.controller"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-48",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						50.0,
						909.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "bang on send"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"destination": [
						"obj-45",
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
						"obj-45",
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
						"obj-45",
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
						"obj-45",
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
						"obj-45",
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
						"obj-45",
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
						"obj-45",
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
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-19",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-21",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-23",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-25",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-27",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-29",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-31",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-33",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-35",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-37",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-39",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-41",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-45",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-43",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-46",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-45",
						0
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-47",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-45",
						0
					]
				}
			}
		],
		"embedsnapshot": 0
	}
}
