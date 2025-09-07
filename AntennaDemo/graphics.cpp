#include <bitset>
#include <algorithm>

#include "graphics.h"

void draw_polygon_wireframe(Polygon polygon_screen, Framebuffer& framebuffer) {
    const Vec3f& v0_pos_orig = polygon_screen.vertices[0].pos;
    const Vec3f& v1_pos_orig = polygon_screen.vertices[1].pos;
    const Vec3f& v2_pos_orig = polygon_screen.vertices[2].pos;

    Vec3f v0_pos_copy = v0_pos_orig;
    Vec3f v1_pos_copy = v1_pos_orig;
    Vec3f v2_pos_copy = v2_pos_orig;

    if (clip_line_coh_suth(v0_pos_copy.x, v0_pos_copy.y, v1_pos_copy.x, v1_pos_copy.y, framebuffer)) {
        const int x0 = static_cast<int>(std::round(v0_pos_copy.x));
        const int y0 = static_cast<int>(std::round(v0_pos_copy.y));
        const int x1 = static_cast<int>(std::round(v1_pos_copy.x));
        const int y1 = static_cast<int>(std::round(v1_pos_copy.y));
        draw_line_dda(x0, y0, x1, y1, sf::Color::White, framebuffer);
    }

    // Восстанавливаем после клиппинга.
    v0_pos_copy = v0_pos_orig;
    if (clip_line_coh_suth(v0_pos_copy.x, v0_pos_copy.y, v2_pos_copy.x, v2_pos_copy.y, framebuffer)) {
        const int x0 = static_cast<int>(std::round(v0_pos_copy.x));
        const int y0 = static_cast<int>(std::round(v0_pos_copy.y));
        const int x1 = static_cast<int>(std::round(v2_pos_copy.x));
        const int y1 = static_cast<int>(std::round(v2_pos_copy.y));
        draw_line_dda(x0, y0, x1, y1, sf::Color::White, framebuffer);
    }

    // Восстанавливаем после клиппинга.
    v1_pos_copy = v1_pos_orig;
    v2_pos_copy = v2_pos_orig;
    if (clip_line_coh_suth(v1_pos_copy.x, v1_pos_copy.y, v2_pos_copy.x, v2_pos_copy.y, framebuffer)) {
        const int x0 = static_cast<int>(std::round(v1_pos_copy.x));
        const int y0 = static_cast<int>(std::round(v1_pos_copy.y));
        const int x1 = static_cast<int>(std::round(v2_pos_copy.x));
        const int y1 = static_cast<int>(std::round(v2_pos_copy.y));
        draw_line_dda(x0, y0, x1, y1, sf::Color::White, framebuffer);
    }
}

void draw_line_dda(int x0, int y0, int x1, int y1, sf::Color color, Framebuffer& framebuffer) {
    // Vertical.
    if (x0 == x1) {
        // Make ascending.
        if (y0 > y1)
            std::swap(y0, y1);

        for (int y = y0; y <= y1; y++)
            set_pixel_color(x0, y, color, framebuffer);
    }
    // Horizontal.
    else if (y0 == y1) {
        // Make ascending.
        if (x0 > x1)
            std::swap(x0, x1);

        for (int x = x0; x <= x1; x++)
            set_pixel_color(x, y0, color, framebuffer);
    }

    int dy = y1 - y0;
    int dx = x1 - x0;

    // Non-steep.
    if (std::abs(dy) <= std::abs(dx)) {
        // Make ascending.
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
            dx = -dx;
            dy = -dy;
        }

        const float slope = static_cast<float>(dy) / dx;

        float y_accum = static_cast<float>(y0);
        for (int x = x0; x <= x1; x++) {
            int y = static_cast<int>(std::round(y_accum));
            set_pixel_color(x, y, color, framebuffer);
            y_accum += slope;
        }
    }
    // Steep.
    else {
        // Make ascending.
        if (y0 > y1) {
            std::swap(y0, y1);
            std::swap(x0, x1);
            dy = -dy;
            dx = -dx;
        }

        const float inv_slope = static_cast<float>(dx) / dy;

        float x_accum = static_cast<float>(x0);
        for (int y = y0; y <= y1; y++) {
            int x = static_cast<int>(std::round(x_accum));
            set_pixel_color(x, y, color, framebuffer);
            x_accum += inv_slope;
        }
    }
}

