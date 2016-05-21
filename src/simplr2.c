#include <pebble.h>

#define KEY_HOURS_24 0


static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;
static TextLayer *s_steps_layer;

static bool hours_24 = true;


/******************************
      Read config updates
*******************************/

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // 24 hour mode selected?
  Tuple *hours_24_t = dict_find(iter, KEY_HOURS_24);
  if(hours_24_t && hours_24_t->value->int8 > 0) {  // Read boolean as an integer
    
    // Apply changes
    hours_24 = true;
    // Persist value
    persist_write_bool(KEY_HOURS_24, true);
  } else {
    hours_24 = false;
    persist_write_bool(KEY_HOURS_24, false);
  }
 
}

/******************************
   Handlers for event updates
*******************************/

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "+++");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
  // Display Time
  static char s_time_text[] = "00:00";

  if (hours_24) {
    strftime(s_time_text, sizeof(s_time_text), "%H:%M", tick_time);
  } else {
    strftime(s_time_text, sizeof(s_time_text), "%H:%p", tick_time);
  }
  

  text_layer_set_text(s_time_layer, s_time_text);

  // Display Date
  static char s_date_text[] = "Jan 10";
  strftime(s_date_text, sizeof(s_date_text), "%b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_text);

  // Display Steps
  static char steps_text[] = "00000 steps";

  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    snprintf(steps_text, sizeof(steps_text), "%d steps", (int)health_service_sum_today(metric));
  } else {
    snprintf(steps_text, sizeof(steps_text), "%d steps", 0);
  }
  text_layer_set_text(s_steps_layer, steps_text);

}

static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_connection_layer, connected ? "connected" : "disconnected");
  text_layer_set_text_color(s_connection_layer, connected ? GColorSpringBud : GColorOrange);
}

/******************************
   Load & unload of layers
*******************************/

static void main_window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);


  s_battery_layer = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(38, 12, bounds.size.w-76, 32),
    GRect(8, 6, bounds.size.w-16, 32)));
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_text(s_battery_layer, "100%");

  s_date_layer = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(38, 12, bounds.size.w-76, 32),
    GRect(8, 6, bounds.size.w-16, 32)));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  text_layer_set_text(s_date_layer, "Jan 1");

  s_time_layer = text_layer_create(GRect(0, 48, bounds.size.w, 56));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorMidnightGreen);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  s_steps_layer = text_layer_create(GRect(0, 100, bounds.size.w, 32));
  text_layer_set_text_color(s_steps_layer, GColorBlack);
  text_layer_set_background_color(s_steps_layer, GColorCadetBlue);
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);
  text_layer_set_text(s_steps_layer, "0 steps");

  s_connection_layer = text_layer_create(GRect(0, 136, bounds.size.w, 20));
  text_layer_set_text_color(s_connection_layer, GColorBlack);
  text_layer_set_background_color(s_connection_layer, GColorClear);
  text_layer_set_font(s_connection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentCenter);
  handle_bluetooth(connection_service_peek_pebble_app_connection());

  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(handle_battery);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));

  handle_battery(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_steps_layer);
}

/******************************
        Initializiation
*******************************/

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
