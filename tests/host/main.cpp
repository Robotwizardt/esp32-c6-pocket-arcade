#include "capture.h"
#include "../../main/main.cpp"

int main(int argc, char **argv) {
    init_display();
    app_main();
    settle();
    auto key_pump=[](unsigned ms){for(unsigned i=0;i<ms;i+=10){lv_tick_inc(10);lv_timer_handler();}};
    test_gpio_levels[10]=0;key_pump(20);test_gpio_levels[10]=1;key_pump(60);
    require(g_page==0,"short bounce ignored");
    test_gpio_levels[10]=0;key_pump(100);require(g_page==1,"right physical key next page");
    key_pump(1000);require(g_page==1,"held key does not repeat");
    test_gpio_levels[10]=1;key_pump(60);test_gpio_levels[9]=0;key_pump(100);
    require(g_page==0,"left physical key previous page");test_gpio_levels[9]=1;key_pump(60);
    test_gpio_levels[9]=test_gpio_levels[10]=0;key_pump(100);require(g_page==0,"simultaneous keys ignored");
    test_gpio_levels[9]=test_gpio_levels[10]=1;key_pump(60);
    test_gpio_levels[9]=0;key_pump(100);require(g_page==0,"first page boundary");test_gpio_levels[9]=1;key_pump(60);
    screenshot("lobby-1.ppm");
    require(find_text(lv_screen_active(), "电量 76%\n充电中"), "battery percentage and charging state");
    test_battery={true,false,false,true,-1};lv_tick_inc(10000);settle();
    require(find_text(lv_screen_active(), "USB供电\n未接电池"), "USB without battery is not zero percent");
    screenshot("battery-usb.ppm");
    test_battery={false,false,false,false,-1};lv_tick_inc(10000);settle();
    require(find_text(lv_screen_active(), "电量 --\n读取失败"), "read failure is explicit");
    test_battery={true,true,false,false,15};lv_tick_inc(10000);settle();
    require(find_text(lv_screen_active(), "电量 15%\n电池供电"), "battery discharging state");
    screenshot("battery-low.ppm");
    test_battery={true,true,true,true,76};lv_tick_inc(10000);settle();

    dump_tree(lv_screen_active());
    tap(60, 160);
    require(find_text(lv_screen_active(), "游戏详情"), "tap badge opens card");
    test_gpio_levels[10]=0;key_pump(100);require(find_text(lv_screen_active(),"游戏详情")&&g_page==0,"physical keys ignored in details");test_gpio_levels[10]=1;key_pump(60);

    tap(60, 44);
    require(find_text(lv_screen_active(), "游戏大厅"), "touch back");
    tap(64, 434);
    require(g_page == 0, "disabled prev must not navigate");
    click_label("2048");
    screenshot("detail.ppm");
    require(find_text(lv_screen_active(), "开始游戏"), "2048 launch enabled");
    click_label("开始游戏");
    require(find_text(lv_screen_active(), "撤销"), "2048 game opens from lobby");
    screenshot("2048-lobby-launch.ppm");
    click_label("返回");
    click_label("贪吃蛇");
    click_label("开始游戏");
    require(find_text(lv_screen_active(), "滑动开始"), "snake launches ready");
    click_label("返回");
    click_label("灯泡全灭"); click_label("开始游戏");
    require(gameLights::state().board != 0, "lights entry"); click_label("返回");
    click_label("叠叠高"); click_label("开始游戏");
    require(gameStack::state().phase == gameStack::Phase::Ready, "stack entry"); click_label("返回");
    click_label("下一页");
    screenshot("lobby-2.ppm");
    require(find_text(lv_screen_active(), "反应挑战"), "second page");
    const char* remaining[]={"打砖块","记忆翻牌","扫雷","反应挑战"};
    for(const char* title:remaining){click_label(title);click_label("开始游戏");click_label("返回");require(g_page==1,"game exit keeps second page");}
    click_label("反应挑战");
    click_label("返回");
    require(find_text(lv_screen_active(), "2 / 3 页"), "return preserves page");
    click_label("下一页");
    click_label("重力滚球");click_label("开始游戏");
    require(find_text(lv_screen_active(),"方向校准"),"motion game calibrates before start");
    click_label("返回");require(g_page==2,"tilt exit preserves third page");
    test_gpio_levels[10]=0;key_pump(100);require(g_page==2,"last page boundary");test_gpio_levels[10]=1;key_pump(60);
    click_label("上一页");
    click_label("上一页");
    click_label("亮度 78%");
    test_gpio_levels[10]=0;key_pump(100);require(g_page==0 && g_brightness_modal,"modal blocks hardware paging");test_gpio_levels[10]=1;key_pump(60);
    screenshot("brightness.ppm");
    dump_tree(lv_screen_active());
    tap(415, 434);
    require(g_page == 0 && g_brightness_modal, "modal blocks underlying next button");
    tap(384, 232);
    require(g_brightness == 100 && g_display->brightness == 100, "brightness upper limit reaches hardware callback");
    tap(97, 232);
    require(g_brightness == 20 && g_display->brightness == 20, "brightness lower limit reaches hardware callback");
    click_label("完成");
    require(find_text(lv_screen_active(), "亮度 20%"), "brightness header updates");
    click_label("亮度 20%");
    require(find_text(lv_screen_active(), "20%"), "reopening preserves brightness");
    auto panel = lv_obj_get_child(g_brightness_modal, 0);
    lv_obj_t *slider = nullptr;
    for(uint32_t i = 0; i < lv_obj_get_child_count(panel); ++i) {
        auto child = lv_obj_get_child(panel, i);
        if(lv_obj_check_type(child, &lv_slider_class)) slider = child;
    }
    require(slider, "brightness slider present");
    lv_slider_set_value(slider, 78, LV_ANIM_OFF);
    lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, nullptr);
    click_label("完成");
    auto all_games_cycle = []() {
        const char *first[] = {"2048", "贪吃蛇", "灯泡全灭", "叠叠高"};
        const char *second[] = {"打砖块", "记忆翻牌", "扫雷", "反应挑战"};
        for(const char *title : first) { click_label(title); click_label("开始游戏"); tap(240,240); click_label("返回"); }
        click_label("下一页");
        for(const char *title : second) { click_label(title); click_label("开始游戏"); tap(240,240); click_label("返回"); }
        click_label("上一页");
    };
    for(int i=0;i<2;++i) all_games_cycle();
    lv_mem_monitor_t mixed_before,mixed_after;lv_mem_monitor(&mixed_before);
    for(int i=0;i<20;++i) all_games_cycle();
    lv_mem_monitor(&mixed_after);
    require(mixed_after.used_cnt==mixed_before.used_cnt && mixed_after.free_size+512>=mixed_before.free_size,"eight-game switches retain no allocations");
    std::printf("PASS: 160 mixed game entries; heap %zu -> %zu\n",mixed_before.free_size,mixed_after.free_size);
    lv_mem_monitor_t before, after;
    // Warm the LVGL draw caches before taking a steady-state heap baseline.
    for(int i = 0; i < 5; ++i) {
        click_label("2048"); click_label("返回");
        click_label("下一页"); click_label("反应挑战"); click_label("返回"); click_label("上一页");
        click_label("亮度 78%"); click_label("完成");
    }
    lv_mem_monitor(&before);
    const int cycles = argc > 1 ? std::atoi(argv[1]) : 100;
    for(int i = 0; i < cycles; ++i) {
        click_label("2048"); click_label("返回");
        click_label("下一页"); click_label("反应挑战"); click_label("返回"); click_label("上一页");
        click_label("亮度 78%"); click_label("完成");
    }
    lv_mem_monitor(&after);
    std::printf("HEAP before=%zu after=%zu max_used=%zu\n", before.free_size, after.free_size, after.max_used);
    require(after.used_cnt == before.used_cnt, "navigation retains no extra live allocations");
    require(after.free_size + 512 >= before.free_size, "allocator fragmentation remains bounded");
    std::printf("PASS: touch, brightness, modal blocking; %d navigation/settings cycles\n", cycles);
}

