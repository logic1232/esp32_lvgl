#include <lvgl.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
// #include <ArduinoUZlib.h> // 引入 Gzip 解码库
#include <FT6336U.h>
#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif
#define I2C_SDA 21 // 4
#define I2C_SCL 22 // 15
#define RST_N_PIN 5
#define INT_N_PIN 17
#define TFT_HOR_RES 480
#define TFT_VER_RES 320
#define TFT_ROTATION LV_DISPLAY_ROTATION_0
#define SCREEN_COUNT 2     // 页面个数
#define SWIPE_THRESHOLD 20 // 滑动距离
/*------------ 触摸驱动对象 ------------*/
FT6336U ft6336u(I2C_SDA, I2C_SCL, RST_N_PIN, INT_N_PIN);
FT6336U_TouchPointType tp;
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
lv_obj_t *screens[SCREEN_COUNT];
uint8_t current_screen = 0;

uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// const char *ssid1 = "CU_fAdh";
// const char *password1 = "7ffkzhbb";
const char *ssid1 = "wedf";
const char *password1 = "2001605aA";
// const char *ssid1 = "CU_TFdz";
// const char *password1 = "e4u397bk";
const char *ntpServer = "pool.ntp.org";
String url = "http://apis.juhe.cn/simpleWeather/query";
String city = "天津";
// String key = "8a52aa2381ca5c29e4f8b31c5e112345";
String key = "5910a2606b97e657991cbdd08c31e0c4";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
lv_color_t color_brown = lv_color_hex(0x8B4513);  // 代码中使用的浅棕色
lv_color_t color_yellow = lv_color_hex(0xFFFF00); // 黄色
lv_color_t color_green = lv_color_hex(0x00FF00);  // 绿色
lv_color_t color_blue = lv_color_hex(0x0000FF);   // 蓝色
lv_color_t color_navy = lv_color_hex(0x000080);   // 深蓝色
lv_color_t color_purple = lv_color_hex(0x800080); // 紫色
void create_lines();
// 声明函数
void fetch_weather_data(void *parameter);
void update_signal_strength(void *parameter);
#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf)
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_display_flush_ready(disp);
    // 实际的显示刷新代码应在这里
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{

    tp = ft6336u.scan();

    /* 判断屏幕是否被按下 */
    bool touched = tp.touch_count;
    if (touched)
    {
        /* 将获取的坐标传入 LVGL */
        data->point.x = TFT_HOR_RES - tp.tp[0].y;
        data->point.y = tp.tp[0].x;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    // 触摸屏读取函数
}

static uint32_t my_tick(void)
{
    return millis();
}

lv_obj_t *label_button;  // 标签对象用于显示日期和时间
lv_obj_t *label_date;    // 标签对象用于显示日期和时间
lv_obj_t *label_local;   // 标签对象用于地点
lv_obj_t *label_wid;     // 标签对象用于显示风力
lv_obj_t *label_aqi;     // 标签对象用于显示空气质量
lv_obj_t *label_weather; // 标签对象用于显示日期和时间
lv_obj_t *label_time;    // 标签对象用于显示日期和时间
lv_obj_t *label_weekday; // 标签对象用于显示星期几
lv_obj_t *line;
lv_obj_t *img;      // 图片对象
time_t currentTime; // 保存当前时间的全局变量
lv_obj_t *img1;     // 图片对象
lv_obj_t *img2;     // 图片对象 第一天天气
lv_obj_t *img3;     // 图片对象
lv_obj_t *img4;     // 图片对象
lv_obj_t *img5;     // 图片对象
lv_obj_t *img6;     // 图片对象
lv_obj_t *img_objects[5];
lv_obj_t *label_temp;
lv_obj_t *label_humidity;
lv_obj_t *labelxiegang;
lv_obj_t *label_TEMP_F1;
lv_obj_t *label_TEMP_F2;
lv_obj_t *label_TEMP_F3;
lv_obj_t *label_TEMP_F4;
lv_obj_t *label_TEMP_F5;

lv_obj_t *label_DATE_F1;
lv_obj_t *label_DATE_F2;
lv_obj_t *label_DATE_F3;
lv_obj_t *label_DATE_F4;
lv_obj_t *label_DATE_F5;

// 创建标签对象显示天气信息
lv_obj_t *label_weather_info;
static lv_point_precise_t line_points[] = {{0, 50}, {480, 50}};      // 画线
static lv_point_precise_t line_points1[] = {{0, 185}, {480, 185}};   // 画线
static lv_point_precise_t line_points2[] = {{96, 185}, {96, 320}};   // 画线
static lv_point_precise_t line_points3[] = {{192, 185}, {192, 320}}; // 画线
static lv_point_precise_t line_points4[] = {{288, 185}, {288, 320}}; // 画线
static lv_point_precise_t line_points5[] = {{384, 185}, {384, 320}}; // 画线
LV_IMG_DECLARE(sunny_48);
LV_IMG_DECLARE(cloudy_48);
LV_IMG_DECLARE(thunder_shower_48);
LV_IMG_DECLARE(small_rain_48);
LV_FONT_DECLARE(lv_chinese_25);
LV_IMG_DECLARE(local_32);
LV_IMG_DECLARE(wid_32);
LV_IMG_DECLARE(aqi_32);
LV_IMG_DECLARE(thunder_shower_128);
TaskHandle_t wifiTaskHandle = NULL; // 全局任务句柄，用于管理 wifi_task
WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, gmtOffset_sec, 60000); // 每60秒更新一次时间