void set_pixel_color(int x, int y, sf::Color color, Framebuffer& framebuffer) {
    const int index = (y * framebuffer.w + x) * 4;

    framebuffer.rgba_array[index] = color.r;
    framebuffer.rgba_array[index + 1] = color.g;
    framebuffer.rgba_array[index + 2] = color.b;
    framebuffer.rgba_array[index + 3] = color.a;
}

void clear_framebuffer(sf::Color color, Framebuffer& framebuffer) {
    for (size_t i = 0; i < framebuffer.rgba_array.size(); i += 4) {
        framebuffer.rgba_array[i] = color.r;
        framebuffer.rgba_array[i + 1] = color.g;
        framebuffer.rgba_array[i + 2] = color.b;
        framebuffer.rgba_array[i + 3] = color.a;
    }
}

void clear_z_buffer(float depth_value, ZBuffer& z_buffer) {
    std::ranges::fill(z_buffer.depth_array, 1.0f);
}

bool clip_line_coh_suth(float& x0, float& y0, float& x1, float& y1, const Framebuffer& framebuffer) {
    enum EdgeBit {
        left = 3,
        right = 2,
        top = 1,
        bottom = 0
    };

    struct Point {
        float x;
        float y;
        std::bitset<4> region_code;
    };

    Point p0{ x0, y0 };
    p0.region_code.set(EdgeBit::left, p0.x < 0);
    p0.region_code.set(EdgeBit::right, p0.x > framebuffer.w - 1);
    p0.region_code.set(EdgeBit::top, p0.y < 0);
    p0.region_code.set(EdgeBit::bottom, p0.y > framebuffer.h - 1);

    Point p1{ x1, y1 };
    p1.region_code.set(EdgeBit::left, p1.x < 0);
    p1.region_code.set(EdgeBit::right, p1.x > framebuffer.w - 1);
    p1.region_code.set(EdgeBit::top, p1.y < 0);
    p1.region_code.set(EdgeBit::bottom, p1.y > framebuffer.h - 1);

    bool line_inside = (p0.region_code | p1.region_code).none();
    bool line_outside = (p0.region_code & p1.region_code).any();
    while (!line_inside && !line_outside) {
        // Make sure the first point is the one that is outside.
        if (p0.region_code.none())
            std::swap(p0, p1);

        // Find the first edge outside of which the point is.
        EdgeBit first_edge{};
        for (int i = 3; i >= 0; --i) {
            if (p0.region_code[i]) {
                first_edge = static_cast<EdgeBit>(i);
                break;
            }
        }

        if (first_edge == EdgeBit::left || first_edge == EdgeBit::right) {
            float edge_x = first_edge == EdgeBit::left ? 0.0f : static_cast<float>(framebuffer.w - 1);
            float slope = (p1.y - p0.y) / (p1.x - p0.x);

            float x_excess = edge_x - p0.x;
            p0.x = edge_x;
            p0.y += x_excess * slope;
        }
        else {
            float edge_y = first_edge == EdgeBit::top ? 0.0f : static_cast<float>(framebuffer.h - 1);
            float inv_slope = (p1.x - p0.x) / (p1.y - p0.y);

            float y_excess = edge_y - p0.y;
            p0.y = edge_y;
            p0.x += y_excess * inv_slope;
        }

        p0.region_code.set(EdgeBit::left, p0.x < 0);
        p0.region_code.set(EdgeBit::right, p0.x > framebuffer.w - 1);
        p0.region_code.set(EdgeBit::top, p0.y < 0);
        p0.region_code.set(EdgeBit::bottom, p0.y > framebuffer.h - 1);

        line_inside = (p0.region_code | p1.region_code).none();
        line_outside = (p0.region_code & p1.region_code).any();
    }

    x0 = p0.x;
    y0 = p0.y;
    x1 = p1.x;
    y1 = p1.y;

    return line_inside;
}

