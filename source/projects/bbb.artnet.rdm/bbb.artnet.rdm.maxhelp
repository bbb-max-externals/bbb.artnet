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
			628.0
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
					"text": "bbb.artnet.rdm",
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
					"text": "bbb.artnet.rdm is an RDM controller over Art-Net.\nDiscovers RDM devices, queries and sets device parameters, manages responders.",
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
						84.0,
						22.0
					],
					"style": "",
					"text": "discover"
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
						114.0,
						132.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Discover RDM devices on the network"
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
					"text": "tod"
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
					"text": "Request Table of Devices"
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
						100.0,
						22.0
					],
					"style": "",
					"text": "identify 1"
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
						130.0,
						188.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Identify device: identify <uid> <on/off>"
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
					"text": "start_address 1"
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
					"text": "Get DMX start address: start_address <uid>"
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
						140.0,
						22.0
					],
					"style": "",
					"text": "label My Device"
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
						170.0,
						244.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Set device label: label <uid> <text>"
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
						108.0,
						22.0
					],
					"style": "",
					"text": "device_info"
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
						138.0,
						272.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Request device info: device_info <uid>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-17",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						300.0,
						164.0,
						22.0
					],
					"style": "",
					"text": "manufacturer_label"
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
						194.0,
						300.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Get manufacturer label: manufacturer_label <uid>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-19",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						328.0,
						148.0,
						22.0
					],
					"style": "",
					"text": "software_version"
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
						178.0,
						328.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Get software version: software_version <uid>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-21",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						356.0,
						156.0,
						22.0
					],
					"style": "",
					"text": "get 1 device_info"
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
						186.0,
						356.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Get RDM parameter: get <uid> <pid>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-23",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						384.0,
						188.0,
						22.0
					],
					"style": "",
					"text": "set 1 start_address 1"
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
						218.0,
						384.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Set RDM parameter: set <uid> <pid> <val>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-25",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						412.0,
						80.0,
						22.0
					],
					"style": "",
					"text": "mute"
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
						110.0,
						412.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Mute device: mute <uid>"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-27",
					"maxclass": "message",
					"numinlets": 2,
					"numoutlets": 1,
					"outlettype": [
						""
					],
					"patching_rect": [
						20.0,
						440.0,
						80.0,
						22.0
					],
					"style": "",
					"text": "unmute"
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
						110.0,
						440.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "Unmute device: unmute <uid>"
				}
			},
			{
				"box": {
					"attr": "net",
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
						132.0,
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
						155.0,
						300.0,
						18.0
					],
					"style": "",
					"text": "Art-Net net (0-127)"
				}
			},
			{
				"box": {
					"attr": "subnet",
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
					"id": "obj-32",
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
					"text": "Art-Net subnet (0-15)"
				}
			},
			{
				"box": {
					"attr": "universe",
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
					"id": "obj-34",
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
					"text": "Art-Net universe (0-15)"
				}
			},
			{
				"box": {
					"attr": "unicast",
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
					"id": "obj-36",
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
					"text": "Enable unicast mode"
				}
			},
			{
				"box": {
					"attr": "unicast_ip",
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
						324.0,
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
					"id": "obj-38",
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
					"text": "Destination IP for unicast"
				}
			},
			{
				"box": {
					"attr": "source_uid",
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
						372.0,
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
					"id": "obj-40",
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
					"text": "RDM controller source UID"
				}
			},
			{
				"box": {
					"attr": "timeout",
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
					"id": "obj-42",
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
					"text": "RDM response timeout (ms)"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-43",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 2,
					"patching_rect": [
						20.0,
						508.0,
						380.0,
						22.0
					],
					"style": "",
					"text": "bbb.artnet.rdm @unicast_ip 127.0.0.1 @universe 1",
					"outlettype": [
						"",
						""
					]
				}
			},
			{
				"box": {
					"id": "obj-44",
					"maxclass": "button",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"patching_rect": [
						20.0,
						548.0,
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
					"id": "obj-45",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						50.0,
						548.0,
						180.0,
						22.0
					],
					"style": "",
					"text": "print bbb.artnet.rdm"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-46",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						50.0,
						573.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "data_out: response/uids/ack"
				}
			},
			{
				"box": {
					"id": "obj-47",
					"maxclass": "button",
					"numinlets": 1,
					"numoutlets": 1,
					"outlettype": [
						"bang"
					],
					"patching_rect": [
						370.0,
						548.0,
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
					"id": "obj-48",
					"maxclass": "newobj",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						400.0,
						548.0,
						180.0,
						22.0
					],
					"style": "",
					"text": "print bbb.artnet.rdm"
				}
			},
			{
				"box": {
					"fontname": "Arial",
					"fontsize": 12.0,
					"id": "obj-49",
					"maxclass": "comment",
					"numinlets": 1,
					"numoutlets": 0,
					"patching_rect": [
						400.0,
						573.0,
						300.0,
						20.0
					],
					"style": "",
					"text": "status_out: nack/timeout/error"
				}
			}
		],
		"lines": [
			{
				"patchline": {
					"destination": [
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-43",
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
						"obj-44",
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
						"obj-47",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-43",
						1
					]
				}
			},
			{
				"patchline": {
					"destination": [
						"obj-48",
						0
					],
					"disabled": 0,
					"hidden": 0,
					"source": [
						"obj-43",
						1
					]
				}
			}
		],
		"embedsnapshot": 0
	}
}
