#pragma once
#include <gui/raw_graphic.h>
#include <stdint.h>
namespace gui
{
    // FOR GRAPHIC MESSAGE :
    struct graphic_raw_request
    {
        uint8_t request_raw_data[256]; // 256 o = 32 uint64_t i think we have a lot
    } __attribute__((packed));
    struct create_graphic_window
    {
        char *name;
        uint64_t width;
        uint64_t height;
        bool resizable = false; // will not be used
    } __attribute__((packed));
    struct individual_request
    {
        uint64_t window_handler_code;
    } __attribute__((packed));
    struct set_window_pos_request
    {
        uint64_t window_handler_code;
        sys::raw_pos position;
    } __attribute__((packed));
    enum WINDOW_DEPTH_TYPE
    {
        BACKGROUND = 1,     // back
        TOP_BACKGROUND = 2, // like the menu bar
        ON_TOP = 3          // in front
    };

    struct window_depth_request
    {
        uint64_t window_handler_code;
        bool set;
        uint8_t type; // 0 on top
    } __attribute__((packed));
    enum GRAPHIC_SYSTEM_REQUEST
    {
        NULL_REQUEST = 0,
        CREATE_WINDOW = 1,
        GET_WINDOW_BACK_BUFFER = 2,
        SWAP_WINDOW_BUFFER = 3,
        GET_WINDOW_POSITION = 4,
        SET_WINDOW_POSITION = 5,
        WINDOW_DEPTH_ACTION = 6,
    };

    // window event are not used for the moment
    enum WINDOW_EVENT
    {
        EVENT_RESIZE = 0,
        EVENT_MOVE = 1,
    };

    struct graphic_system_service_protocol
    {
        uint16_t request_type;

        union
        {
            graphic_raw_request raw_information; // used for later, size : 256o (2048 byte)
            create_graphic_window create_window_info;
            individual_request get_request;
            set_window_pos_request set_pos;
            window_depth_request depth_request;
        };
    } __attribute__((packed));
    // FOR OTHER THING
    struct pixel
    {
        union
        {
            struct
            {
                uint8_t r;
                uint8_t g;
                uint8_t b;
                uint8_t a;
            };
            uint32_t pix;
        };
        constexpr pixel()
        {
            pix = 0;
        };
        constexpr pixel(uint32_t p)
        {
            pix = p;
        };
        constexpr pixel(uint8_t vr, uint8_t vg, uint8_t vb, uint8_t va = 255)
        {
            r = vr;
            g = vg;
            b = vb;
            a = va;
        }

        void blend_alpha(unsigned int colora, unsigned int alpha)
        {
            unsigned int rb1 = ((0x100 - alpha) * (colora & 0xFF00FF)) >> 8;
            unsigned int rb2 = (alpha * (pix & 0xFF00FF)) >> 8;
            unsigned int g1 = ((0x100 - alpha) * (colora & 0x00FF00)) >> 8;
            unsigned int g2 = (alpha * (pix & 0x00FF00)) >> 8;

            this->pix = ((rb1 | rb2) & 0xFF00FF) + ((g1 | g2) & 0x00FF00);
            a = 255;
        }
    };
    struct pos
    {
        int32_t x;
        int32_t y;
        pos()
        {
            x = 0;
            y = 0;
        }
        pos(int32_t vx, int32_t vy)
        {
            x = vx;
            y = vy;
        }
    } __attribute__((packed));

    class graphic_context
    {
        uint64_t context_width;
        uint64_t context_height;
        uint64_t wid = 0;
        pixel *back_buffer;
        char *context_name;
        int graphic_pid = 0;
        static constexpr int filter_width = 3;
        static constexpr int filter_height = 3;
        static constexpr int divisor = 16;
        int filter[9] = {
            1, 2, 1,
            2, 4, 2,
            1, 2, 1};

    public:
        graphic_context(uint64_t width, uint64_t height, const char *name) __attribute__((__target__("no-sse")));
        void draw_filled_circle(const pos origin, const int radius, const pixel color);
        constexpr uint64_t get_window_id() const
        {
            return wid;
        }
        enum filled_circle_part
        {
            TOP_RIGHT = 0,
            BOTTOM_RIGHT,
            BOTTOM_LEFT,
            TOP_LEFT
        };
        constexpr void set_pixel(pixel p, unsigned int x, unsigned int y)
        {

            back_buffer[(x) + (y)*context_width] = p;
        }
        void apply_blur(uint64_t fromx, uint64_t fromy, uint64_t width, uint64_t height);
        void draw_filled_circle_part(const pos origin, const int radius, const pixel color, const filled_circle_part part);
        void draw_rounded_rectangle(int radius, const uint64_t x, const uint64_t y, const uint64_t width, const uint64_t height, const pixel color);
        void draw_rounded_rectangle_b(int radius, const uint64_t x, const uint64_t y, const uint64_t width, const uint64_t height, const pixel color);
        void draw_rectangle(const uint64_t x, const uint64_t y, const uint64_t width, const uint64_t height, const pixel color);
        void draw_basic_char(const uint64_t x, const uint64_t y, const char chr, const pixel color);
        void draw_basic_string(const uint64_t x, const uint64_t y, const char *str, const pixel color);
        void clear_buffer(const pixel color);
        void swap_buffer();
        void set_on_top();
        void set_as_background();
        void set_on_top_of_background();
        bool is_on_top();
        bool is_mouse_inside();
        sys::raw_pos get_graphic_context_position();
        void set_graphic_context_position(const sys::raw_pos position);
    };
    uint64_t get_basic_font_width_text(const char *text);

    void swap_buffer(pixel *buffer1, const pixel *buffer2, uint64_t buffer_length);

    void raw_clear_buffer(pixel *buffer, uint64_t size, pixel value = {0, 0, 0, 255});
} // namespace gui
