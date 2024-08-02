#include <lvgl.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif

#define TFT_HOR_RES   480
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0

#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];
const char * ssid1="CU_fAdh";
const char * password1="7ffkzhbb";
#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char * buf) {
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t * px_map) {
    lv_display_flush_ready(disp);
    // 实际的显示刷新代码应在这里
}

void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    // 触摸屏读取函数
}

static uint32_t my_tick(void) {
    return millis();
}

lv_obj_t *label_date; // 标签对象用于显示日期和时间
lv_obj_t *label_time; // 标签对象用于显示日期和时间
lv_obj_t *label_weekday;   // 标签对象用于显示星期几
time_t currentTime; // 保存当前时间的全局变量
lv_obj_t *line;
static lv_point_precise_t line_points[] = { {0, 50},{480, 50} }; // 画线

void update_time_label(void *parameter) { // 时间更新任务
    while (true) {
        // 获取当前时间
        struct tm * timeinfo = localtime(&currentTime);

        // 格式化日期和时间字符串
        char date_str[32];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);

        // 格式化星期几字符串
        char weekday_str[32];
        strftime(weekday_str, sizeof(weekday_str), "%A", timeinfo);

 // 格式化日期和时间字符串
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);


    // 创建日期标签
   
    lv_label_set_text(label_date, date_str);
    lv_obj_align(label_date, LV_ALIGN_TOP_LEFT, 10, 10);

 // 时间标签
   
    lv_label_set_text(label_time, time_str);
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 240, 10);


    // 创建星期几标签
  
    lv_label_set_text(label_weekday, weekday_str);
    lv_obj_align(label_weekday, LV_ALIGN_TOP_LEFT, 140, 10);



        // 更新时间
        currentTime += 1; // 每次调用这个函数，增加1秒

        // 延时1秒
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    
    lv_init();
    lv_tick_set_cb(my_tick);

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif

    lv_display_t *disp;
#if LV_USE_TFT_ESPI
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);
#else
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    // 创建并初始化样式
    static lv_style_t style_bg;
    lv_style_init(&style_bg);

    // 设置背景颜色为黄色
    lv_style_set_bg_color(&style_bg, lv_color_hex(0xd1d498)); // 黄色

    // 应用样式到屏幕
    lv_obj_add_style(lv_scr_act(), &style_bg, LV_PART_MAIN);

    // 创建日期标签
    label_date = lv_label_create(lv_scr_act());
    lv_label_set_text(label_date, "Loading...");
    lv_obj_align(label_date, LV_ALIGN_TOP_LEFT, 10, 10);

 // 创建日期标签
    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "Loading...");
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 340, 10);


    // 创建星期几标签
    label_weekday = lv_label_create(lv_scr_act());
    lv_label_set_text(label_weekday, "Loading...");
    lv_obj_align(label_weekday, LV_ALIGN_TOP_LEFT, 140, 10);

    // 创建并设置线
    line = lv_line_create(lv_scr_act());
    lv_line_set_points(line, line_points, 2);
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_color(&style_line, lv_color_hex(0x000000)); // 黑色
    lv_style_set_line_width(&style_line, 3); // 线宽
    lv_obj_add_style(line, &style_line, 0);

    // 创建并设置日期和时间标签样式
    static lv_style_t style_date_time;
    lv_style_init(&style_date_time);
    lv_style_set_text_font(&style_date_time, &lv_font_montserrat_18);
    lv_style_set_text_color(&style_date_time, lv_color_hex(0x000000)); // 黑色
    lv_obj_add_style(label_date, &style_date_time, 0);
    lv_obj_add_style(label_time, &style_date_time, 0);
    lv_obj_add_style(label_weekday, &style_date_time, 0);
    // 创建并设置星期几标签样式
    static lv_style_t style_weekday;
    lv_style_init(&style_weekday);
    lv_style_set_text_font(&style_weekday, &lv_font_montserrat_18);
    lv_style_set_text_color(&style_weekday, lv_color_hex(0x000000)); // 黑色
    lv_obj_add_style(label_weekday, &style_weekday, 0);

    // 设置初始时间为2024年8月1日13点46分
    struct tm initialTime;
    initialTime.tm_year = 2024 - 1900;  // 年份是从1900开始的，所以需要减去1900
    initialTime.tm_mon = 8 - 1;         // 月份是从0开始的，所以需要减去1
    initialTime.tm_mday = 1;
    initialTime.tm_hour = 13;
    initialTime.tm_min = 46;
    initialTime.tm_sec = 0;
    currentTime = mktime(&initialTime);

    // 创建FreeRTOS任务
    xTaskCreate(update_time_label, "UpdateTime", 2048, NULL, 1, NULL);

    Serial.println("Setup done");
}

void loop() {
    lv_timer_handler();
    delay(5);
}
