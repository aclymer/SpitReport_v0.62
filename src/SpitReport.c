#include "pebble.h"
	
static Window *window;

static TextLayer *wind_layer;
static TextLayer *mateocons_layer;
static TextLayer *spot_name_layer;
static TextLayer *surf_ht_layer;
static TextLayer *water_temp_layer;
static BitmapLayer *icon_id_layer;
static GBitmap *icon_id_bitmap = NULL;
static BitmapLayer *button_bar_layer;
static GBitmap *bmp_button_bar;
const char *index = "refresh";
static AppSync sync;
static uint8_t sync_buffer[1024];

enum surfKey {
	INDEX_KEY = 0x0,			// TUPLE_CSTRING
	SURF_ICON_ID_KEY = 0x1,		// TUPLE_INTEGER
	SURF_WIND_KEY = 0x2,		// TUPLE_CSTRING
	SURF_SPOT_NAME_KEY = 0x3,	// TUPLE_CSTRING
	SPOT_ID_KEY = 0x4,			// TUPLE_INTEGER
	SURF_HT_KEY = 0x5,			// TUPLE_CSTRING
	WATER_TEMP_KEY = 0x6		// TUPLE_CSTRING
};

static const uint32_t SURF_ICONS[] = {
	RESOURCE_ID_IMAGE_POOR, //0
	RESOURCE_ID_IMAGE_POOR_FAIR, //1
	RESOURCE_ID_IMAGE_FAIR, //2
	RESOURCE_ID_IMAGE_FAIR_GOOD, //3
	RESOURCE_ID_IMAGE_GOOD, //4
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	switch (key) {
		case SURF_ICON_ID_KEY:
		if (icon_id_bitmap) {
			gbitmap_destroy(icon_id_bitmap);
		}
		icon_id_bitmap = gbitmap_create_with_resource(SURF_ICONS[new_tuple->value->uint8]);
		bitmap_layer_set_bitmap(icon_id_layer, icon_id_bitmap);
		break;
		
		case SURF_WIND_KEY:
		// App Sync keeps new_tuple in sync_buffer, so we may use it directly
		text_layer_set_text(wind_layer, new_tuple->value->cstring);
		break;
		
		case SURF_SPOT_NAME_KEY:
		text_layer_set_text(spot_name_layer, new_tuple->value->cstring);
		break;
		
		case SURF_HT_KEY:
		text_layer_set_text(surf_ht_layer, new_tuple->value->cstring);
		break;
		
		case WATER_TEMP_KEY:
		text_layer_set_text(water_temp_layer, new_tuple->value->cstring);
		break;
	}
}

static void send_cmd(void) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	Tuplet direction = TupletCString(0, index);	
	dict_write_tuplet(iter, &direction);
	Tuplet load = TupletCString(2, "Loading...");
	dict_write_tuplet(iter, &load);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	index = "above";
	send_cmd();
	index = "refresh";
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	index = "below";
	send_cmd();
	index = "refresh";
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	index = "refresh";
	send_cmd();
}

