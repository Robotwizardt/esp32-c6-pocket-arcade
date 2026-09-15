#include <driver/gpio.h>
/*
 * Pocket Arcade Lobby
 *
 * The display/touch bring-up in this project is adapted from the official
 * Waveshare ESP32-C6 Touch AMOLED 2.16 ESP-IDF example. The UI below is
 * intentionally independent of the individual games: add a GameEntry and a
 * launch callback when a game is ready, without changing the lobby layout.
 */

#include <stdint.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_random.h>

#include "lvgl.h"
#include "fonts/lobby_fonts.h"
#include "game_2048_storage.h"
#include "game_2048_ui.h"
#include "game_snake_ui.h"
#include "game_stack_ui.h"
#include "game_lights_ui.h"
#include "game_tilt_ui.h"
#include "imu_port.h"
#include "game_reaction_ui.h"
#include "game_breakout_ui.h"
#include "game_mines_ui.h"
#include "game_memory_ui.h"
#include "lvgl_bsp.h"
#include "power_bsp.h"
#include "user_config.h"

namespace {

static const char *TAG = "game_lobby";

constexpr uint8_t kGamesPerPage = 4;
constexpr uint8_t kDefaultBrightness = 78;

struct GameEntry {
    const char *id;
    const char *title;
    const char *subtitle;
    const char *description;
    const char *badge;
    const char *category;
    uint32_t accent;
    void (*launch)(void);
};

static void request_lobby(void);
static void launch_2048(void);
static void launch_snake(void);
static void launch_stack(void);
static void launch_lights(void);
static void launch_tilt(void);
static void launch_reaction(void);
static void launch_breakout(void);
static void launch_mines(void);
static void launch_memory(void);

/* A launch callback keeps the lobby's navigation contract stable for game modules. */
static const GameEntry kGames[] = {
    {"merge-2048", "2048", "合并数字", "滑动合并相同数字，挑战更高分数。", "数", "益智", 0xF59E0B, launch_2048},
    {"snake", "贪吃蛇", "经典街机", "全屏滑动转向，吃到食物后温和加速。", "蛇", "街机", 0x22C55E, launch_snake},
    {"lights", "灯泡全灭", "点亮思路", "点击切换自身及相邻灯泡，将所有灯泡熄灭。", "灯", "益智", 0xFBBF24, launch_lights},
    {"stack", "叠叠高", "精准落块", "点击落下方块，对齐堆叠，挑战更高层数。", "叠", "反应", 0x38BDF8, launch_stack},
    {"breakout", "打砖块", "击碎砖块", "移动挡板接住小球，清除所有砖块。", "砖", "街机", 0x38BDF8, launch_breakout},
    {"memory", "记忆翻牌", "配对挑战", "记住卡片的位置，用更少的步数找到所有配对。", "牌", "益智", 0xA78BFA, launch_memory},
    {"mines", "扫雷", "逻辑推理", "根据数字判断地雷位置，找出所有安全方格。", "雷", "益智", 0xFB7185, launch_mines},
    {"reaction", "反应挑战", "手速测试", "目标出现后立即点击，测试你的反应速度。", "快", "反应", 0x2DD4BF, launch_reaction},
    {"tilt", "重力滚球", "倾斜闯关", "校准方向后倾斜屏幕，让小球穿过迷宫进入终点。", "球", "体感", 0x4ADE80, launch_tilt},
};

constexpr uint8_t kGameCount = sizeof(kGames) / sizeof(kGames[0]);
constexpr uint8_t kPageCount = (kGameCount + kGamesPerPage - 1) / kGamesPerPage;

static I2cMasterBus *g_i2c_bus = nullptr;
static DisplayPort *g_display = nullptr;
static lv_obj_t *g_active_screen = nullptr;
static lv_obj_t *g_brightness_modal = nullptr;
static lv_obj_t *g_brightness_value_label = nullptr;
static lv_obj_t *g_light_button_label = nullptr;
static uint8_t g_page = 0;
static bool g_lobby_visible = false;
static uint8_t g_brightness = kDefaultBrightness;

static constexpr uint32_t kBackground = 0x070A12;
static constexpr uint32_t kSurface = 0x111827;
static constexpr uint32_t kSurfaceRaised = 0x172033;
static constexpr uint32_t kSurfacePressed = 0x26364E;
static constexpr uint32_t kText = 0xF8FAFC;
static constexpr uint32_t kTextMuted = 0xA8B3C7;
static constexpr uint32_t kTextDim = 0x66738A;
static constexpr uint32_t kLine = 0x263247;

static lv_color_t color(uint32_t hex)
{
    return lv_color_hex(hex);
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                            uint32_t text_color = kText)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color(text_color), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    return label;
}