void draw_polygon_solid(Polygon polygon_screen, Framebuffer& framebuffer, ZBuffer& z_buffer) {
    Vertex v0 = polygon_screen.vertices[0];
    Vertex v1 = polygon_screen.vertices[1];
    Vertex v2 = polygon_screen.vertices[2];

    // Сортируем вершины по y по возрастанию.
    if (v0.pos.y > v1.pos.y)
        std::swap(v0, v1);
    if (v1.pos.y > v2.pos.y)
        std::swap(v1, v2);
    if (v0.pos.y > v1.pos.y)
        std::swap(v0, v1);

    // Trivial reject по верхней/нижней границе.
    // NOTE: Нет смысла исключать ноль для верхней границы, поскольку в итоге все-равно
    // получим ceil(0)-1 для y-координаты нижней скан-линии и растеризации не будет.
    // Для нижней же границы ноль нужно исключить, чтобы не было пропуска пикселов по нижней стороне смежного треугольника.
    if (v2.pos.y <= 0.0f || v0.pos.y > (framebuffer.h - 1))
        return;

    // Trivial reject по левой границе.
    // NOTE: Нет смысла исключать ноль для левой границы, поскольку в итоге все-равно
    // получим ceil(0)-1 для концов всех скан-линий и растеризации не будет.
    if ((v0.pos.x <= 0.0f) && (v1.pos.x <= 0.0f) && (v2.pos.x <= 0.0f))
        return;

    // Trivial reject по правой границе.
    // NOTE: Важно исключить ноль, чтобы не было пропуска пикселов по правой стороне смежного треугольника.
    if ((v0.pos.x > (framebuffer.w - 1)) && (v1.pos.x > (framebuffer.w - 1)) && (v2.pos.x > (framebuffer.w - 1)))
        return;

    // Классифицируем треугольник.
    // NOTE: Проверяем на точное равенство, а не через epsilon, поскольку в противном случае
    // возможна некорректная растеризация: пропуск пиксела или двойная растеризация одного и того же
    // пиксела для двух смежных треугольников.
    const bool flat_bottom = (v1.pos.y == v2.pos.y);
    const bool flat_top = (v0.pos.y == v1.pos.y);
    if (flat_bottom)
        draw_flat_bottom_polygon_solid(Polygon{ {v0, v1, v2}, polygon_screen.albedo_color }, framebuffer, z_buffer);
    else if (flat_top)
        draw_flat_top_polygon_solid(Polygon{ {v0, v1, v2}, polygon_screen.albedo_color }, framebuffer, z_buffer);
    else { // Разделяем треугольник на flat_bottom и flat_top.
        const float inv_slope = (v2.pos.x - v0.pos.x) / (v2.pos.y - v0.pos.y); // Наклон самого длинного ребра.
        const float height_top_triangle = v1.pos.y - v0.pos.y;

        // Точка пересечения на длинном ребре при разделении треугольников.
        const float intersect_x = v0.pos.x + height_top_triangle * inv_slope;
        const float intersect_y = v1.pos.y;

        // Определяем z-атрибут точки пересечения, интерполируя вдоль самого длинного ребра.
        const float z_slope_vert = (v2.pos.z - v0.pos.z) / (v2.pos.y - v0.pos.y);
        const float intersect_z = v0.pos.z + (height_top_triangle * z_slope_vert);

        Vertex intersect{ {intersect_x, intersect_y, intersect_z} };

        draw_flat_bottom_polygon_solid(Polygon{ {v0, intersect, v1}, polygon_screen.albedo_color }, framebuffer, z_buffer);
        draw_flat_top_polygon_solid(Polygon{ {intersect, v1, v2}, polygon_screen.albedo_color }, framebuffer, z_buffer);
    }
}