void update_time_label(void *parameter)
{ // 时间更新任务
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 延迟 1 秒

    // 初始化 xLastWakeTime 变量
    xLastWakeTime = xTaskGetTickCount();
    while (true)
    {

        timeClient.update();
        String formattedTime = timeClient.getFormattedTime();

        // 将时间分解为 tm 结构
        struct tm timeinfo;
        time_t now = timeClient.getEpochTime();
        localtime_r(&now, &timeinfo);

        // 格式化日期和时间字符串
        char date_str[32];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d", &timeinfo);

        // 格式化星期几字符串
        char weekday_str[32];
        strftime(weekday_str, sizeof(weekday_str), "%A", &timeinfo);

        // 格式化时间字符串
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);

        // 更新标签内容和位置
        lv_label_set_text(label_date, date_str);
        lv_obj_align(label_date, LV_ALIGN_TOP_LEFT, 10, 10);

        lv_label_set_text(label_time, time_str);
        lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 340, 10);

        lv_label_set_text(label_weekday, weekday_str);
        lv_obj_align(label_weekday, LV_ALIGN_TOP_LEFT, 140, 10);

        Serial.println("running");

        // 延时1秒
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
void wifi_task(void *parameter)
{
    while (1)
    {
        Serial.println("Connecting to WiFi...");
        WiFi.begin(ssid1, password1);
        while (WiFi.status() != WL_CONNECTED)
        {
            vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒检查一次连接状态
        }
        Serial.println("Connected to WiFi");
        // 启动 NTP 客户端
        timeClient.begin();
        xTaskCreatePinnedToCore(update_signal_strength, "updata_signal_strength", 2048, NULL, 2, NULL, 1);
        // vTaskDelay(pdMS_TO_TICKS(000)); // 每秒检查一次连接状态
        xTaskCreatePinnedToCore(update_time_label, "UpdateTime", 4096, NULL, 3, NULL, 1);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒检查一次连接状态
        xTaskCreatePinnedToCore(fetch_weather_data, "FetchWeatherData", 4096, NULL, 2, NULL, 1);

        vTaskSuspend(NULL); // 连接成功后挂起任务
    }
}
void update_signal_strength(void *parameter)
{
    while (1)
    {
        int32_t rssi = WiFi.RSSI();
        Serial.println(rssi);
        LV_IMG_DECLARE(WIFI_1);
        LV_IMG_DECLARE(WIFI_2);
        LV_IMG_DECLARE(WIFI_3);
        LV_IMG_DECLARE(WIFI_4);
        // 检查WiFi连接状态
        if (WiFi.status() == WL_CONNECTED)
        {
            // 根据RSSI值更新图像
            if (rssi > -50)
            {
                lv_img_set_src(img, &WIFI_4);
            }
            else if (rssi > -60)
            {
                lv_img_set_src(img, &WIFI_3);
            }
            else if (rssi > -70)
            {
                lv_img_set_src(img, &WIFI_2);
            }
            else if (rssi > -80)
            {
                lv_img_set_src(img, &WIFI_1);
            }
            else
            {
                lv_img_set_src(img, &WIFI_1);
            }
            lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN); // 隐藏图片
            vTaskResume(wifiTaskHandle);
        }
        lv_obj_align(img, LV_ALIGN_TOP_RIGHT, -10, 5); // 设置图像位置

        vTaskDelay(pdMS_TO_TICKS(10000)); // 每10秒重试一次
    }
}

