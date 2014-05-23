/*
 * ALPS touchpad PS/2 mouse driver
 *
 * Copyright (c) 2003 Peter Osterlund <petero2@telia.com>
 * Copyright (c) 2005 Vojtech Pavlik <vojtech@suse.cz>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _ALPS_H
#define _ALPS_H

#define ALPS_PROTO_V1	1
#define ALPS_PROTO_V2	2
#define ALPS_PROTO_V3	3
#define ALPS_PROTO_V4	4
#define ALPS_PROTO_V5	5
#define ALPS_PROTO_V6	6
#define ALPS_PROTO_V7	7

#define MAX_IMG_PT_NUM		5
#define V7_IMG_PT_NUM		2

#define ZONE_NORMAL				0x01
#define ZONE_RESTING			0x02
#define ZONE_LEFT_BTN			0x04
#define ZONE_RIGHT_BTN			0x08

#define DOLPHIN_COUNT_PER_ELECTRODE	64
#define DOLPHIN_PROFILE_XOFFSET		8	/* x-electrode offset */
#define DOLPHIN_PROFILE_YOFFSET		1	/* y-electrode offset */

/*
 * enum V7_PACKET_ID - defines the packet type for V7
 * V7_PACKET_ID_IDLE: There's no finger and no button activity.
 * V7_PACKET_ID_TWO: There's one or two non-resting fingers on touchpad
 *  or there's button activities.
 * V7_PACKET_ID_MULTI: There are at least three non-resting fingers.
 * V7_PACKET_ID_NEW: The finger position in slot is not continues from
 *  previous packet.
*/
enum V7_PACKET_ID {
	 V7_PACKET_ID_IDLE,
	 V7_PACKET_ID_TWO,
	 V7_PACKET_ID_MULTI,
	 V7_PACKET_ID_NEW,
	 V7_PACKET_ID_TRACKSTICK,
};

/**
 * struct alps_model_info - touchpad ID table
 * @signature: E7 response string to match.
 * @command_mode_resp: For V3/V4 touchpads, the final byte of the EC response
 *   (aka command mode response) identifies the firmware minor version.  This
 *   can be used to distinguish different hardware models which are not
 *   uniquely identifiable through their E7 responses.
 * @proto_version: Indicates V1/V2/V3/...
 * @byte0: Helps figure out whether a position report packet matches the
 *   known format for this model.  The first byte of the report, ANDed with
 *   mask0, should match byte0.
 * @mask0: The mask used to check the first byte of the report.
 * @flags: Additional device capabilities (passthrough port, trackstick, etc.).
 *
 * Many (but not all) ALPS touchpads can be identified by looking at the
 * values returned in the "E7 report" and/or the "EC report."  This table
 * lists a number of such touchpads.
 */
struct alps_model_info {
	unsigned char signature[3];
	unsigned char command_mode_resp;
	unsigned char proto_version;
	unsigned char byte0, mask0;
	unsigned char flags;
};

/**
 * struct alps_nibble_commands - encodings for register accesses
 * @command: PS/2 command used for the nibble
 * @data: Data supplied as an argument to the PS/2 command, if applicable
 *
 * The ALPS protocol uses magic sequences to transmit binary data to the
 * touchpad, as it is generally not OK to send arbitrary bytes out the
 * PS/2 port.  Each of the sequences in this table sends one nibble of the
 * register address or (write) data.  Different versions of the ALPS protocol
 * use slightly different encodings.
 */
struct alps_nibble_commands {
	int command;
	unsigned char data;
};

/**
 * struct alps_btn - decoded version of the button status
 * @left: Left touchpad button is active.
 * @right: Right touchpad button is active.
 * @middle: Middle touchpad button is active.
 * @ts_left: Left trackstick button is active.
 * @ts_right: Right trackstick button is active.
 * @ts_middle: Middle trackstick button is active.
 */
struct alps_btn {
	unsigned int left:1;
	unsigned int right:1;
	unsigned int middle:1;

	unsigned int ts_left:1;
	unsigned int ts_right:1;
	unsigned int ts_middle:1;
};

/**
 * struct alps_btn - decoded version of the X Y Z postion for ST.
 * @x: X position for ST.
 * @y: Y position for ST.
 * @z: Z position for ST.
 */
struct alps_abs_data {
	unsigned int x;
	unsigned int y;
	unsigned int z;
};

/**
 * struct alps_fields - decoded version of the report packet
 * @fingers: Number of fingers for MT.
 * @pt: X Y Z postion for ST.
 * @pt: X Y Z postion for image MT.
 * @x_map: Bitmap of active X positions for MT.
 * @y_map: Bitmap of active Y positions for MT.
 * @first_mp: Packet is the first of a multi-packet report.
 * @is_mp: Packet is part of a multi-packet report.
 * @btn: Button activity status
 */
struct alps_fields {
	unsigned int fingers;
	struct alps_abs_data pt;
	struct alps_abs_data pt_img[MAX_IMG_PT_NUM];
	unsigned int x_map;
	unsigned int y_map;
	unsigned int first_mp:1;
	unsigned int is_mp:1;
	struct alps_btn btn;
};