void draw_flat_bottom_polygon_solid(Polygon polygon_screen, Framebuffer& framebuffer, ZBuffer& z_buffer) {
    Vertex v0 = polygon_screen.vertices[0];
    Vertex v1 = polygon_screen.vertices[1];
    Vertex v2 = polygon_screen.vertices[2];

    // Сортируем нижние вершины по x по возрастанию.
    if (v1.pos.x > v2.pos.x)
        std::swap(v1, v2);

    const float height = v2.pos.y - v0.pos.y; // Определяем высоту по правой нижней вершине.
    const float slope_left_inv = (v1.pos.x - v0.pos.x) / height;
    const float slope_right_inv = (v2.pos.x - v0.pos.x) / height;

    // Начинаем отрисовку с верхней вершины.
    float scan_line_start = v0.pos.x;
    float scan_line_end = v0.pos.x;

    // Z-атрибут начала текущей скан-линии.
    // NOTE: Текущая скан-линия - это скан-линия, соответствующая текущему значению y.
    float scan_line_start_z = v0.pos.z;

    // Slope'ы z-атрибутов при интерполяции по боковым рёбрам.
    float z_slope_vert_left = (v1.pos.z - v0.pos.z) / height;
    float z_slope_vert_right = (v2.pos.z - v0.pos.z) / height;

    // Slope z-атрибута при интерполяции вдоль скан-линий.
    // NOTE: Постоянный для всех скан-линий.
    float z_slope_horiz = (z_slope_vert_right - z_slope_vert_left) / (slope_right_inv - slope_left_inv);

    // Клиппинг с верхней границей экрана.
    int y_start = 0;
    if (v0.pos.y < 0.0f) {
        // Корректируем начало и конец первой скан-линии.
        const float clip_height = 0.0f - v0.pos.y;
        scan_line_start = scan_line_start + clip_height * slope_left_inv;
        scan_line_end = scan_line_end + clip_height * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * clip_height;
    }
    else {
        // Определяем y-координату первой скан-линии (следуем правилу top-left).
        y_start = static_cast<int>(std::ceil(v0.pos.y));

        // Корректируем начало и конец первой скан-линии.
        const float delta_y = y_start - v0.pos.y;
        scan_line_start = scan_line_start + delta_y * slope_left_inv;
        scan_line_end = scan_line_end + delta_y * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * delta_y;
    }

    // Определяем y-координату последней скан-линии (следуем правилу top-left).
    // NOTE: Если для текущего flat_bottom треугольника существует снизу смежный flat_top,
    // то поскольку последняя скан-линия flat_bottom треугольника определяется по правой нижней координате,
    // то первая скан-линия смежного flat_top треугольника должна определяться по правой верхней,
    // чтобы не было ни пропуска скан-линии ни наложения.
    int y_end = static_cast<int>(std::ceil(v2.pos.y) - 1);
    if (v2.pos.y > framebuffer.h)
        y_end = framebuffer.h - 1;

    for (int y = y_start; y <= y_end; y++) {
        // Вычисляем целочисленные значения начала и конца текущей скан-линии, следуя правилу top-left.
        // Если начало скан-линии находится за левой границей окна, прижимаем к нулю.
        // Если конец скан-линии находится за правой границей окна плюс один пиксел, то прижимаем к правой границе.
        const int scan_line_start_int = (scan_line_start < 0.0f) ? 0 : static_cast<int>(std::ceil(scan_line_start));
        const int scan_line_end_int = (scan_line_end > framebuffer.w) ? (framebuffer.w - 1) : static_cast<int>(std::ceil(scan_line_end) - 1);

        // Интерполируем z-атрибут начала текущей скан-линии с учетом перехода к целочисленным координатам.
        // NOTE: Интерполяция конца скан-линии не требуется, поскольку мы получим корректный z-атрибут
        // естественным образом, итеративно интерполируясь вправо.
        float cur_frag_z = scan_line_start_z;
        const float scan_line_start_delta = static_cast<float>(scan_line_start_int) - scan_line_start;
        cur_frag_z += z_slope_horiz * scan_line_start_delta;
        for (int x = scan_line_start_int; x <= scan_line_end_int; x++) {
            if (perform_depth_test(x, y, cur_frag_z, z_buffer))
                set_pixel_color(x, y, polygon_screen.albedo_color, framebuffer);

            cur_frag_z += z_slope_horiz;
        }

        // Вычисляем начало и конец следующей скан-линии.
        scan_line_start += slope_left_inv;
        scan_line_end += slope_right_inv;

        // Интерполируем z-атрибут начала следующей скан-линии.
        scan_line_start_z += z_slope_vert_left;
    }
}