void lvgl_task(void *pvParameter)
{
    while (1)
    {
        lv_timer_handler();           // 处理 LVGL 定时器
        vTaskDelay(pdMS_TO_TICKS(2)); // 每 5 毫秒调用一次
    }
}
UBaseType_t taskCount;
static lv_coord_t prev_x = 0; // 移动到函数外部，并在初始化时设置

void switch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        bool state = lv_obj_has_state(obj, LV_STATE_CHECKED); // 获取开关状态
        digitalWrite(2, state ? HIGH : LOW);                  // 根据状态控制GPIO电平
    }
}

static void gesture_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e); // 获取事件类型
                                                 // lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_GESTURE)
    {
        lv_indev_t *indev = lv_indev_get_act();
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        if (dir == LV_DIR_RIGHT)
        {
            if (current_screen > 0)
            {
                current_screen--;
                lv_scr_load(screens[current_screen]);
                lv_scr_load_anim(screens[0], LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 100, 0, true);
            }
        }
        else if (dir == LV_DIR_LEFT)
        {
            // 左滑动，清理当前屏幕并创建新屏幕
            if (current_screen < SCREEN_COUNT - 1)
            {
                current_screen++;
                lv_scr_load(screens[current_screen]);
                lv_scr_load_anim(screens[1], LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 0, true);
            }
        }
    }
}