/**
 * struct v7_raw - data decoded from raw packet for V7.
 * @pkt_id: An id that specifies the type of packet.
 * @additional_fingers: Number of additional finger that is neighter included
 *  in pt slot nor reflected in rest_left and rest_right flag of data packet.
 * @rest_left: There are fingers on left resting zone.
 * @rest_right: There are fingers on right resting zone.
 */
struct v7_raw {
	unsigned char pkt_id;
	unsigned int additional_fingers;
	unsigned char rest_left;
	unsigned char rest_right;
};

/**
 * struct alps_bl_pt_attr - generic attributes of touch points for buttonless device
 * @zone: The part of touchpad that the touch point locates
 * @is_counted: The touch point is not a resting finger.
 * @is_init_pt_got: The touch down point is got.
 * @init_pt: The X Y Z position of the touch down point.
 */
struct alps_bl_pt_attr {
	unsigned char zone;
	unsigned char is_counted;
	unsigned char is_init_pt_got;
	struct alps_abs_data init_pt;
};

/**
 * struct alps_data - private data structure for the ALPS driver
 * @dev2: "Relative" device used to report trackstick or mouse activity.
 * @phys: Physical path for the relative device.
 * @nibble_commands: Command mapping used for touchpad register accesses.
 * @addr_command: Command used to tell the touchpad that a register address
 *   follows.
 * @proto_version: Indicates V1/V2/V3/...
 * @byte0: Helps figure out whether a position report packet matches the
 *   known format for this model.  The first byte of the report, ANDed with
 *   mask0, should match byte0.
 * @mask0: The mask used to check the first byte of the report.
 * @flags: Additional device capabilities (passthrough port, trackstick, etc.).
 * @x_max: Largest possible X position value.
 * @y_max: Largest possible Y position value.
 * @resting_zone_y_min: Smallest Y postion value of the bottom resting zone.
 * @x_bits: Number of X bits in the MT bitmap.
 * @y_bits: Number of Y bits in the MT bitmap.
 * @img_fingers: Number of image fingers.
 * @hw_init: Protocol-specific hardware init function.
 * @process_packet: Protocol-specific function to process a report packet.
 * @decode_fields: Protocol-specific function to read packet bitfields.
 * @set_abs_params: Protocol-specific function to configure the input_dev.
 * @prev_fin: Finger bit from previous packet.
 * @multi_packet: Multi-packet data in progress.
 * @multi_data: Saved multi-packet data.
 * @x1: First X coordinate from last MT report.
 * @x2: Second X coordinate from last MT report.
 * @y1: First Y coordinate from last MT report.
 * @y2: Second Y coordinate from last MT report.
 * @fingers: Number of fingers from last MT report.
 * @quirks: Bitmap of ALPS_QUIRK_*.
 * @timer: Timer for flushing out the final report packet in the stream.
 * @v7: Data decoded from raw packet for V7
 * @phy_btn: Physical button is active.
 * @prev_phy_btn: Physical button of previous packet is active.
 * @pressed_btn_bits: Pressed positon of button zone
 * @pt_attr: Generic attributes of touch points for buttonless device.
 */
struct alps_data {
	struct input_dev *dev2;
	char phys[32];

	/* these are autodetected when the device is identified */
	const struct alps_nibble_commands *nibble_commands;
	int addr_command;
	unsigned char proto_version;
	unsigned char byte0, mask0;
	unsigned char flags;
	int x_max;
	int y_max;
	int resting_zone_y_min;
	int x_bits;
	int y_bits;
	unsigned char slot_number;

	int (*hw_init)(struct psmouse *psmouse);
	void (*process_packet)(struct psmouse *psmouse);
	void (*decode_fields)(struct alps_fields *f, unsigned char *p,
			      struct psmouse *psmouse);
	void (*set_abs_params)(struct alps_data *priv, struct input_dev *dev1);

	int prev_fin;
	int multi_packet;
	unsigned char multi_data[6];
	int x1, x2, y1, y2;
	int fingers;
	u8 quirks;
	struct timer_list timer;

	/* these are used for buttonless touchpad*/
	union {
		struct v7_raw v7;
	} r;
	unsigned char phy_btn;
	unsigned char prev_phy_btn;
	unsigned char pressed_btn_bits;
	struct alps_bl_pt_attr pt_attr[MAX_IMG_PT_NUM];
};

#define ALPS_QUIRK_TRACKSTICK_BUTTONS	1 /* trakcstick buttons in trackstick packet */

#ifdef CONFIG_MOUSE_PS2_ALPS
int alps_detect(struct psmouse *psmouse, bool set_properties);
int alps_init(struct psmouse *psmouse);
#else
inline int alps_detect(struct psmouse *psmouse, bool set_properties)
{
	return -ENOSYS;
}
inline int alps_init(struct psmouse *psmouse)
{
	return -ENOSYS;
}
#endif /* CONFIG_MOUSE_PS2_ALPS */

#endif