void draw_flat_top_polygon_solid(Polygon polygon_screen, Framebuffer& framebuffer, ZBuffer& z_buffer) {
    Vertex v0 = polygon_screen.vertices[0];
    Vertex v1 = polygon_screen.vertices[1];
    Vertex v2 = polygon_screen.vertices[2];

    // Сортируем верхние вершины по x по возрастанию.
    if (v0.pos.x > v1.pos.x)
        std::swap(v0, v1);

    const float height = v2.pos.y - v1.pos.y; // Определяем высоту по правой верхней вершине.
    const float slope_left_inv = (v2.pos.x - v0.pos.x) / height;
    const float slope_right_inv = (v2.pos.x - v1.pos.x) / height;

    // Начинаем отрисовку с двух верхних вершин.
    float scan_line_start = v0.pos.x;
    float scan_line_end = v1.pos.x;

    // Z-атрибут начала текущей скан-линии.
    float scan_line_start_z = v0.pos.z;

    // Slope'ы z-атрибутов при интерполяции по боковым рёбрам.
    float z_slope_vert_left = (v2.pos.z - v0.pos.z) / height;
    float z_slope_vert_right = (v2.pos.z - v1.pos.z) / height;

    // Slope z-атрибута при интерполяции вдоль скан-линий.
    // NOTE: Постоянный для всех скан-линий.
    float z_slope_horiz = (z_slope_vert_right - z_slope_vert_left) / (slope_right_inv - slope_left_inv);

    // Клиппинг с верхней границей экрана.
    // NOTE: Чтобы быть последовательными, ориентируемся на правую вершину, поскольку по ней определяем
    // высоту и по ней же определяем y-координату первой скан-линии. Более того, по правой же вершине определяем
    // для flat_bottom треугольника y-координату последней скан-линии.
    int y_start = 0;
    if (v1.pos.y < 0.0f) {
        // Корректируем начало и конец первой скан-линии.
        const float clip_height = 0.0f - v1.pos.y;
        scan_line_start = scan_line_start + clip_height * slope_left_inv;
        scan_line_end = scan_line_end + clip_height * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * clip_height;
    }
    else {
        // Первую скан-линию определяем по правой верхней вершине, как описано в замечании
        // к растеризации flat_bottom треугольника.
        y_start = static_cast<int>(std::ceil(v1.pos.y));

        // Корректируем начало и конец первой скан-линии.
        const float delta_y = y_start - v1.pos.y;
        scan_line_start = scan_line_start + delta_y * slope_left_inv;
        scan_line_end = scan_line_end + delta_y * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * delta_y;
    }

    int y_end = static_cast<int>(std::ceil(v2.pos.y) - 1);
    if (v2.pos.y > framebuffer.h)
        y_end = framebuffer.h - 1;

    for (int y = y_start; y <= y_end; y++) {
        // Вычисляем целочисленные значения начала и конца текущей скан-линии, следуя правилу top-left.
        // Если начало скан-линии находится за левой границей окна, прижимаем к нулю.
        // Если конец скан-линии находится за правой границей окна плюс один пиксел, то прижимаем к правой границе.
        const int scan_line_start_int = (scan_line_start < 0.0f) ? 0 : static_cast<int>(std::ceil(scan_line_start));
        const int scan_line_end_int = (scan_line_end > framebuffer.w) ? (framebuffer.w - 1) : static_cast<int>(std::ceil(scan_line_end) - 1);

        // Интерполируем z начала текущей скан-линии с учетом перехода к целочисленным координатам.
        float cur_frag_z = scan_line_start_z;
        const float scan_line_start_delta = static_cast<float>(scan_line_start_int) - scan_line_start;
        cur_frag_z += z_slope_horiz * scan_line_start_delta;
        for (int x = scan_line_start_int; x <= scan_line_end_int; x++) {
            if (perform_depth_test(x, y, cur_frag_z, z_buffer))
                set_pixel_color(x, y, polygon_screen.albedo_color, framebuffer);

            cur_frag_z += z_slope_horiz;
        }

        // Вычисляем начало и конец следующей скан-линии.
        scan_line_start += slope_left_inv;
        scan_line_end += slope_right_inv;

        // Интерполируем z-атрибут начала следующей скан-линии.
        scan_line_start_z += z_slope_vert_left;
    }
}

bool perform_depth_test(int frag_x, int frag_y, float frag_z, ZBuffer& z_buffer) {
    const int index = z_buffer.w * frag_y + frag_x;
    if (frag_z < z_buffer.depth_array[index]) {
        z_buffer.depth_array[index] = frag_z;
        return true;
    }

    return false;
}

