-- Clone this file ("config.lua") and name it "config.local.lua"" so that it stays unversioned and unique to your filesystem,
-- if for example you want to preserve your original window resolution and coordinates across further commits.
-- Hypersomnia will try to read "config.local.lua"" and if there is no such file, it shall try loading "config.lua".

-- various game launching types passed as values only to the variable "launch_mode"
launch_type = {
	-- Will enter the game's main menu.
	MAIN_MENU = 0,

	-- Will launch the game locally without networking at all.
	LOCAL = 1,
	
	-- Same as **LOCAL**, but will launch *determinism_test_cloned_cosmoi_count* copies of the world 
	-- running in parallel to whom applied are exactly the same inputs.
	-- If the worlds differ at some point, the game will hit an assertion.
	LOCAL_DETERMINISM_TEST = 2,
	
	-- Director mode. You can record, replay and rewind actions for multiple entities,
	-- so that you can create elaborate scenes for intros, trailers etc. 
	-- Remember that in-game replayed results may look different due to disabled interpolation
	-- in the director mode and different behaviour of audiovisual response systems.
	DIRECTOR = 3,

	-- Choreographic mode.
	-- Will load a scenario filename specified by choreographic_input_scenario_path.
	-- Very helpful in creating in-game commentaries and gameplay footages in general.
	--
	-- Choreographic mode is not the same thing as the director mode!
	-- This one has the ability to automatically replay and rewind multiple scenes produced by the director mode,
	-- with varying speeds, character focus and additionally with audio overlay (commentator's voice for example)
	CHOREOGRAPHIC = 4,

	-- Will use *connect_address* and *connect_port* to connect to a remote host and start the multiplayer simulation.
	ONLY_CLIENT = 5,

	-- Will use *server_port* to setup a listenserver without a game client.
	ONLY_SERVER = 6,

	-- **ONLY_SERVER** and **ONLY_CLIENT** in the same process
	CLIENT_AND_SERVER = 7,

	-- **ONLY_SERVER** and two clients on split-screen. 
	-- For debugging purposes. The server will use *alternative_port* for the second connection.
	TWO_CLIENTS_AND_SERVER = 8
}

-- machine input recording modes for deterministic bug reproduction
recording_type = {
	-- no recording at all (best performance)
	DISABLE = 0,
	-- record with buffer (moderate performance, but some important machine inputs may be lost if a segfault is encountered)
	-- NOT IMPLEMENTED!!!
	LIVE_WITH_BUFFER = 1,
	-- record live (saves machine inputs each frame to the file and does not proceed until it is done)
	LIVE = 2
}

config_table = {
	launch_mode = launch_type.LOCAL,
	
	input_recording_mode = recording_type.LIVE,

	-- initial replaying speed of the machine entropies recording
	recording_replay_speed = 1,

	-- see LOCAL_DETERMINISM_TEST
	determinism_test_cloned_cosmoi_count = 2,

	-- game window settings
	window_name = "example",
	fullscreen = false,
	window_border = 1,
	window_x = 100,
	window_y = 10,
	bpp = 24,
	resolution_w = 1280,
	resolution_h = 768,
	doublebuffer = true,

	check_content_integrity_every_launch = true,

	-- 1 - more hdd space required, but launching the game is way faster
	save_regenerated_atlases_as_binary = true,

	-- 1 always regenerates the entire content. only for debugging pruposes - the game will take a lot longer to launch every time.
	debug_regenerate_content_every_launch = false,

	-- detail value. so that the regenerator does not have to query actual value from opengl.
	packer_detail_max_atlas_size = 8192,

	-- if true, runs unit tests on every launch.
	debug_run_unit_tests = false,

	-- if true, logs all unit tests that succeed, not just failures.
	debug_log_successful_unit_tests = false,

	-- if true, breaks on the first failure.
	debug_break_on_unit_test_failure = true,

	-- should enable head-related transfer function for OpenAL?
	enable_hrtf = true,

	-- maximum number of audible sound effects
	max_number_of_sound_sources = 4096,

	-- value of "" means the default audio device will be used
	audio_output_device = "",
	-- OpenAL Soft on Line 1 (Virtual Audio Cable)
	
	-- volume settings
	sound_effects_volume = 0.1,
	music_volume = 1,

	-- Flag. 1 disables the cursor clipping so that it is easier to mark a breakpoint, for example. 0 is for normal playing.
	debug_disable_cursor_clipping = false,
	
	-- vec2. Sensitivity of mouse movement in-game.
	mouse_sensitivity = vec2.new(1.5, 1.5),
	
	-- Network variables. See launch_mode for details.
	connect_address = "192.168.1.8",
	connect_port = 13372,

	server_port = 13372,

	alternative_port = 13373,
	
	-- Client-chosen nickname of the controlled character.
	nickname = "Sentinel",

	-- Client-chosen nickname of the second controlled character used in launch_mode.TWO_CLIENTS_AND_SERVER.
	debug_second_nickname = "Billan",
	
	-- Frequency of the simulation. 1/tickrate equals the fixed delta time in seconds, so tickrate = 60 means that the logical step advances the simulation about around 16 milliseconds.
	default_tickrate = 60,

	-- Client-side jitter buffer time to preserve smooth display of the past. The bigger the value, the bigger the lag.
	jitter_buffer_ms = 50,

	-- Server-side jitter buffer time for client commands. Useful for lag simulation.
	client_commands_jitter_buffer_ms = 0,
	
	interpolation_speed = 525,
	misprediction_smoothing_multiplier = 1.2,

	-- Used by the server to inject random inputs to the other players to examine and test lag compensation strategies.
	debug_randomize_entropies_in_client_setup = true,

	-- How often the above input injection happens. The less it is, the more erratic the movements are.
	debug_randomize_entropies_in_client_setup_once_every_steps = 1,

	-- Flag. 1 will launch a http daemon on the localhost in a separate thread which samples the server statistics. Used as a widget on http://hypersomnia.pl
	server_launch_http_daemon = true,

	-- What port to open the web daemon on. Recommended value: 80.
	server_http_daemon_port = 80,

	-- Format of the broadcasted widget.
	server_http_daemon_html_file_path = "web/session_report.html",

	-- Path to server's private information
	db_path = "P:/Projects/db/",

	-- Survey number
	survey_num_file_path = "survey_num.in",
	
	-- Post data for some requests
	post_data_file_path = "post.txt",
	
	-- Will refresh last session time on the blog
	last_session_update_link = "patrykcysarz.pl/comment-system/web/stats/last-update/set",

	-- Intro scene path to be opened with DIRECTOR mode
	director_input_scene_entropy_path = "director/sequence_3.ent",

	-- Input scenario for choreographic launch mode
	choreographic_input_scenario_path = "choreographic/gameplay_1.chg", 
	
	-- Main menu intro scene path recorded with DIRECTOR mode
	menu_intro_scene_entropy_path = "director/menu_intro.ent",

	-- Menu theme path
	menu_theme_path = "hypersomnia/music/menu_theme.flac",

	-- Menu intro timing settings
	rewind_intro_scene_by_secs = 3.5,
	start_menu_music_at_secs = 63.5 - 22.5,

	skip_credits = false,
	
	-- Url for the rolling news bar in the main menu
	latest_news_url = "http://hypersomnia.pl/latest_post/",

	-- hotbar appearance settings
	hotbar_increase_inside_alpha_when_selected = false,
	hotbar_colorize_inside_when_selected = true,
	hotbar_primary_selected_color = rgba.new(0, 255, 255, 255),
	hotbar_secondary_selected_color = rgba.new(86, 156, 214, 255),

	-- debug drawing settings
	debug_drawing_enabled = true,
	debug_draw_colinearization = false,
	debug_draw_forces = true,
	debug_draw_friction_field_collisions_of_entering = false,
	debug_draw_explosion_forces = false,
	debug_draw_visibility = false
}