static lv_obj_t *make_button(lv_obj_t *parent, int32_t x, int32_t y, int32_t width,
                             int32_t height, const char *text, uint32_t fill = kSurfaceRaised)
{
    lv_obj_t *button = lv_obj_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(button, color(fill), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, color(kSurfacePressed), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, color(kLine), 0);
    lv_obj_set_style_border_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);

    lv_obj_t *label = make_label(button, text, &lobby_zh_20);
    lv_obj_center(label);
    return button;
}

static void set_button_label_color(lv_obj_t *button, uint32_t text_color)
{
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != nullptr) {
        lv_obj_set_style_text_color(label, color(text_color), 0);
    }
}

static lv_obj_t *make_screen(void)
{
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_set_size(screen, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_set_style_bg_color(screen, color(kBackground), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_radius(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    return screen;
}

// Called only at startup or from deferred navigation callbacks. Free the old
// view before building the next one so their widget trees never overlap in RAM.
static void begin_screen_transition()
{
    g_lobby_visible = false;
    lv_obj_t *previous = g_active_screen ? g_active_screen : lv_screen_active();
    lv_obj_t *blank = make_screen();
    g_active_screen = blank;
    lv_screen_load(blank);
    if (previous) lv_obj_delete(previous);
    g_brightness_modal = nullptr;
    g_brightness_value_label = nullptr;
    g_light_button_label = nullptr;
}

static void replace_screen(lv_obj_t *next)
{
    lv_obj_t *previous = g_active_screen;
    g_active_screen = next;
    lv_screen_load(next);
    if (previous != nullptr && previous != next) {
        lv_obj_delete_async(previous);
    }
}

static void show_lobby(void);
static void show_detail(const GameEntry *game);

static void create_page_dots(lv_obj_t *parent)
{
    constexpr int32_t kDotSize = 8;
    constexpr int32_t kActiveWidth = 24;
    constexpr int32_t kGap = 8;
    int32_t total_width = (kPageCount - 1) * kGap;
    for (uint8_t index = 0; index < kPageCount; ++index) {
        total_width += (index == g_page) ? kActiveWidth : kDotSize;
    }

    int32_t x = (BSP_LCD_H_RES - total_width) / 2;
    for (uint8_t index = 0; index < kPageCount; ++index) {
        const int32_t width = (index == g_page) ? kActiveWidth : kDotSize;
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_pos(dot, x, 428);
        lv_obj_set_size(dot, width, kDotSize);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(dot, color(index == g_page ? 0x7DD3FC : 0x334155), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        x += width + kGap;
    }
}

static void show_lobby_async(void *)
{
    show_lobby();
}

static void show_detail_async(void *arg)
{
    show_detail(static_cast<const GameEntry *>(arg));
}

static void request_lobby(void)
{
    lv_async_call(show_lobby_async, nullptr);
}

static void launch_2048(void)
{
    // The game owns its screen and asks the lobby to be restored through the
    // same asynchronous navigation path used by the detail page.
    lv_obj_t *game_screen = game2048::create_screen(request_lobby);
    replace_screen(game_screen);
}

static void launch_snake(void)
{
    gameSnake::configure(esp_random());
    lv_obj_t *game_screen = gameSnake::create_screen(request_lobby);
    replace_screen(game_screen);
}

static void launch_stack(void)
{
    replace_screen(gameStack::create_screen(request_lobby));
}

static void launch_lights(void)
{
    gameLights::configure(esp_random());
    replace_screen(gameLights::create_screen(request_lobby));
}

static void launch_memory(void)
{
    gameMemory::configure(esp_random());
    replace_screen(gameMemory::create_screen(request_lobby));
}

static void launch_mines(void)
{
    gameMines::configure(esp_random());
    replace_screen(gameMines::create_screen(request_lobby));
}

static void launch_breakout(void)
{
    replace_screen(gameBreakout::create_screen(request_lobby));
}

static void launch_reaction(void)
{
    gameReaction::configure(esp_random());
    replace_screen(gameReaction::create_screen(request_lobby));
}

static void launch_game_async(void *arg)
{
    const GameEntry *selected = static_cast<const GameEntry *>(arg);
    if (selected && selected->launch) {
        begin_screen_transition();
        selected->launch();
    }
}

static void launch_tilt(void)
{
    gameTilt::configure(imuPort::read, imuPort::active);
    replace_screen(gameTilt::create_screen(request_lobby));
}

static void request_detail(const GameEntry *game)
{
    lv_async_call(show_detail_async, const_cast<GameEntry *>(game));
}

static void game_card_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    request_detail(static_cast<const GameEntry *>(lv_event_get_user_data(event)));
}

static void change_page(intptr_t delta)
{
    if (!g_lobby_visible || g_brightness_modal) return;
    if (delta < 0 && g_page > 0) {
        --g_page;
    } else if (delta > 0 && g_page + 1 < kPageCount) {
        ++g_page;
    } else {
        return;
    }
    g_lobby_visible = false;
    request_lobby();
}

static void page_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED)
        change_page(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
}

struct PageKey { gpio_num_t pin; bool raw=false, stable=false; uint32_t changed=0; };
static PageKey g_page_keys[]={{GPIO_NUM_9},{GPIO_NUM_10}};
static void poll_page_keys(lv_timer_t *)
{
    const uint32_t now=lv_tick_get();
    bool pressed[2]={};
    for(unsigned i=0;i<2;++i){
        auto &key=g_page_keys[i];bool down=gpio_get_level(key.pin)==0;
        if(down!=key.raw){key.raw=down;key.changed=now;}
        if(key.stable!=key.raw && uint32_t(now-key.changed)>=40){
            key.stable=key.raw;pressed[i]=key.stable;
        }
    }
    if(g_page_keys[0].raw && g_page_keys[1].raw) return;
    if(pressed[0]) change_page(-1);
    if(pressed[1]) change_page(1);
}
static void init_page_keys()
{
    gpio_config_t config={};config.pin_bit_mask=(1ULL<<9)|(1ULL<<10);
    config.mode=GPIO_MODE_INPUT;config.pull_up_en=GPIO_PULLUP_ENABLE;
    config.pull_down_en=GPIO_PULLDOWN_DISABLE;config.intr_type=GPIO_INTR_DISABLE;
    if(gpio_config(&config)!=ESP_OK){ESP_LOGW(TAG,"Page buttons unavailable");return;}
    for(auto &key:g_page_keys){key.raw=key.stable=gpio_get_level(key.pin)==0;key.changed=lv_tick_get();}
    lv_timer_create(poll_page_keys,10,nullptr);
    ESP_LOGI(TAG,"Page keys ready: previous=GPIO9 next=GPIO10");
}

static void brightness_value_changed(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    lv_obj_t *slider = lv_event_get_target_obj(event);
    g_brightness = static_cast<uint8_t>(lv_slider_get_value(slider));
    if (g_display != nullptr) {
        g_display->Set_Backlight(g_brightness);
    }
    if (g_brightness_value_label != nullptr) {
        char value_text[12];
        snprintf(value_text, sizeof(value_text), "%u%%", g_brightness);
        lv_label_set_text(g_brightness_value_label, value_text);
    }
    if (g_light_button_label != nullptr) {
        char light_text[20];
        snprintf(light_text, sizeof(light_text), "亮度 %u%%", g_brightness);
        lv_label_set_text(g_light_button_label, light_text);
    }
}

static void close_brightness(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || g_brightness_modal == nullptr) {
        return;
    }
    lv_obj_t *modal = g_brightness_modal;
    g_brightness_modal = nullptr;
    g_brightness_value_label = nullptr;
    lv_obj_delete_async(modal);
}

static void open_brightness(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || g_brightness_modal != nullptr || g_active_screen == nullptr) {
        return;
    }

    lv_obj_t *scrim = lv_obj_create(g_active_screen);
    g_brightness_modal = scrim;
    lv_obj_set_size(scrim, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_set_pos(scrim, 0, 0);
    lv_obj_add_flag(scrim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(scrim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scrim, color(0x000000), 0);
    lv_obj_set_style_bg_opa(scrim, 190, 0);
    lv_obj_set_style_border_width(scrim, 0, 0);
    lv_obj_set_style_pad_all(scrim, 0, 0);

    lv_obj_t *panel = lv_obj_create(scrim);
    lv_obj_set_size(panel, 336, 224);
    lv_obj_center(panel);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, color(0x151F32), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, color(0x394A67), 0);
    lv_obj_set_style_radius(panel, 22, 0);
    /* Positions below are deliberately relative to the full panel. */
    lv_obj_set_style_pad_all(panel, 0, 0);

    lv_obj_t *title = make_label(panel, "屏幕亮度", &lobby_zh_24);
    lv_obj_set_pos(title, 24, 20);
    lv_obj_t *hint = make_label(panel, "滑动调节屏幕亮度", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(hint, 24, 51);

    lv_obj_t *slider = lv_slider_create(panel);
    lv_obj_set_pos(slider, 24, 86);
    lv_obj_set_size(slider, 288, 34);
    lv_slider_set_range(slider, 20, 100);
    lv_slider_set_value(slider, g_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, color(0x26364D), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, color(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, color(kText), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, brightness_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    char value_text[12];
    snprintf(value_text, sizeof(value_text), "%u%%", g_brightness);
    g_brightness_value_label = make_label(panel, value_text, &lobby_zh_20, 0x7DD3FC);
    lv_obj_set_pos(g_brightness_value_label, 268, 126);

    lv_obj_t *done = make_button(panel, 24, 153, 120, 48, "完成", 0x1E3A5F);
    lv_obj_add_event_cb(done, close_brightness, LV_EVENT_CLICKED, nullptr);
}

static lv_obj_t *create_game_card(lv_obj_t *parent, const GameEntry &game, int32_t x, int32_t y)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, 208, 132);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, color(kSurface), 0);
    lv_obj_set_style_bg_color(card, color(kSurfacePressed), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, color(kLine), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_opa(card, 25, 0);
    lv_obj_set_style_shadow_color(card, color(0x000000), 0);
    lv_obj_add_event_cb(card, game_card_event, LV_EVENT_CLICKED, const_cast<GameEntry *>(&game));

    lv_obj_t *accent = lv_obj_create(card);
    lv_obj_set_pos(accent, 0, 0);
    lv_obj_set_size(accent, 8, 132);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(accent, color(game.accent), 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 4, 0);

    lv_obj_t *badge = lv_obj_create(card);
    lv_obj_set_pos(badge, 18, 18);
    lv_obj_set_size(badge, 52, 52);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(badge, color(game.accent), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_color(badge, color(game.accent), 0);
    lv_obj_set_style_radius(badge, 14, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_t *badge_label = make_label(badge, game.badge, &lobby_zh_20, game.accent);
    lv_obj_center(badge_label);

    lv_obj_t *title = make_label(card, game.title, &lobby_zh_24);
    lv_obj_set_pos(title, 84, 16);
    lv_obj_set_width(title, 108);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);

    lv_obj_t *subtitle = make_label(card, game.subtitle, &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(subtitle, 84, 55);
    lv_obj_set_width(subtitle, 108);
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_DOT);

    lv_obj_t *category = make_label(card, game.category, &lobby_zh_16, game.accent);
    lv_obj_set_pos(category, 18, 96);

    lv_obj_t *status = make_label(card, game.launch != nullptr ? "可游玩" : "待上线",
                                  &lobby_zh_16, game.launch != nullptr ? game.accent : kTextDim);
    lv_obj_set_pos(status, 142, 96);
    return card;
}

static void update_battery(lv_timer_t *timer)
{
    auto *label = static_cast<lv_obj_t *>(lv_timer_get_user_data(timer));
    const BatteryStatus status = ReadBatteryStatus();
    char text[80];
    uint32_t tint = kTextMuted;
    if (!status.valid) {
        snprintf(text, sizeof(text), "电量 --\n读取失败");
    } else if (!status.connected) {
        snprintf(text, sizeof(text), "%s\n未接电池", status.usb ? "USB供电" : "电量 --");
    } else {
        if (status.percent >= 0 && status.percent <= 100)
            snprintf(text, sizeof(text), "电量 %d%%\n%s", status.percent,
                     status.charging ? "充电中" : (status.usb ? "USB供电" : "电池供电"));
        else
            snprintf(text, sizeof(text), "电量 --\n%s", status.charging ? "充电中" : "电池供电");
        tint = status.charging ? 0x4ADE80 : (status.percent >= 0 && status.percent <= 20 ? 0xFBBF24 : kTextMuted);
    }
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color(tint), 0);
}

static void delete_battery_timer(lv_event_t *event)
{
    lv_timer_delete(static_cast<lv_timer_t *>(lv_event_get_user_data(event)));
}

static void show_lobby(void)
{
    begin_screen_transition();
    g_lobby_visible = true;
    lv_obj_t *screen = make_screen();

    lv_obj_t *accent = lv_obj_create(screen);
    lv_obj_set_pos(accent, 24, 24);
    lv_obj_set_size(accent, 8, 40);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(accent, color(0x38BDF8), 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 4, 0);

    lv_obj_t *brand = make_label(screen, "游戏大厅", &lobby_zh_24);
    lv_obj_set_pos(brand, 44, 21);
    lv_obj_t *subhead = make_label(screen, "掌上游戏乐园", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(subhead, 45, 55);

    lv_obj_t *battery = make_label(screen, "", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(battery, 220, 25);
    lv_obj_set_width(battery, 122);
    lv_timer_t *battery_timer = lv_timer_create(update_battery, 10000, battery);
    lv_obj_add_event_cb(battery, delete_battery_timer, LV_EVENT_DELETE, battery_timer);
    update_battery(battery_timer);

    char light_text[20];
    snprintf(light_text, sizeof(light_text), "亮度 %u%%", g_brightness);
    lv_obj_t *light = make_button(screen, 350, 22, 106, 44, light_text, 0x13233A);
    g_light_button_label = lv_obj_get_child(light, 0);
    lv_obj_add_event_cb(light, open_brightness, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *line = lv_obj_create(screen);
    lv_obj_set_pos(line, 24, 82);
    lv_obj_set_size(line, 432, 1);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(line, color(kLine), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);

    lv_obj_t *prompt = make_label(screen, "点击卡片，查看游戏介绍", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(prompt, 24, 94);

    char page_text[16];
    snprintf(page_text, sizeof(page_text), "%u / %u 页", static_cast<unsigned>(g_page + 1), static_cast<unsigned>(kPageCount));
    lv_obj_t *page = make_label(screen, page_text, &lobby_zh_16, kTextDim);
    lv_obj_set_pos(page, 371, 94);

    const int32_t positions[][2] = {{24, 116}, {248, 116}, {24, 260}, {248, 260}};
    const uint8_t first = g_page * kGamesPerPage;
    for (uint8_t slot = 0; slot < kGamesPerPage; ++slot) {
        const uint8_t index = first + slot;
        if (index < kGameCount) {
            create_game_card(screen, kGames[index], positions[slot][0], positions[slot][1]);
        }
    }

    lv_obj_t *previous = make_button(screen, 24, 410, 82, 48, "上一页", 0x13233A);
    lv_obj_add_event_cb(previous, page_event, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(-1)));
    if (g_page == 0) {
        lv_obj_set_style_bg_color(previous, color(0x0E1522), 0);
        set_button_label_color(previous, kTextDim);
        lv_obj_clear_flag(previous, LV_OBJ_FLAG_CLICKABLE);
    }

    create_page_dots(screen);

    lv_obj_t *next = make_button(screen, 374, 410, 82, 48, "下一页", 0x13233A);
    lv_obj_add_event_cb(next, page_event, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(1)));
    if (g_page + 1 >= kPageCount) {
        lv_obj_set_style_bg_color(next, color(0x0E1522), 0);
        set_button_label_color(next, kTextDim);
        lv_obj_clear_flag(next, LV_OBJ_FLAG_CLICKABLE);
    }

    replace_screen(screen);
}

static void show_detail(const GameEntry *game)
{
    if (game == nullptr) {
        return;
    }

    begin_screen_transition();
    lv_obj_t *screen = make_screen();
    lv_obj_t *back = make_button(screen, 24, 22, 104, 48, "返回", 0x13233A);
    lv_obj_add_event_cb(back, [](lv_event_t *event) {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            request_lobby();
        }
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *section = make_label(screen, "游戏详情", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(section, 150, 38);

    lv_obj_t *hero = lv_obj_create(screen);
    lv_obj_set_pos(hero, 24, 94);
    lv_obj_set_size(hero, 432, 146);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hero, color(kSurface), 0);
    lv_obj_set_style_bg_opa(hero, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hero, 1, 0);
    lv_obj_set_style_border_color(hero, color(game->accent), 0);
    lv_obj_set_style_radius(hero, 20, 0);
    lv_obj_set_style_pad_all(hero, 0, 0);

    lv_obj_t *hero_badge = lv_obj_create(hero);
    lv_obj_set_pos(hero_badge, 24, 28);
    lv_obj_set_size(hero_badge, 88, 88);
    lv_obj_clear_flag(hero_badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(hero_badge, color(game->accent), 0);
    lv_obj_set_style_bg_opa(hero_badge, LV_OPA_20, 0);
    lv_obj_set_style_border_width(hero_badge, 1, 0);
    lv_obj_set_style_border_color(hero_badge, color(game->accent), 0);
    lv_obj_set_style_radius(hero_badge, 22, 0);
    lv_obj_set_style_pad_all(hero_badge, 0, 0);
    lv_obj_t *hero_badge_label = make_label(hero_badge, game->badge, &lobby_zh_28, game->accent);
    lv_obj_center(hero_badge_label);

    lv_obj_t *title = make_label(hero, game->title, &lobby_zh_28);
    lv_obj_set_pos(title, 136, 20);
    lv_obj_t *subtitle = make_label(hero, game->subtitle, &lobby_zh_20, kTextMuted);
    lv_obj_set_pos(subtitle, 138, 64);
    lv_obj_t *category = make_label(hero, game->category, &lobby_zh_16, game->accent);
    lv_obj_set_pos(category, 138, 105);

    lv_obj_t *description_panel = lv_obj_create(screen);
    lv_obj_set_pos(description_panel, 24, 260);
    lv_obj_set_size(description_panel, 432, 124);
    lv_obj_clear_flag(description_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(description_panel, color(0x0D1422), 0);
    lv_obj_set_style_bg_opa(description_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(description_panel, 0, 0);
    lv_obj_set_style_radius(description_panel, 18, 0);
    lv_obj_set_style_pad_all(description_panel, 0, 0);
    lv_obj_t *description_title = make_label(description_panel, "玩法介绍", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(description_title, 20, 16);
    lv_obj_t *description = make_label(description_panel, game->description, &lobby_zh_20, kText);
    lv_obj_set_pos(description, 20, 43);
    lv_obj_set_width(description, 385);
    lv_label_set_long_mode(description, LV_LABEL_LONG_WRAP);

    lv_obj_t *launch = nullptr;
    if (game->launch != nullptr) {
        launch = make_button(screen, 24, 408, 432, 48, "开始游戏", 0x1E3A5F);
        lv_obj_add_event_cb(launch, [](lv_event_t *event) {
            if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
                return;
            }
            const GameEntry *selected = static_cast<const GameEntry *>(lv_event_get_user_data(event));
            if (selected != nullptr && selected->launch != nullptr) {
                lv_async_call(launch_game_async, const_cast<GameEntry *>(selected));
            }
        }, LV_EVENT_CLICKED, const_cast<GameEntry *>(game));
    } else {
        launch = make_button(screen, 24, 408, 432, 48, "敬请期待", 0x1A2434);
        lv_obj_clear_flag(launch, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_border_color(launch, color(game->accent), 0);
        set_button_label_color(launch, game->accent);
    }

    replace_screen(screen);
}

} // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Pocket Arcade lobby");

    const bool storage_ready = game2048_storage::initialize();
    game2048::configure(esp_random(), game2048_storage::load, game2048_storage::save);
    ESP_LOGI(TAG, "2048 archive storage: %s", storage_ready ? "NVS ready" : "RAM only");

    g_i2c_bus = new I2cMasterBus(BSP_I2C_SCL, BSP_I2C_SDA, BSP_I2C_NUM);
    Custom_PmicPortInit(g_i2c_bus, 0x34);
    imuPort::initialize(g_i2c_bus->Get_I2cBusHandle());

    /* The Waveshare board uses CS=15 and touch INT=5; keep them explicit. */
    g_display = new DisplayPort(*g_i2c_bus,
                                BSP_LCD_H_RES,
                                BSP_LCD_V_RES,
                                BSP_LCD_PCLK,
                                BSP_LCD_DATA0,
                                BSP_LCD_DATA1,
                                BSP_LCD_DATA2,
                                BSP_LCD_DATA3,
                                BSP_LCD_CS,
                                BSP_LCD_TOUCH_INT,
                                BSP_LCD_TOUCH_RST,
                                BSP_LCD_SPI_NUM);
    g_display->DisplayPort_TouchInit();
    g_display->Set_Backlight(g_brightness);
    Lvgl_PortInit(*g_display);

    if (Lvgl_lock(-1) == ESP_OK) {
        show_lobby();
        init_page_keys();
        Lvgl_unlock();
        ESP_LOGI(TAG, "Chinese lobby ready: 480x480, %u entries, brightness %u%%", kGameCount, g_brightness);
    }
}