void draw_polygon_flat_shaded(Polygon polygon_screen, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer) {
    Vertex v0 = polygon_screen.vertices[0];
    Vertex v1 = polygon_screen.vertices[1];
    Vertex v2 = polygon_screen.vertices[2];

    // Моделируем освещение.
    const float ambient = 32.0f; // rgb.
    //const float ambient = 0.0f; // rgb.
    const float light_intensity = std::max(0.0f, Vec3f::dot(polygon_screen.normal, -light.dir));
    const sf::Uint8& r = polygon_screen.albedo_color.r;
    const sf::Uint8& g = polygon_screen.albedo_color.g;
    const sf::Uint8& b = polygon_screen.albedo_color.b;
    const sf::Uint8 r_lighted = static_cast<sf::Uint8>(std::round(std::clamp((ambient * r) / 255.0f + r * light_intensity, 0.0f, 255.0f)));
    const sf::Uint8 g_lighted = static_cast<sf::Uint8>(std::round(std::clamp((ambient * g) / 255.0f + g * light_intensity, 0.0f, 255.0f)));
    const sf::Uint8 b_lighted = static_cast<sf::Uint8>(std::round(std::clamp((ambient * b) / 255.0f + b * light_intensity, 0.0f, 255.0f)));
    const sf::Color lighted_color{ r_lighted, g_lighted, b_lighted };

    // Сортируем вершины по y по возрастанию.
    if (v0.pos.y > v1.pos.y)
        std::swap(v0, v1);
    if (v1.pos.y > v2.pos.y)
        std::swap(v1, v2);
    if (v0.pos.y > v1.pos.y)
        std::swap(v0, v1);

    // Trivial reject по верхней/нижней границе.
    // NOTE: Нет смысла исключать ноль для верхней границы, поскольку в итоге все-равно
    // получим ceil(0)-1 для y-координаты нижней скан-линии и растеризации не будет.
    // Для нижней же границы ноль нужно исключить, чтобы не было пропуска пикселов по нижней стороне смежного треугольника.
    if (v2.pos.y <= 0.0f || v0.pos.y > (framebuffer.h - 1))
        return;

    // Trivial reject по левой границе.
    // NOTE: Нет смысла исключать ноль для левой границы, поскольку в итоге все-равно
    // получим ceil(0)-1 для концов всех скан-линий и растеризации не будет.
    if ((v0.pos.x <= 0.0f) && (v1.pos.x <= 0.0f) && (v2.pos.x <= 0.0f))
        return;

    // Trivial reject по правой границе.
    // NOTE: Важно исключить ноль, чтобы не было пропуска пикселов по правой стороне смежного треугольника.
    if ((v0.pos.x > (framebuffer.w - 1)) && (v1.pos.x > (framebuffer.w - 1)) && (v2.pos.x > (framebuffer.w - 1)))
        return;

    // Классифицируем треугольник.
    // NOTE: Проверяем на точное равенство, а не через epsilon, поскольку в противном случае
    // возможна некорректная растеризация: пропуск пиксела или двойная растеризация одного и того же
    // пиксела для двух смежных треугольников.
    const bool flat_bottom = (v1.pos.y == v2.pos.y);
    const bool flat_top = (v0.pos.y == v1.pos.y);
    if (flat_bottom)
        draw_flat_bottom_polygon_solid(Polygon{ {v0, v1, v2}, lighted_color }, framebuffer, z_buffer);
    else if (flat_top)
        draw_flat_top_polygon_solid(Polygon{ {v0, v1, v2}, lighted_color }, framebuffer, z_buffer);
    else { // Разделяем треугольник на flat_bottom и flat_top.
        const float inv_slope = (v2.pos.x - v0.pos.x) / (v2.pos.y - v0.pos.y); // Наклон самого длинного ребра.
        const float height_top_triangle = v1.pos.y - v0.pos.y;

        // Точка пересечения на длинном ребре при разделении треугольников.
        const float intersect_x = v0.pos.x + height_top_triangle * inv_slope;
        const float intersect_y = v1.pos.y;

        // Определяем z-атрибут точки пересечения, интерполируя вдоль самого длинного ребра.
        const float z_slope_vert = (v2.pos.z - v0.pos.z) / (v2.pos.y - v0.pos.y);
        const float intersect_z = v0.pos.z + (height_top_triangle * z_slope_vert);

        Vertex intersect{ {intersect_x, intersect_y, intersect_z} };

        draw_flat_bottom_polygon_solid(Polygon{ {v0, intersect, v1}, lighted_color }, framebuffer, z_buffer);
        draw_flat_top_polygon_solid(Polygon{ {intersect, v1, v2}, lighted_color }, framebuffer, z_buffer);
    }
}

