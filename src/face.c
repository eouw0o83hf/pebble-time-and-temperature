#include "face.h"
#include <pebble.h>

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_font_code_bold_60;
static GFont s_res_droid_serif_28_bold;
static GFont s_res_gothic_28_bold;
static TextLayer *s_layer_time;
static TextLayer *s_layer_date;
static TextLayer *s_layer_weather;
static Layer *s_battery_canvas_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_fullscreen(s_window, true);
  
  s_res_font_code_bold_60 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CODE_BOLD_60));
  s_res_droid_serif_28_bold = fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  // s_layer_time
  s_layer_time = text_layer_create(GRect(0, 24, 144, 60));
  text_layer_set_background_color(s_layer_time, GColorClear);
  text_layer_set_text_color(s_layer_time, GColorWhite);
  text_layer_set_text(s_layer_time, "--:--");
  text_layer_set_text_alignment(s_layer_time, GTextAlignmentCenter);
  text_layer_set_font(s_layer_time, s_res_font_code_bold_60);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_layer_time);
  
  // s_layer_date
  s_layer_date = text_layer_create(GRect(0, 94, 144, 30));
  text_layer_set_background_color(s_layer_date, GColorClear);
  text_layer_set_text_color(s_layer_date, GColorWhite);
  text_layer_set_text(s_layer_date, "[date]");
  text_layer_set_text_alignment(s_layer_date, GTextAlignmentCenter);
  text_layer_set_font(s_layer_date, s_res_droid_serif_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_layer_date);
  
  // s_layer_weather
  s_layer_weather = text_layer_create(GRect(0, 128, 144, 28));
  text_layer_set_background_color(s_layer_weather, GColorClear);
  text_layer_set_text_color(s_layer_weather, GColorWhite);
  text_layer_set_text(s_layer_weather, "[weather]");
  text_layer_set_text_alignment(s_layer_weather, GTextAlignmentCenter);
  text_layer_set_font(s_layer_weather, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_layer_weather);
  
  // s_battery_canvas_layer
  s_battery_canvas_layer = layer_create(GRect(0, 0, 144, 20));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_battery_canvas_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_layer_time);
  text_layer_destroy(s_layer_date);
  text_layer_destroy(s_layer_weather);
  layer_destroy(s_battery_canvas_layer);
  fonts_unload_custom_font(s_res_font_code_bold_60);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

// BEGIN ACTUAL CUSTOM CODE

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 	1
#define KEY_LATITUDE	2
#define KEY_LONGITUDE	3
	
#define PERSIST_WEATHER	1
#define PERSIST_LATLON	2

static int s_battery_charge_percent;
static bool s_battery_is_charging;

static void battery_ui_update(Layer *layer, GContext *context) {
	int charge_width = 144 * s_battery_charge_percent / 100;
	APP_LOG(APP_LOG_LEVEL_INFO, "Setting charge width to %d pixels", charge_width);
	
	graphics_context_set_fill_color(context, GColorWhite);
	
	GRect rect;
	if(s_battery_is_charging) {
		rect = GRect(charge_width, 0, 144 - charge_width, 20);
	} else {
		rect = GRect(0, 0, charge_width, 20);
	}
	graphics_fill_rect(context, rect, 5, GCornerNone);
}

static void battery_handler(BatteryChargeState new_state) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Battery charge %d, Is charging %d!", new_state.charge_percent, new_state.is_charging ? 1 : 0);
	s_battery_charge_percent = new_state.charge_percent;
	s_battery_is_charging = new_state.is_charging;
	
	// Force an update
	layer_mark_dirty(s_battery_canvas_layer);
}