void create_screens()
{
    screens[0] = lv_obj_create(NULL);

    screens[1] = lv_obj_create(NULL);
    lv_obj_t *sw = lv_switch_create(screens[1]);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(sw, switch_event_handler, LV_EVENT_ALL, NULL); // 绑定事件回调
    // lv_obj_set_style_bg_color(screens[1], lv_color_hex(0xF0FDE0), LV_PART_MAIN);
    // lv_label_create(screens[1]);

    lv_obj_add_event_cb(screens[0], gesture_event_handler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(screens[1], gesture_event_handler, LV_EVENT_GESTURE, NULL);
}
void setup()
{
    pinMode(2, OUTPUT);
    Serial.begin(115200);
    ft6336u.begin();

    lv_init();

    lv_tick_set_cb(my_tick);

// lv_obj_t *screen2 = lv_obj_create(NULL);
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
    create_screens();
    lv_scr_load(screens[0]);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    // 创建按钮
    // lv_obj_t *sw = lv_switch_create(lv_screen_active());
    // lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
    //  lv_obj_t *sw1 = lv_switch_create(lv_screen_active());
    // lv_obj_align(sw1, LV_ALIGN_RIGHT_MID, 0, -20);

    // // 创建并初始化样式
    // static lv_style_t style_bg;
    // lv_style_init(&style_bg);
    // lv_style_set_bg_color(&style_bg, lv_color_hex(0xFFFFE0));
    // lv_obj_add_style(screens[0], &style_bg, LV_PART_MAIN);

    // 字符显示天气状况
    label_weather = lv_label_create(screens[0]);
    lv_obj_align(label_weather, LV_ALIGN_LEFT_MID, 130, -40);
    lv_obj_set_style_text_font(label_weather, &lv_chinese_25, 0);

    // 创建标签
    label_date = lv_label_create(lv_scr_act());
    lv_label_set_text(label_date, "Loading...");
    lv_obj_align(label_date, LV_ALIGN_TOP_LEFT, 10, 10);

    // 创建风向标签
    label_wid = lv_label_create(screens[0]);
    lv_label_set_text(label_wid, "Loading...");
    lv_obj_align(label_wid, LV_ALIGN_LEFT_MID, 50, 0);
    lv_obj_set_style_text_font(label_wid, &lv_chinese_25, 0);

    // 创建空气质量标签
    label_aqi = lv_label_create(screens[0]);
    lv_label_set_text(label_aqi, "Loading...");
    lv_obj_align(label_aqi, LV_ALIGN_LEFT_MID, 220, 0);
    lv_obj_set_style_text_font(label_aqi, &lv_chinese_25, 0);

    // 创建位置标签
    label_local = lv_label_create(screens[0]);
    lv_obj_align(label_local, LV_ALIGN_LEFT_MID, 50, -40);
    lv_obj_set_style_text_font(label_local, &lv_chinese_25, 0);

    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "Loading...");
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 340, 10);

    label_weekday = lv_label_create(lv_scr_act());
    lv_label_set_text(label_weekday, "Loading...");
    lv_obj_align(label_weekday, LV_ALIGN_TOP_LEFT, 140, 10);

    // 创建并设置线条
    create_lines();
    // 创建并设置标签样式
    static lv_style_t style_date_time;
    lv_style_init(&style_date_time);
    lv_style_set_text_font(&style_date_time, &lv_font_montserrat_18);
    lv_style_set_text_color(&style_date_time, lv_color_hex(0x000000)); // 标签文本颜色为黑色
    lv_obj_add_style(label_date, &style_date_time, 0);
    lv_obj_add_style(label_time, &style_date_time, 0);
    lv_obj_add_style(label_weekday, &style_date_time, 0);
    // 创建温度标签
    label_temp = lv_label_create(screens[0]);
    labelxiegang = lv_label_create(screens[0]);
    // lv_label_set_text(label_temp, "Temp:");
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 20, 60);
    lv_obj_align(labelxiegang, LV_ALIGN_TOP_LEFT, 120, 60);
    // 假设你已经将 LV_FONT_MONTSERRAT_18 字体添加到 LVGL
    lv_obj_set_style_text_font(labelxiegang, &lv_font_montserrat_38, 0);
    // 创建湿度标签
    label_humidity = lv_label_create(screens[0]);
    lv_obj_align(label_humidity, LV_ALIGN_TOP_LEFT, 140, 60); // 调整位置

    /*未来第一天天气 */
    label_TEMP_F1 = lv_label_create(screens[0]); // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_TEMP_F1, &lv_font_montserrat_22, 0);
    lv_obj_align(label_TEMP_F1, LV_ALIGN_BOTTOM_LEFT, 10, -10); // 将标签居中显示
    label_DATE_F1 = lv_label_create(screens[0]);                // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_DATE_F1, &lv_font_montserrat_22, 0);
    lv_obj_align(label_DATE_F1, LV_ALIGN_BOTTOM_LEFT, 10, -100); // 将标签居中显示
    /*未来第二天天气 */
    label_TEMP_F2 = lv_label_create(screens[0]); // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_TEMP_F2, &lv_font_montserrat_22, 0);
    lv_obj_align(label_TEMP_F2, LV_ALIGN_BOTTOM_LEFT, 106, -10); // 将标签居中显示
    label_DATE_F2 = lv_label_create(screens[0]);                 // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_DATE_F2, &lv_font_montserrat_22, 0);
    lv_obj_align(label_DATE_F2, LV_ALIGN_BOTTOM_LEFT, 106, -100); // 将标签居中显示

    /*未来第3天天气 */
    label_TEMP_F3 = lv_label_create(screens[0]); // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_TEMP_F3, &lv_font_montserrat_22, 0);
    lv_obj_align(label_TEMP_F3, LV_ALIGN_BOTTOM_LEFT, 202, -10); // 将标签居中显示
    label_DATE_F3 = lv_label_create(screens[0]);                 // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_DATE_F3, &lv_font_montserrat_22, 0);
    lv_obj_align(label_DATE_F3, LV_ALIGN_BOTTOM_LEFT, 202, -100); // 将标签居中显示
    /*未来第4天天气 */
    label_TEMP_F4 = lv_label_create(screens[0]); // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_TEMP_F4, &lv_font_montserrat_22, 0);
    lv_obj_align(label_TEMP_F4, LV_ALIGN_BOTTOM_LEFT, 298, -10); // 将标签居中显示
    label_DATE_F4 = lv_label_create(screens[0]);                 // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_DATE_F4, &lv_font_montserrat_22, 0);
    lv_obj_align(label_DATE_F4, LV_ALIGN_BOTTOM_LEFT, 298, -100); // 将标签居中显示
    /*未来第5天天气 */
    label_TEMP_F5 = lv_label_create(screens[0]); // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_TEMP_F5, &lv_font_montserrat_22, 0);
    lv_obj_align(label_TEMP_F5, LV_ALIGN_BOTTOM_LEFT, 390, -10); // 将标签居中显示
    label_DATE_F5 = lv_label_create(screens[0]);                 // 在屏幕上创建一个标签对象
    lv_obj_set_style_text_font(label_DATE_F5, &lv_font_montserrat_22, 0);
    lv_obj_align(label_DATE_F5, LV_ALIGN_BOTTOM_LEFT, 394, -100); // 将标签居中显示

    // 创建并设置湿度标签样式
    static lv_style_t style_humidity;
    lv_style_init(&style_humidity);
    lv_style_set_text_font(&style_humidity, &lv_font_montserrat_38);
    lv_style_set_text_color(&style_humidity, lv_color_hex(0x000000)); // 标签文本颜色为黑色
    lv_obj_add_style(label_humidity, &style_humidity, 0);

    // 创建并设置标签样式
    static lv_style_t style_temp;
    lv_style_init(&style_temp);
    lv_style_set_text_font(&style_temp, &lv_font_montserrat_38);
    lv_style_set_text_color(&style_temp, lv_color_hex(0x000000)); // 标签文本颜色为黑色
    lv_obj_add_style(label_temp, &style_temp, 0);
    // 设置初始时间为2024年8月1日13点46分
    struct tm initialTime;
    initialTime.tm_year = 2024 - 1900; // 年份是从1900开始的，所以需要减去1900
    initialTime.tm_mon = 8 - 1;        // 月份是从0开始的，所以需要减去1
    initialTime.tm_mday = 1;
    initialTime.tm_hour = 13;
    initialTime.tm_min = 46;
    initialTime.tm_sec = 0;
    currentTime = mktime(&initialTime);

    // 加载WiFi图像
    LV_IMG_DECLARE(WIFI);

    img = lv_image_create(lv_scr_act());
    img1 = lv_image_create(lv_scr_act());
    img2 = lv_image_create(lv_scr_act());
    img3 = lv_image_create(lv_scr_act());
    img4 = lv_image_create(lv_scr_act());
    img5 = lv_image_create(lv_scr_act());
    img6 = lv_image_create(lv_scr_act());
    lv_obj_align(img6, LV_ALIGN_LEFT_MID, 10, -40); // 将标签居中显示
    lv_obj_align(img2, LV_ALIGN_LEFT_MID, 10, 0);   // 将标签居中显示
    lv_obj_align(img3, LV_ALIGN_LEFT_MID, 180, 0);  // 将标签居中显示
    //   lv_image_set_src(img, &WIFI);
    //  lv_obj_align(img, LV_ALIGN_TOP_RIGHT, -10, 10); // 设置图像位置
    // 添加事件处理函数

    Serial.println("Setup done");

    // 创建WiFi连接任务
    xTaskCreate(wifi_task, "WiFiTask", 8192, NULL, 1, &wifiTaskHandle);
    // 创建FreeRTOS任务

    xTaskCreatePinnedToCore(lvgl_task, "LVGL Task", 6144, NULL, 10, NULL, 1);
    //  taskCount = uxTaskGetNumberOfTasks();
}