void draw_flat_bottom_polygon_flat_shaded(Polygon polygon_screen, Framebuffer& framebuffer, ZBuffer& z_buffer) {
    Vertex v0 = polygon_screen.vertices[0];
    Vertex v1 = polygon_screen.vertices[1];
    Vertex v2 = polygon_screen.vertices[2];

    // Сортируем нижние вершины по x по возрастанию.
    if (v1.pos.x > v2.pos.x)
        std::swap(v1, v2);

    const float height = v2.pos.y - v0.pos.y; // Определяем высоту по правой нижней вершине.
    const float slope_left_inv = (v1.pos.x - v0.pos.x) / height;
    const float slope_right_inv = (v2.pos.x - v0.pos.x) / height;

    // Начинаем отрисовку с верхней вершины.
    float scan_line_start = v0.pos.x;
    float scan_line_end = v0.pos.x;

    // Z-атрибут начала текущей скан-линии.
    // NOTE: Текущая скан-линия - это скан-линия, соответствующая текущему значению y.
    float scan_line_start_z = v0.pos.z;

    // Slope'ы z-атрибутов при интерполяции по боковым рёбрам.
    float z_slope_vert_left = (v1.pos.z - v0.pos.z) / height;
    float z_slope_vert_right = (v2.pos.z - v0.pos.z) / height;

    // Slope z-атрибута при интерполяции вдоль скан-линий.
    // NOTE: Постоянный для всех скан-линий.
    float z_slope_horiz = (z_slope_vert_right - z_slope_vert_left) / (slope_right_inv - slope_left_inv);

    // Клиппинг с верхней границей экрана.
    int y_start = 0;
    if (v0.pos.y < 0.0f) {
        // Корректируем начало и конец первой скан-линии.
        const float clip_height = 0.0f - v0.pos.y;
        scan_line_start = scan_line_start + clip_height * slope_left_inv;
        scan_line_end = scan_line_end + clip_height * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * clip_height;
    }
    else {
        // Определяем y-координату первой скан-линии (следуем правилу top-left).
        y_start = static_cast<int>(std::ceil(v0.pos.y));

        // Корректируем начало и конец первой скан-линии.
        const float delta_y = y_start - v0.pos.y;
        scan_line_start = scan_line_start + delta_y * slope_left_inv;
        scan_line_end = scan_line_end + delta_y * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * delta_y;
    }

    // Определяем y-координату последней скан-линии (следуем правилу top-left).
    // NOTE: Если для текущего flat_bottom треугольника существует снизу смежный flat_top,
    // то поскольку последняя скан-линия flat_bottom треугольника определяется по правой нижней координате,
    // то первая скан-линия смежного flat_top треугольника должна определяться по правой верхней,
    // чтобы не было ни пропуска скан-линии ни наложения.
    int y_end = static_cast<int>(std::ceil(v2.pos.y) - 1);
    if (v2.pos.y > framebuffer.h)
        y_end = framebuffer.h - 1;

    for (int y = y_start; y <= y_end; y++) {
        // Вычисляем целочисленные значения начала и конца текущей скан-линии, следуя правилу top-left.
        // Если начало скан-линии находится за левой границей окна, прижимаем к нулю.
        // Если конец скан-линии находится за правой границей окна плюс один пиксел, то прижимаем к правой границе.
        const int scan_line_start_int = (scan_line_start < 0.0f) ? 0 : static_cast<int>(std::ceil(scan_line_start));
        const int scan_line_end_int = (scan_line_end > framebuffer.w) ? (framebuffer.w - 1) : static_cast<int>(std::ceil(scan_line_end) - 1);

        // Интерполируем z-атрибут начала текущей скан-линии с учетом перехода к целочисленным координатам.
        // NOTE: Интерполяция конца скан-линии не требуется, поскольку мы получим корректный z-атрибут
        // естественным образом, итеративно интерполируясь вправо.
        float cur_frag_z = scan_line_start_z;
        const float scan_line_start_delta = static_cast<float>(scan_line_start_int) - scan_line_start;
        cur_frag_z += z_slope_horiz * scan_line_start_delta;
        for (int x = scan_line_start_int; x <= scan_line_end_int; x++) {
            if (perform_depth_test(x, y, cur_frag_z, z_buffer))
                set_pixel_color(x, y, polygon_screen.albedo_color, framebuffer);

            cur_frag_z += z_slope_horiz;
        }

        // Вычисляем начало и конец следующей скан-линии.
        scan_line_start += slope_left_inv;
        scan_line_end += slope_right_inv;

        // Интерполируем z-атрибут начала следующей скан-линии.
        scan_line_start_z += z_slope_vert_left;
    }
}

