#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xcb.h>

#include <GL/glx.h>
#include <GL/gl.h>

#include "augs/window_framework/window.h"

namespace augs {
	window::window(const window_settings& settings) {
		int default_screen = 0xdeadbeef;

		/* Open Xlib Display */ 
		display = XOpenDisplay(0);

		default_screen = DefaultScreen(display);

		/* Get the XCB connection from the display */
		connection = XGetXCBConnection(display);

		if(!connection) {
			XCloseDisplay(display);
			throw window_error("Can't get xcb connection from display");
		}

		/* Acquire event queue ownership */
		XSetEventQueueOwner(display, XCBOwnsEventQueue);

		/* Find XCB screen */
		xcb_screen_t *screen = 0;
		xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
		for(int screen_num = default_screen;
						screen_iter.rem && screen_num > 0;
						--screen_num, xcb_screen_next(&screen_iter));
		screen = screen_iter.data;

		auto setup_and_run = [this, settings](int default_screen, xcb_screen_t *screen) {
			int visualID = 0;

			/* Query framebuffer configurations */
			GLXFBConfig *fb_configs = 0;
			int num_fb_configs = 0;
			fb_configs = glXGetFBConfigs(display, default_screen, &num_fb_configs);

			if (!fb_configs || num_fb_configs == 0) {
				throw window_error("glXGetFBConfigs failed");
			}

			/* Select first framebuffer config and query visualID */
			GLXFBConfig fb_config = fb_configs[0];
			glXGetFBConfigAttrib(display, fb_config, GLX_VISUAL_ID , &visualID);

			/* Create OpenGL context */
			context = glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, 0, True);

			if (!context) {
				throw window_error("glXCreateNewContext failed");
			}

			/* Create XID's for colormap and window */
			xcb_colormap_t colormap = xcb_generate_id(connection);
			window_id = xcb_generate_id(connection);

			/* Create colormap */
			xcb_create_colormap(
				connection,
				XCB_COLORMAP_ALLOC_NONE,
				colormap,
				screen->root,
				visualID
			);

			/* Create window */
			uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
			uint32_t valuelist[] = { eventmask, colormap, 0 };
			uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;

			xcb_create_window(
				connection,
				XCB_COPY_FROM_PARENT,
				window_id,
				screen->root,
				settings.position.x, settings.position.y,
				settings.size.x, settings.size.y,
				0,
				XCB_WINDOW_CLASS_INPUT_OUTPUT,
				visualID,
				valuemask,
				valuelist
			);


			// NOTE: window must be mapped before glXMakeContextCurrent
			xcb_map_window(connection, window_id); 

			glxwindow = glXCreateWindow(display, fb_config, window_id, 0);

			if(!window_id) {
				xcb_destroy_window(connection, window_id);
				glXDestroyContext(display, context);

				throw window_error("glXCreateWindow failed");
			}

			drawable = glxwindow;

			/* make OpenGL context current */
			if (!set_as_current()) {
				xcb_destroy_window(connection, window_id);
				glXDestroyContext(display, context);

				throw window_error("glXMakeContextCurrent failed");
			}
		};

		setup_and_run(default_screen, screen);
		apply(settings, true);
	}

	void window::destroy() {
		if (display) {
			unset_if_current();

			glXDestroyWindow(display, glxwindow);

			xcb_destroy_window(connection, window_id);

			glXDestroyContext(display, context);

			XCloseDisplay(display);

			context = 0;
			glxwindow = 0;
			drawable = 0;
			display = nullptr;
			connection = nullptr;
		}	
	}

	void window::set_window_name(const std::string& name) {
		xcb_change_property (connection,
				XCB_PROP_MODE_REPLACE,
				window_id,
				XCB_ATOM_WM_NAME,
				XCB_ATOM_STRING,
				8,
				name.length(),
				name.c_str()
		);
	}

	void window::set_window_border_enabled(const bool) {}

	bool window::swap_buffers() { 
		glXSwapBuffers(display, drawable);
		return true;
	}

	void window::show() {}
	void window::set_mouse_pos_frozen(const bool) {}

	bool window::is_mouse_pos_frozen() const {
		return false;
	}

	void window::collect_entropy(local_entropy& into) {
		/* Wait for event */
		//return;
	//	xcb_generic_event_t *event = xcb_poll_for_event(connection);
		
		// if (!event) {
			// throw window_error("i/o error in xcb_wait_for_event");
		// }
		
		xcb_generic_event_t* event = nullptr;
		
		while (event = xcb_poll_for_event(connection)) {
			event::change ch;

			switch (event->response_type & ~0x80) {
				case XCB_KEY_PRESS:
					break;
				default:
					break;
			}

			free(event);
		}
	}

	void window::set_window_rect(const xywhi) {}

	xywhi window::get_window_rect() const { 
		xcb_get_geometry_cookie_t  geomCookie = xcb_get_geometry (connection, window_id);
		
		std::unique_ptr<xcb_get_geometry_reply_t, decltype(free)*> geom { 
			xcb_get_geometry_reply (connection, geomCookie, NULL), free
		};

		return { geom->x, geom->y, geom->width, geom->height }; 
	}

	bool window::is_active() const { return false; }


	bool window::set_as_current_impl() {
#if BUILD_OPENGL
		return glXMakeContextCurrent(display, drawable, drawable, context);
#else
		return true;
#endif
	}

	void window::set_current_to_none_impl() {
#if BUILD_OPENGL
	// For now we only will have one window anyway
	//	 glXMakeContextCurrent(display, None, None, nullptr);
#endif
	}

	std::optional<std::string> window::open_file_dialog(
		const std::vector<file_dialog_filter>& filters,
		std::string custom_title
	) const {
		return std::nullopt;
	}

	std::optional<std::string> window::save_file_dialog(
		const std::vector<file_dialog_filter>& filters,
		std::string custom_title
	) const {
		return std::nullopt;
	}
}