static void update_time() {
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	// make all char buffers long-lived
	static char time_buffer[] = "00:00";
	static char date_buffer[] = "Jan 01";
		
	// move to buffer
	if(clock_is_24h_style() == true) {
		strftime(time_buffer, sizeof(time_buffer), "%H:%M", tick_time);
	} else {
		strftime(time_buffer, sizeof(time_buffer), "%I:%M", tick_time);
	}
	
	strftime(date_buffer, sizeof(date_buffer), "%b %d", tick_time);
	
	// persist to UI
	text_layer_set_text(s_layer_time, time_buffer);
	text_layer_set_text(s_layer_date, date_buffer);
	
	// Refresh weather every five minutes
	if(tick_time->tm_min % 5 == 0) {	
		APP_LOG(APP_LOG_LEVEL_INFO, "Requesting updated weather");

		DictionaryIterator *iter;
  		app_message_outbox_begin(&iter);
		
		// This is where you write to the request dictionary.
		// Here's an example I found of doing that:
		//  Tuplet tuple = (symbol == NULL)
        //              ? TupletInteger(quote_key, 1)
        //              : TupletCString(quote_key, symbol);
		//	dict_write_tuplet(iter, &tuple);
		
  		dict_write_end(iter);
  		app_message_outbox_send();
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	
	static char temp_buffer[8];
	static char cond_buffer[32];
	static char weather_buffer[32];
	
	static char lat_buffer[8];
	static char lon_buffer[8];
	static char latlon_buffer[32];
	
	APP_LOG(APP_LOG_LEVEL_INFO, "response received from phone");
	
	// This is how to pull a key out on demand
	/*
		Tuple *thing = dict_find(iterator, KEY_TEMPERATURE);
		if(thing == NULL) {
			APP_LOG(APP_LOG_LEVEL_ERROR, "error finding temperature item");

		} else {
			APP_LOG(APP_LOG_LEVEL_WARNING, "found it!");
		}
	*/
	
	// This is how to iteratively find all key/value pairs
	Tuple *t = dict_read_first(iterator);
	while(t != NULL) {
		switch(t->key) {
			case KEY_TEMPERATURE:
				snprintf(temp_buffer, sizeof(temp_buffer), "%d°F", (int)t->value->int32);
				break;
			
			case KEY_CONDITIONS:
				snprintf(cond_buffer, sizeof(cond_buffer), "%s", t->value->cstring);
				break;
			
			case KEY_LATITUDE:
				snprintf(lat_buffer, sizeof(lat_buffer), "%.6s", t->value->cstring);
				break;
			
			case KEY_LONGITUDE:
				snprintf(lon_buffer, sizeof(lon_buffer), t->value->cstring[0] == '-' ? "%.7s" : "%.6s", t->value->cstring);
				break;
			
			default:
				APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
				break;
		}
		
		t = dict_read_next(iterator);
	}
	
	// Assemble
	snprintf(weather_buffer, sizeof(weather_buffer), "%s, %s", temp_buffer, cond_buffer);
	text_layer_set_text(s_layer_weather, weather_buffer);
	
	//snprintf(latlon_buffer, sizeof(latlon_buffer), "%s, %s", lat_buffer, lon_buffer);
	//text_layer_set_text(s_layer_latlon, latlon_buffer);
	
	persist_write_string(PERSIST_WEATHER, weather_buffer);
	persist_write_string(PERSIST_LATLON, latlon_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed because %d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send succeeded");
}

void CustomInit(void) {		
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	update_time();
	
	layer_set_update_proc(s_battery_canvas_layer, battery_ui_update);
	battery_state_service_subscribe(battery_handler);
	battery_handler(battery_state_service_peek());
	
	// SDK best practice: register callbacks before opening appMessage
	app_message_register_inbox_received(inbox_received_callback);	
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	
	// Now open appMessage
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	if(persist_exists(PERSIST_WEATHER)) {
		static char weather_buffer[32];
		persist_read_string(PERSIST_WEATHER, weather_buffer, sizeof(weather_buffer));
		text_layer_set_text(s_layer_weather, weather_buffer);
	}
	if(persist_exists(PERSIST_LATLON)) {
		static char latlon_buffer[32];
		persist_read_string(PERSIST_LATLON, latlon_buffer, sizeof(latlon_buffer));
		//text_layer_set_text(s_latlon_layer, latlon_buffer);
	}
}


void show_face(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
}

void hide_face(void) {
  window_stack_remove(s_window, true);
}
