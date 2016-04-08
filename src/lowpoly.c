#include <pebble.h>

static Window *main_window;
static TextLayer *time_layer, *date_layer;
static GBitmap *bg_bitmap, *bt_icon_bitmap, *battery_icon_bitmap;
static BitmapLayer *bg_bitmap_layer, *bt_icon_layer, *battery_icon_layer;
static GFont timeFont, dateFont;
static int battery_level;
static Layer *battery_layer;

typedef enum {
  AppKeyBackgroundColor = 0,
  AppKeyVibrate = 1,
  AppKeyLeadingZero = 2,
  AppKeyShowBattery = 3
} AppKey;

static void battery_callback(BatteryChargeState state) {
  battery_level = state.charge_percent;
  layer_mark_dirty(battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = (int)(float)(((float)battery_level / 100.0F) * 16.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  if(width<=4){
    graphics_context_set_fill_color(ctx, GColorRed);
  } else {
    graphics_context_set_fill_color(ctx, GColorWhite);
  }
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}


static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(bt_icon_layer), connected);
  if(!connected) {
    if(persist_read_bool(AppKeyVibrate)) vibes_double_pulse();
  }
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Time
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  if(persist_read_bool(AppKeyLeadingZero)) {
    text_layer_set_text(time_layer, s_buffer);
  } else {
    if(s_buffer[0]=='0'){
      char *temp = s_buffer;
      temp++;
      text_layer_set_text(time_layer, temp);
    } else {
      text_layer_set_text(time_layer, s_buffer);
    }
  }

  // Date
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%A %d", tick_time);
  text_layer_set_text(date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Background bitmap image
  Tuple *bg_color_t = dict_find(iter, AppKeyBackgroundColor);
  if(bg_color_t) {
    gbitmap_destroy(bg_bitmap);
    if(bg_color_t->value->int32==1){          // Blue
      persist_write_int(AppKeyBackgroundColor, 1);
      bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_BLUE_ROUND, RESOURCE_ID_BLUE));
      bitmap_layer_set_bitmap(bg_bitmap_layer, bg_bitmap);
    } else if(bg_color_t->value->int32==2){   // Red
      persist_write_int(AppKeyBackgroundColor, 2);
      bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_RED_ROUND, RESOURCE_ID_RED));
      bitmap_layer_set_bitmap(bg_bitmap_layer, bg_bitmap);
    } else if(bg_color_t->value->int32==3){   // Green
      persist_write_int(AppKeyBackgroundColor, 3);
      bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_GREEN_ROUND, RESOURCE_ID_GREEN));
      bitmap_layer_set_bitmap(bg_bitmap_layer, bg_bitmap);
    }
  }

  // Vibrate on disconnect
  Tuple *vibrate_t = dict_find(iter, AppKeyVibrate);
  if(vibrate_t) {
    if(vibrate_t->value->int32 == 1){       // Vibrate on disconnect
      persist_write_bool(AppKeyVibrate, true);
    } else {                                // Dont' vibrate on disconnect
      persist_write_bool(AppKeyVibrate, false);
    }
  }

  // Leading zero
  Tuple *leadingzero_t = dict_find(iter, AppKeyLeadingZero);
  if(leadingzero_t){
    if(leadingzero_t->value->int32 == 1){   // Show leading zero
      persist_write_bool(AppKeyLeadingZero, true);
      update_time();
    } else {                                // Hide leading zero
      persist_write_bool(AppKeyLeadingZero, false);
      update_time();
    }
  }

  // Battery meter
  Tuple *battery_t = dict_find(iter, AppKeyShowBattery);
  if(battery_t){
    if(battery_t->value->int32 == 1){   // Show battery meter
      persist_write_bool(AppKeyShowBattery, true);
      battery_state_service_subscribe(battery_callback);
      battery_callback(battery_state_service_peek());
      layer_set_hidden(bitmap_layer_get_layer(battery_icon_layer),false);
      layer_set_hidden(battery_layer,false);
    } else {                            // Hide battery meter
      persist_write_bool(AppKeyShowBattery, false);
      layer_set_hidden(bitmap_layer_get_layer(battery_icon_layer),true);
      layer_set_hidden(battery_layer,true);
    }
  }
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  window_set_background_color(window, GColorBlack);

  // Load custom fonts
  timeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BLOCKY_58));
  dateFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BLOCKY_20));

  // Add the background image
  if(persist_read_int(AppKeyBackgroundColor)==1){
    bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_BLUE_ROUND, RESOURCE_ID_BLUE));
  } else if(persist_read_int(AppKeyBackgroundColor)==2){
    bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_RED_ROUND, RESOURCE_ID_RED));
  } else if(persist_read_int(AppKeyBackgroundColor)==3){
    bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_GREEN_ROUND, RESOURCE_ID_GREEN));
  } else {
    bg_bitmap = gbitmap_create_with_resource(PBL_IF_ROUND_ELSE(RESOURCE_ID_BLUE_ROUND, RESOURCE_ID_BLUE));
  }
  bg_bitmap_layer = bitmap_layer_create(GRect(0, 0, PBL_IF_ROUND_ELSE(180, 144), PBL_IF_ROUND_ELSE(180, 168)));
  bitmap_layer_set_compositing_mode(bg_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(bg_bitmap_layer, bg_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(bg_bitmap_layer));

  // Create time TextLayer
  time_layer = PBL_IF_ROUND_ELSE(text_layer_create(GRect(3, 40, 180, 60)), text_layer_create(GRect(3, 30, 144, 60)));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_font(time_layer, timeFont);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  // Create date TextLayer
  date_layer = text_layer_create(PBL_IF_ROUND_ELSE(GRect(1, 105, 180, 30),GRect(1, 95, 144, 30)));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_font(date_layer, dateFont);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

  // Bluetooth icon
  bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH);
  bt_icon_layer = bitmap_layer_create(PBL_IF_ROUND_ELSE(GRect(75, 10, 30, 30),GRect(2, 2, 30, 30)));
  bitmap_layer_set_compositing_mode(bt_icon_layer, GCompOpSet);
  bitmap_layer_set_bitmap(bt_icon_layer, bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(bt_icon_layer));
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  // Battery icon
  battery_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY);
  battery_icon_layer = bitmap_layer_create(PBL_IF_ROUND_ELSE(GRect(77, 136, 24, 24),GRect(117, -2, 24, 24)));
  bitmap_layer_set_compositing_mode(battery_icon_layer, GCompOpSet);
  bitmap_layer_set_bitmap(battery_icon_layer, battery_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(battery_icon_layer));

  // Battery meter
  battery_layer = layer_create(PBL_IF_ROUND_ELSE(GRect(80, 145, 16, 6),GRect(120, 7, 16, 6)));
  layer_set_update_proc(battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), battery_layer);

  if(persist_read_bool(AppKeyShowBattery)){
    layer_set_hidden(bitmap_layer_get_layer(battery_icon_layer),false);
    layer_set_hidden(battery_layer,false);
  } else {
    layer_set_hidden(bitmap_layer_get_layer(battery_icon_layer),true);
    layer_set_hidden(battery_layer,true);
  }
}

static void main_window_unload(Window *window) {
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  fonts_unload_custom_font(timeFont);
  fonts_unload_custom_font(dateFont);
  gbitmap_destroy(bg_bitmap);
  bitmap_layer_destroy(bg_bitmap_layer);
  gbitmap_destroy(bt_icon_bitmap);
  bitmap_layer_destroy(bt_icon_layer);
  gbitmap_destroy(battery_icon_bitmap);
  bitmap_layer_destroy(battery_icon_layer);
  layer_destroy(battery_layer);
}


static void init() {
  main_window = window_create();

  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  if(persist_read_bool(AppKeyShowBattery)){
    battery_state_service_subscribe(battery_callback);
    battery_callback(battery_state_service_peek());
  }

  app_message_register_inbox_received((AppMessageInboxReceived) inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