void loop()
{
    //   lv_timer_handler();
    // delay(5);

    // Serial.print("Number of tasks: ");
    // Serial.println(taskCount);
}
// 创建线条对象并设置其点和样式
void create_lines()
{
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_color(&style_line, lv_color_hex(0x000000)); // 线颜色为黑色
    lv_style_set_line_width(&style_line, 3);                      // 线宽度

    // 创建并设置每条线
    lv_obj_t *line;

    // 创建第一条线
    line = lv_line_create(screens[0]);
    lv_line_set_points(line, line_points, 2);
    lv_obj_add_style(line, &style_line, 0);

    // 创建第二条线
    line = lv_line_create(screens[0]);
    lv_line_set_points(line, line_points1, 2);
    lv_obj_add_style(line, &style_line, 0);

    // 创建第三条线
    line = lv_line_create(screens[0]);
    lv_line_set_points(line, line_points2, 2);
    lv_obj_add_style(line, &style_line, 0);

    // 创建第四条线
    line = lv_line_create(screens[0]);
    lv_line_set_points(line, line_points3, 2);
    lv_obj_add_style(line, &style_line, 0);

    // 创建第五条线
    line = lv_line_create(screens[0]);
    lv_line_set_points(line, line_points4, 2);
    lv_obj_add_style(line, &style_line, 0);

    // 创建第六条线
    line = lv_line_create(screens[0]);
    lv_line_set_points(line, line_points5, 2);
    lv_obj_add_style(line, &style_line, 0);

    // // 创建第七条线
    // line = lv_line_create(screens[0]);
    // lv_line_set_points(line, line_points6, 2);
    // lv_obj_add_style(line, &style_line, 0);
}
int temp, humidity;
void fetch_weather_data(void *parameter)
{
    String aqi_level;
    lv_label_set_text(labelxiegang, "/");
    // 初始化图像对象和标签
    for (int i = 0; i < 5; i++)
    {
        img_objects[i] = lv_img_create(screens[0]);
        lv_obj_align(img_objects[i], LV_ALIGN_BOTTOM_LEFT, 20 + (96 * i), -50); // 设置图像位置
    }
    while (true)
    {

        if (WiFi.status() == WL_CONNECTED)
        {

            HTTPClient http;
            http.begin(url + "?city=" + city + "&key=" + key);
            int http_code = http.GET();
            String response = http.getString();

            if (http_code == HTTP_CODE_OK)
            {
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, response);
                temp = doc["result"]["realtime"]["temperature"].as<int>();
                humidity = doc["result"]["realtime"]["humidity"].as<int>();
                String info = doc["result"]["realtime"]["info"].as<String>();
                String wid_direct = doc["result"]["realtime"]["direct"].as<String>();
                String wid_strength = doc["result"]["realtime"]["power"].as<String>();
                int aqi_data = doc["result"]["realtime"]["aqi"].as<int>();

                if (aqi_data < 50)
                {
                    aqi_level = "优";
                }
                else if (aqi_data < 100)
                {
                    aqi_level = "良";
                }
                else if (aqi_data < 150)
                {
                    aqi_level = "中等";
                }
                else if (aqi_data < 200)
                {
                    aqi_level = "差";
                }
                else if (aqi_data < 300)
                {
                    aqi_level = "很差";
                }
                else if (aqi_data < 500)
                {
                    aqi_level = "严重";
                }

                String wid = wid_direct + wid_strength;
                String aqi = aqi_level + "(" + String(aqi_data) + ")";
                lv_image_set_src(img2, &wid_32);
                lv_image_set_src(img3, &aqi_32);
                lv_label_set_text(label_aqi, aqi.c_str());
                lv_label_set_text(label_wid, wid.c_str());
                lv_label_set_text(label_local, "天津");
                lv_image_set_src(img6, &local_32);
                lv_label_set_text(label_weather, info.c_str());
                auto futureWeather1 = doc["result"]["future"][0];
                auto futureWeather2 = doc["result"]["future"][1];
                auto futureWeather3 = doc["result"]["future"][2];
                auto futureWeather4 = doc["result"]["future"][3];
                auto futureWeather5 = doc["result"]["future"][4];
                /*未来温度 */
                String Temp_f1 = futureWeather1["temperature"].as<String>();
                Temp_f1.replace("℃", "");
                String Temp_f1_show = Temp_f1 + "°C";
                String Temp_f2 = futureWeather2["temperature"].as<String>();
                Temp_f2.replace("℃", "");
                String Temp_f2_show = Temp_f2 + "°C";
                String Temp_f3 = futureWeather3["temperature"].as<String>();
                Temp_f3.replace("℃", "");
                String Temp_f3_show = Temp_f3 + "°C";
                String Temp_f4 = futureWeather4["temperature"].as<String>();
                Temp_f4.replace("℃", "");
                String Temp_f4_show = Temp_f4 + "°C";
                String Temp_f5 = futureWeather5["temperature"].as<String>();
                Temp_f5.replace("℃", "");
                String Temp_f5_show = Temp_f5 + "°C";
                /*未来时间 */
                String Date_f1 = futureWeather1["date"].as<String>();
                String Date_f2 = futureWeather2["date"].as<String>();
                String Date_f3 = futureWeather3["date"].as<String>();
                String Date_f4 = futureWeather4["date"].as<String>();
                String Date_f5 = futureWeather5["date"].as<String>();
                // 去掉前面的 "2024-"
                String shortDate1 = Date_f1.substring(5); // 从第6个字符开始提取
                String shortDate2 = Date_f2.substring(5); // 从第6个字符开始提取
                String shortDate3 = Date_f3.substring(5); // 从第6个字符开始提取
                String shortDate4 = Date_f4.substring(5); // 从第6个字符开始提取
                String shortDate5 = Date_f5.substring(5); // 从第6个字符开始提取
                int weathers[] = {
                    futureWeather1["wid"]["day"].as<int>(),
                    futureWeather2["wid"]["day"].as<int>(),
                    futureWeather3["wid"]["day"].as<int>(),
                    futureWeather4["wid"]["day"].as<int>(),
                    futureWeather5["wid"]["day"].as<int>(),
                };

                for (int i = 0; i < 5; i++)
                {

                    if (weathers[i] == 00) // 晴
                    {
                        lv_img_set_src(img_objects[i], &sunny_48);
                    }
                    else if (weathers[i] == 01 || weathers[i] == 02) // 阴，多云
                    {
                        lv_img_set_src(img_objects[i], &cloudy_48);
                    }
                    else if (weathers[i] == 04) // 雷阵雨
                    {
                        lv_img_set_src(img_objects[i], &thunder_shower_48);
                    }
                     else if (weathers[i] == 07) // 小雨
                    {
                        lv_img_set_src(img_objects[i], &small_rain_48);
                    }

                    // else if (weathers[i] == 05)  //雷阵雨+冰雹  没搞
                    // {
                    //     lv_img_set_src(img_objects[i], &thunder_shower_48);
                    // }
                    // else if (weathers[i] == 06)  //雪加雨  没搞
                    // {
                    //     lv_img_set_src(img_objects[i], &thunder_shower_48);
                    // }
                    // else if (weathers[i] == 05)  //雷阵雨+冰雹  没搞
                    // {
                    //     lv_img_set_src(img_objects[i], &thunder_shower_48);
                    // }
                    // else if (weathers[i] == 05)  //雷阵雨+冰雹  没搞
                    // {
                    //     lv_img_set_src(img_objects[i], &thunder_shower_48);
                    // }
                    // else if (weathers[i] == 05)  //雷阵雨+冰雹  没搞
                    // {
                    //     lv_img_set_src(img_objects[i], &thunder_shower_48);
                    // }
                    // else if (weathers[i] == 05)  //雷阵雨+冰雹  没搞
                    // {
                    //     lv_img_set_src(img_objects[i], &thunder_shower_48);
                    // }

                    else
                    {
                    }
                }

                /*未来时间 */
                lv_label_set_text(label_DATE_F1, shortDate1.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_DATE_F2, shortDate2.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_DATE_F3, shortDate3.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_DATE_F4, shortDate4.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_DATE_F5, shortDate5.c_str()); // 设置标签的文本内容

                /*未来温度 */
                lv_label_set_text(label_TEMP_F1, Temp_f1_show.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_TEMP_F2, Temp_f2_show.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_TEMP_F3, Temp_f3_show.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_TEMP_F4, Temp_f4_show.c_str()); // 设置标签的文本内容
                lv_label_set_text(label_TEMP_F5, Temp_f5_show.c_str()); // 设置标签的文本内容

                // 更新标签内容
                String temp_str = String(temp) + "°C";
                String humidity_str = String(humidity) + "%";

                // 更新温度标签内容和颜色
                lv_label_set_text(label_temp, temp_str.c_str());
                lv_color_t temp_color;
                if (temp <= 0)
                {
                    temp_color = lv_color_hex(0x0000FF); // 蓝色
                }
                else if (temp <= 15)
                {
                    temp_color = lv_color_hex(0x00FFFF); // 青色
                }
                else if (temp <= 25)
                {
                    temp_color = lv_color_hex(0x00FF00); // 绿色
                }
                else if (temp <= 35)
                {
                    temp_color = lv_color_hex(0xFFFF00); // 黄色
                }
                else if (temp <= 40)
                {
                    temp_color = lv_color_hex(0xFFA500); // 橙色
                }
                else
                {
                    temp_color = lv_color_hex(0xFF0000); // 红色
                }
                lv_obj_set_style_text_color(label_temp, temp_color, 0);

                // 更新湿度标签内容和颜色
                lv_label_set_text(label_humidity, humidity_str.c_str());
                lv_color_t humidity_color;
                if (humidity < 30)
                {
                    humidity_color = lv_color_hex(0xFF0000); // 红色 (干燥)
                }
                else if (humidity < 60)
                {
                    humidity_color = lv_color_hex(0x00FF00); // 绿色 (舒适)
                }
                else
                {
                    humidity_color = lv_color_hex(0x0000FF); // 蓝色 (潮湿)
                }
                lv_obj_set_style_text_color(label_humidity, humidity_color, 0);

                // 阴晴图片显示
                if (info == "雷阵雨")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    lv_img_set_src(img1, &thunder_shower_128);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 90, -40); // 设置图像位置
                }
                else if (info == "阵雨")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(shower_128);
                    lv_img_set_src(img1, &shower_128);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 90, -40); // 设置图像位置
                }
                else if (info == "小雨")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(small_rain);
                    lv_img_set_src(img1, &small_rain);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }
                else if (info == "多云")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(cloudy_light);
                    lv_img_set_src(img1, &cloudy_light);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }
                else if (info == "阴")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(cloudy_light);
                    lv_img_set_src(img1, &cloudy_light);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }
                else if (info == "阴转多云")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(cloudy_light);
                    lv_img_set_src(img1, &cloudy_light);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }
                else if (info == "阴转多云")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(cloudy_light);
                    lv_img_set_src(img1, &cloudy_light);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }
                else if (info == "大雨")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(hearvyrain_128);
                    lv_img_set_src(img1, &hearvyrain_128);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }
                else if (info == "晴")
                {
                    lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
                    LV_IMG_DECLARE(sunny_light);
                    lv_img_set_src(img1, &sunny_light);
                    lv_obj_align(img1, LV_ALIGN_CENTER, 50, -60); // 设置图像位置
                }

                else
                {

                    lv_obj_add_flag(img1, LV_OBJ_FLAG_HIDDEN); // 隐藏图片
                }
            }
            else
            {
                Serial.printf("HTTP GET failed with code %d\n", http_code);
            }
            http.end();
        }
        else
        {
            Serial.println("WiFi not connected");
        }
        int state = digitalRead(2);

        // 反转引脚状态
        if (state == HIGH)
        {
            digitalWrite(2, LOW);
        }
        else
        {
            digitalWrite(2, HIGH);
        }
        vTaskDelay(pdMS_TO_TICKS(1800000)); // 每分钟请求一次
    }
}