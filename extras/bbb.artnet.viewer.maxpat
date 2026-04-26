{
	"patcher" : {
		"fileversion" : 1,
		"appversion" : {
			"major" : 8,
			"minor" : 6,
			"revision" : 4,
			"processor" : "x86",
			"platform" : "macintel"
		},
		"classnamespace" : "box",
		"rect" : [100.0, 100.0, 960.0, 720.0],
		"bglocked" : 0,
		"openrect" : [0.0, 0.0, 0.0, 0.0],
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 2,
		"gridsize" : [15.0, 15.0],
		"gridsnaponopen" : 0,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 2,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "Art-Net DMX viewer and control UI",
		"digest" : "Viewer and controller for Art-Net DMX",
		"tags" : "artnet, dmx, viewer, controller",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [
			{
				"box" : {
					"id" : "obj-1",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [20.0, 20.0, 400.0, 28.0],
					"fontsize" : 18.0,
					"text" : "bbb.artnet Viewer & Controller"
				}
			},
			{
				"box" : {
					"id" : "obj-2",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [20.0, 50.0, 300.0, 20.0],
					"fontsize" : 11.0,
					"text" : "Art-Net DMX send/receive utility"
				}
			},
			{
				"box" : {
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [20.0, 85.0, 200.0, 20.0],
					"fontsize" : 13.0,
					"text" : "━━ Controller (Send) ━━"
				}
			},
			{
				"box" : {
					"id" : "obj-4",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [20.0, 115.0, 150.0, 22.0],
					"text" : "target_ip 2.0.0.1"
				}
			},
			{
				"box" : {
					"id" : "obj-5",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [180.0, 115.0, 220.0, 22.0],
					"text" : "target_ip 255.255.255.255"
				}
			},
			{
				"box" : {
					"id" : "obj-6",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["bang"],
					"patching_rect" : [20.0, 145.0, 300.0, 22.0],
					"text" : "bbb.artnet.controller @target_ip 2.0.0.1 @mode 0"
				}
			},
			{
				"box" : {
					"id" : "obj-7",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [20.0, 175.0, 60.0, 18.0],
					"text" : "Universe:"
				}
			},
			{
				"box" : {
					"id" : "obj-8",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [85.0, 175.0, 50.0, 20.0],
					"minimum" : 0,
					"maximum" : 15
				}
			},
			{
				"box" : {
					"id" : "obj-9",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [85.0, 200.0, 120.0, 22.0],
					"text" : "prepend universe"
				}
			},
			{
				"box" : {
					"id" : "obj-10",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [220.0, 175.0, 55.0, 18.0],
					"text" : "Subnet:"
				}
			},
			{
				"box" : {
					"id" : "obj-11",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [280.0, 175.0, 50.0, 20.0],
					"minimum" : 0,
					"maximum" : 15
				}
			},
			{
				"box" : {
					"id" : "obj-12",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [280.0, 200.0, 110.0, 22.0],
					"text" : "prepend subnet"
				}
			},
			{
				"box" : {
					"id" : "obj-13",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [20.0, 230.0, 160.0, 18.0],
					"text" : "Channels 1-8 (0-255):"
				}
			},
			{
				"box" : {
					"id" : "obj-14",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [20.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-15",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [70.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-16",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [120.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-17",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [170.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-18",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [220.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-19",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [270.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-20",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [320.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-21",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [370.0, 255.0, 45.0, 20.0],
					"minimum" : 0,
					"maximum" : 255
				}
			},
			{
				"box" : {
					"id" : "obj-22",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [30.0, 280.0, 20.0, 18.0],
					"text" : "1"
				}
			},
			{
				"box" : {
					"id" : "obj-23",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [80.0, 280.0, 20.0, 18.0],
					"text" : "2"
				}
			},
			{
				"box" : {
					"id" : "obj-24",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [130.0, 280.0, 20.0, 18.0],
					"text" : "3"
				}
			},
			{
				"box" : {
					"id" : "obj-25",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [180.0, 280.0, 20.0, 18.0],
					"text" : "4"
				}
			},
			{
				"box" : {
					"id" : "obj-26",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [230.0, 280.0, 20.0, 18.0],
					"text" : "5"
				}
			},
			{
				"box" : {
					"id" : "obj-27",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [280.0, 280.0, 20.0, 18.0],
					"text" : "6"
				}
			},
			{
				"box" : {
					"id" : "obj-28",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [330.0, 280.0, 20.0, 18.0],
					"text" : "7"
				}
			},
			{
				"box" : {
					"id" : "obj-29",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [380.0, 280.0, 20.0, 18.0],
					"text" : "8"
				}
			},
			{
				"box" : {
					"id" : "obj-30",
					"maxclass" : "newobj",
					"numinlets" : 8,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [20.0, 310.0, 150.0, 22.0],
					"text" : "pak 0 0 0 0 0 0 0 0"
				}
			},
			{
				"box" : {
					"id" : "obj-31",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [500.0, 85.0, 200.0, 20.0],
					"fontsize" : 13.0,
					"text" : "━━ Viewer (Receive) ━━"
				}
			},
			{
				"box" : {
					"id" : "obj-32",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [500.0, 145.0, 200.0, 22.0],
					"text" : "bbb.artnet.node @mode 1"
				}
			},
			{
				"box" : {
					"id" : "obj-33",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [500.0, 175.0, 60.0, 18.0],
					"text" : "Universe:"
				}
			},
			{
				"box" : {
					"id" : "obj-34",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [565.0, 175.0, 50.0, 20.0],
					"minimum" : 0,
					"maximum" : 15
				}
			},
			{
				"box" : {
					"id" : "obj-35",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [565.0, 200.0, 120.0, 22.0],
					"text" : "prepend universe"
				}
			},
			{
				"box" : {
					"id" : "obj-36",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [700.0, 175.0, 55.0, 18.0],
					"text" : "Subnet:"
				}
			},
			{
				"box" : {
					"id" : "obj-37",
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["int"],
					"patching_rect" : [760.0, 175.0, 50.0, 20.0],
					"minimum" : 0,
					"maximum" : 15
				}
			},
			{
				"box" : {
					"id" : "obj-38",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [""],
					"patching_rect" : [760.0, 200.0, 110.0, 22.0],
					"text" : "prepend subnet"
				}
			},
			{
				"box" : {
					"id" : "obj-39",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [500.0, 230.0, 160.0, 18.0],
					"text" : "Received DMX data:"
				}
			},
			{
				"box" : {
					"id" : "obj-40",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 8,
					"outlettype" : ["int", "int", "int", "int", "int", "int", "int", "int"],
					"patching_rect" : [500.0, 310.0, 200.0, 22.0],
					"text" : "unpack 0 0 0 0 0 0 0 0"
				}
			},
			{
				"box" : {
					"id" : "obj-41",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [500.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-42",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [550.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-43",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [600.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-44",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [650.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-45",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [700.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-46",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [750.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-47",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [800.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-48",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : ["float"],
					"patching_rect" : [850.0, 340.0, 45.0, 20.0]
				}
			},
			{
				"box" : {
					"id" : "obj-49",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [510.0, 365.0, 20.0, 18.0],
					"text" : "1"
				}
			},
			{
				"box" : {
					"id" : "obj-50",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [560.0, 365.0, 20.0, 18.0],
					"text" : "2"
				}
			},
			{
				"box" : {
					"id" : "obj-51",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [610.0, 365.0, 20.0, 18.0],
					"text" : "3"
				}
			},
			{
				"box" : {
					"id" : "obj-52",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [660.0, 365.0, 20.0, 18.0],
					"text" : "4"
				}
			},
			{
				"box" : {
					"id" : "obj-53",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [710.0, 365.0, 20.0, 18.0],
					"text" : "5"
				}
			},
			{
				"box" : {
					"id" : "obj-54",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [760.0, 365.0, 20.0, 18.0],
					"text" : "6"
				}
			},
			{
				"box" : {
					"id" : "obj-55",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [810.0, 365.0, 20.0, 18.0],
					"text" : "7"
				}
			},
			{
				"box" : {
					"id" : "obj-56",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [860.0, 365.0, 20.0, 18.0],
					"text" : "8"
				}
			},
			{
				"box" : {
					"id" : "obj-57",
					"maxclass" : "multislider",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : ["", ""],
					"patching_rect" : [500.0, 400.0, 440.0, 200.0],
					"setsize" : 512,
					"setminmax" : [0.0, 255.0],
					"slidercolor" : [0.2, 0.6, 1.0, 1.0]
				}
			}
		],
		"lines" : [
			{
				"patchline" : {
					"source" : ["obj-4", 0],
					"destination" : ["obj-6", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-5", 0],
					"destination" : ["obj-6", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-8", 0],
					"destination" : ["obj-9", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-9", 0],
					"destination" : ["obj-6", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-11", 0],
					"destination" : ["obj-12", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-12", 0],
					"destination" : ["obj-6", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-14", 0],
					"destination" : ["obj-30", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-15", 0],
					"destination" : ["obj-30", 1]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-16", 0],
					"destination" : ["obj-30", 2]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-17", 0],
					"destination" : ["obj-30", 3]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-18", 0],
					"destination" : ["obj-30", 4]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-19", 0],
					"destination" : ["obj-30", 5]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-20", 0],
					"destination" : ["obj-30", 6]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-21", 0],
					"destination" : ["obj-30", 7]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-30", 0],
					"destination" : ["obj-6", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-34", 0],
					"destination" : ["obj-35", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-35", 0],
					"destination" : ["obj-32", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-37", 0],
					"destination" : ["obj-38", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-38", 0],
					"destination" : ["obj-32", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-32", 0],
					"destination" : ["obj-40", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-32", 0],
					"destination" : ["obj-57", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 0],
					"destination" : ["obj-41", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 1],
					"destination" : ["obj-42", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 2],
					"destination" : ["obj-43", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 3],
					"destination" : ["obj-44", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 4],
					"destination" : ["obj-45", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 5],
					"destination" : ["obj-46", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 6],
					"destination" : ["obj-47", 0]
				}
			},
			{
				"patchline" : {
					"source" : ["obj-40", 7],
					"destination" : ["obj-48", 0]
				}
			}
		]
	}
}
