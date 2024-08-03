enum bone : int {
	Head = 110,
	Neck = 67,
	Chest = 66,
	Pelvis = 2,
	LShoulder = 9,
	LElbow = 10,
	LHand = 11,
	RShoulder = 38,
	RElbow = 39,
	RHand = 40,
	LHip = 71,
	LKnee = 72,
	LFoot = 73,
	RHip = 78,
	RKnee = 79,
	RFoot = 82,
	Root = 0
};

__int64 dynamic_uworld;

enum offset {
	uworld = 0x118011A8, // 
	game_instance = 0x1D8,//
	game_state = 0x160, //
	local_player = 0x38, //
	player_controller = 0x30,//
	acknowledged_pawn = 0x338,//
	aactor = 0xA0,//updated
	skeletal_mesh = 0x318, //
	player_state = 0x2b0, //
	root_component = 0x198, //
	velocity = 0x168, //
	relative_location = 0x120,//
	relative_rotation = 0x138,
	current_weapon = 0xA68,// updated
	weapon_data = 0x510, // updated
	tier = 0x9b, // updated
	team_index = 0x1211, //
	player_array = 0x2A8, //
	pawn_private = 0x308, //
	component_to_world = 0x1c0, //
};

enum triggerbotoffsets {
	location_under_reticle = 0x2560,
};