void draw_flat_top_polygon_flat_shaded(Polygon polygon_screen, Framebuffer& framebuffer, ZBuffer& z_buffer) {
    Vertex v0 = polygon_screen.vertices[0];
    Vertex v1 = polygon_screen.vertices[1];
    Vertex v2 = polygon_screen.vertices[2];

    // Сортируем верхние вершины по x по возрастанию.
    if (v0.pos.x > v1.pos.x)
        std::swap(v0, v1);

    const float height = v2.pos.y - v1.pos.y; // Определяем высоту по правой верхней вершине.
    const float slope_left_inv = (v2.pos.x - v0.pos.x) / height;
    const float slope_right_inv = (v2.pos.x - v1.pos.x) / height;

    // Начинаем отрисовку с двух верхних вершин.
    float scan_line_start = v0.pos.x;
    float scan_line_end = v1.pos.x;

    // Z-атрибут начала текущей скан-линии.
    float scan_line_start_z = v0.pos.z;

    // Slope'ы z-атрибутов при интерполяции по боковым рёбрам.
    float z_slope_vert_left = (v2.pos.z - v0.pos.z) / height;
    float z_slope_vert_right = (v2.pos.z - v1.pos.z) / height;

    // Slope z-атрибута при интерполяции вдоль скан-линий.
    // NOTE: Постоянный для всех скан-линий.
    float z_slope_horiz = (z_slope_vert_right - z_slope_vert_left) / (slope_right_inv - slope_left_inv);

    // Клиппинг с верхней границей экрана.
    // NOTE: Чтобы быть последовательными, ориентируемся на правую вершину, поскольку по ней определяем
    // высоту и по ней же определяем y-координату первой скан-линии. Более того, по правой же вершине определяем
    // для flat_bottom треугольника y-координату последней скан-линии.
    int y_start = 0;
    if (v1.pos.y < 0.0f) {
        // Корректируем начало и конец первой скан-линии.
        const float clip_height = 0.0f - v1.pos.y;
        scan_line_start = scan_line_start + clip_height * slope_left_inv;
        scan_line_end = scan_line_end + clip_height * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * clip_height;
    }
    else {
        // Первую скан-линию определяем по правой верхней вершине, как описано в замечании
        // к растеризации flat_bottom треугольника.
        y_start = static_cast<int>(std::ceil(v1.pos.y));

        // Корректируем начало и конец первой скан-линии.
        const float delta_y = y_start - v1.pos.y;
        scan_line_start = scan_line_start + delta_y * slope_left_inv;
        scan_line_end = scan_line_end + delta_y * slope_right_inv;

        // Интерполируем z-атрибут начала первой скан-линии.
        scan_line_start_z += z_slope_vert_left * delta_y;
    }

    int y_end = static_cast<int>(std::ceil(v2.pos.y) - 1);
    if (v2.pos.y > framebuffer.h)
        y_end = framebuffer.h - 1;

    for (int y = y_start; y <= y_end; y++) {
        // Вычисляем целочисленные значения начала и конца текущей скан-линии, следуя правилу top-left.
        // Если начало скан-линии находится за левой границей окна, прижимаем к нулю.
        // Если конец скан-линии находится за правой границей окна плюс один пиксел, то прижимаем к правой границе.
        const int scan_line_start_int = (scan_line_start < 0.0f) ? 0 : static_cast<int>(std::ceil(scan_line_start));
        const int scan_line_end_int = (scan_line_end > framebuffer.w) ? (framebuffer.w - 1) : static_cast<int>(std::ceil(scan_line_end) - 1);

        // Интерполируем z начала текущей скан-линии с учетом перехода к целочисленным координатам.
        float cur_frag_z = scan_line_start_z;
        const float scan_line_start_delta = static_cast<float>(scan_line_start_int) - scan_line_start;
        cur_frag_z += z_slope_horiz * scan_line_start_delta;
        for (int x = scan_line_start_int; x <= scan_line_end_int; x++) {
            if (perform_depth_test(x, y, cur_frag_z, z_buffer))
                set_pixel_color(x, y, polygon_screen.albedo_color, framebuffer);

            cur_frag_z += z_slope_horiz;
        }

        // Вычисляем начало и конец следующей скан-линии.
        scan_line_start += slope_left_inv;
        scan_line_end += slope_right_inv;

        // Интерполируем z-атрибут начала следующей скан-линии.
        scan_line_start_z += z_slope_vert_left;
    }
}

sf::Color read_framebuffer(int x, int y, const Framebuffer& framebuffer) {
    const int index = (framebuffer.w * y + x) * 4;

    const sf::Uint8 r = framebuffer.rgba_array[index];
    const sf::Uint8 g = framebuffer.rgba_array[index + 1];
    const sf::Uint8 b = framebuffer.rgba_array[index + 2];
    const sf::Uint8 a = framebuffer.rgba_array[index + 3];

    return sf::Color(r, g, b, a);
}

float read_z_buffer(int x, int y, const ZBuffer& z_buffer) {
    const int index = z_buffer.w * y + x;
    return z_buffer.depth_array[index];
}
