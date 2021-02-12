#pragma once
#include <gui/graphic_system.h>
#include <stddef.h>

namespace gui
{
    class widget
    {
    protected:
        int64_t widget_width;
        int64_t widget_height;
        int64_t widget_x;
        int64_t widget_y;
        bool widget_should_draw = true;
        bool is_position_inside_widget(const pos pos);

    public:
        widget();
        virtual void update_widget() = 0;
        virtual void draw_widget(graphic_context &context) = 0;
        constexpr bool &should_redraw()
        {
            return widget_should_draw;
        }
        // note: i do void* as c++ doesn't like recursive include
        virtual void init_widget(void *new_parent) = 0;
    };

    class widget_list
    {

        widget **list;
        uint64_t list_length = 0;

    public:
        void init(size_t length);

        void add_widget(widget *widget);

        bool update_all();
        void draw_all(graphic_context &context);
    };
} // namespace gui