void select_multi_click_handler(ClickRecognizerRef recognizer, void *context) {
	index = "return";
	send_cmd();
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	
	// Add Mateocons Layer
	layer_add_child(window_layer, text_layer_get_layer(mateocons_layer));

	
	// Spot Name Layer
	spot_name_layer = text_layer_create(GRect(0, -6, 120, 52));
	text_layer_set_text_color(spot_name_layer, GColorWhite);
	text_layer_set_background_color(spot_name_layer, GColorClear);
	text_layer_set_font(spot_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(spot_name_layer, GTextAlignmentCenter);
	text_layer_set_overflow_mode(spot_name_layer, GTextOverflowModeWordWrap);
	layer_add_child(window_layer, text_layer_get_layer(spot_name_layer));

	//Surf Image Layer	
	icon_id_layer = bitmap_layer_create(GRect(22, 46, 80, 80));
	layer_add_child(window_layer, bitmap_layer_get_layer(icon_id_layer));
	
	// Water Temp Layer
	water_temp_layer = text_layer_create(GRect(24, 92, 40, 18));
	text_layer_set_text_color(water_temp_layer, GColorBlack);
	text_layer_set_background_color(water_temp_layer, GColorClear);
	text_layer_set_font(water_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(water_temp_layer, GTextAlignmentLeft);
	layer_add_child(window_layer, text_layer_get_layer(water_temp_layer));
	
	// Surf Height Layer
	surf_ht_layer = text_layer_create(GRect(72, 76, 30, 24));
	text_layer_set_text_color(surf_ht_layer, GColorWhite);
	text_layer_set_background_color(surf_ht_layer, GColorClear);
	text_layer_set_font(surf_ht_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_text_alignment(surf_ht_layer, GTextAlignmentRight);
	layer_add_child(window_layer, text_layer_get_layer(surf_ht_layer));
			
	// Button Bar Layer
	bmp_button_bar = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BUTTON_BAR);
	button_bar_layer = bitmap_layer_create(GRect(120, 0, 24, 152));
	bitmap_layer_set_bitmap(button_bar_layer, bmp_button_bar);
	layer_add_child(window_layer, bitmap_layer_get_layer(button_bar_layer));

	// Wind Layer
	wind_layer = text_layer_create(GRect(46, 122, 74, 32));
	text_layer_set_text_color(wind_layer, GColorWhite);
	text_layer_set_background_color(wind_layer, GColorClear);
	text_layer_set_font(wind_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_text_alignment(wind_layer, GTextAlignmentLeft);
	layer_add_child(window_layer, text_layer_get_layer(wind_layer));
	
	Tuplet initial_values[] = {
		TupletCString(INDEX_KEY, "refresh"),
		TupletInteger(SURF_ICON_ID_KEY, (uint8_t) 4),
		TupletCString(SURF_WIND_KEY, "Loading..."),
		TupletCString(SURF_SPOT_NAME_KEY, "SpitCast.com"),
		TupletCString(SURF_HT_KEY, "- ft"),
		TupletCString(WATER_TEMP_KEY, "--\u00B0F")
	};
	
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
				  sync_tuple_changed_callback, sync_error_callback, NULL);
	
	send_cmd();
}

void click_config_provider(Window *window) {
	
	//	Single Click Up Action
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	
	//	Single Click Down
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	
	//	Single Click Select
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	
	//	Long Click Select
	window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 10, 0 , true, select_multi_click_handler);
}

static void window_unload(Window *window) {
	app_sync_deinit(&sync);
	
	if (icon_id_bitmap) {
		gbitmap_destroy(icon_id_bitmap);
	}
	text_layer_destroy(mateocons_layer);
	text_layer_destroy(spot_name_layer);
	text_layer_destroy(surf_ht_layer);
	text_layer_destroy(water_temp_layer);
	text_layer_destroy(wind_layer);
	bitmap_layer_destroy(icon_id_layer);
	gbitmap_destroy(bmp_button_bar);
	bitmap_layer_destroy(button_bar_layer);
}

static void init(void) {
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	
	//Mateocons Layer Init
	GFont FONT_MATEOCONS_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MATEOCONS_28));
	mateocons_layer = text_layer_create(GRect(10, 122, 28, 32));
	text_layer_set_text_color(mateocons_layer, GColorWhite);
	text_layer_set_background_color(mateocons_layer, GColorClear);
	text_layer_set_font(mateocons_layer, FONT_MATEOCONS_28);
	text_layer_set_text_alignment(mateocons_layer, GTextAlignmentCenter);
	text_layer_set_text(mateocons_layer,"F");
	
	//Click Config Provider Init
	window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
	
	const int inbound_size = 1024;
	const int outbound_size = 256;
	app_message_open(inbound_size, outbound_size);
	
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
