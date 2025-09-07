#include <iostream>
#include <ranges>
#include <numbers>
#include <format>
#include <random>
#include <algorithm>
#include <chrono>

#include <SFML/Graphics.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Vec3f.h"
#include "Vec4f.h"
#include "Mat4f.h"

#include "graphics.h"

void load_model(std::string model_path, std::vector<Polygon>& polygons);
void rasterize_polygons_flat_shaded_ortho(const std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const Mat4f& transform);
void rasterize_polygons_wireframe_ortho(const std::vector<Polygon>& polygons, Framebuffer& framebuffer, const Mat4f& transform);

void draw_node0(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer);
void draw_node1(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer);
void draw_node2(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer);
void draw_node3(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer);
void draw_node4(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer);

sf::Color get_random_color();

using high_res_clock = std::chrono::high_resolution_clock;

int main() {
	constexpr int w = 800;
	constexpr int h = 600;

	sf::Font font;
	if (!font.loadFromFile("assets/fira_mono.ttf")) {
		std::cout << "SFML: font.loadFromFile() failed.\n";
		return 1;
	}

	sf::Text fps_text;
	fps_text.setString("fps: 0");
	fps_text.setFont(font);
	fps_text.setCharacterSize(16);
	fps_text.setFillColor(sf::Color::White);
	fps_text.setPosition(5.0f, 5.0f);

	sf::RenderWindow window(sf::VideoMode(w, h), "Antenna Demo");
	window.setFramerateLimit(0);
	window.setVerticalSyncEnabled(true);

	sf::Texture texture;
	if (!texture.create(w, h)) {
		std::cout << "SFML: texture.create() failed.\n";
		return 1;
	}
	sf::Sprite sprite(texture);

	Light light{ Vec3f{0.0f, 0.0f, -1.0f} };
	Framebuffer framebuffer{ w, h, std::vector<sf::Uint8>(w * h * 4) };
	ZBuffer z_buffer{ w, h, std::vector<float>(w * h) };

	std::vector<Polygon> node0_polygons;
	std::vector<Polygon> node1_polygons;
	std::vector<Polygon> node2_polygons;
	std::vector<Polygon> node3_polygons;
	std::vector<Polygon> node4_polygons;
	std::string node0_path = "assets/antenna/node_0.obj";
	std::string node1_path = "assets/antenna/node_1.obj";
	std::string node2_path = "assets/antenna/node_2.obj";
	std::string node3_path = "assets/antenna/node_3.obj";
	std::string node4_path = "assets/antenna/node_4.obj";
	load_model(node0_path, node0_polygons);
	load_model(node1_path, node1_polygons);
	load_model(node2_path, node2_polygons);
	load_model(node3_path, node3_polygons);
	load_model(node4_path, node4_polygons);

	auto measure_start = high_res_clock::now();
	int frame_count = 0;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}

		clear_framebuffer(sf::Color::Blue, framebuffer);
		clear_z_buffer(1.0f, z_buffer);

		draw_node0(node0_polygons, light, framebuffer, z_buffer);
		draw_node1(node1_polygons, light, framebuffer, z_buffer);
		draw_node2(node2_polygons, light, framebuffer, z_buffer);
		draw_node3(node3_polygons, light, framebuffer, z_buffer);
		draw_node4(node4_polygons, light, framebuffer, z_buffer);

		texture.update(framebuffer.rgba_array.data());

		window.clear();
		window.draw(sprite);
		window.draw(fps_text);
		window.display();

		std::chrono::duration<float> elapsed_seconds = high_res_clock::now() - measure_start;
		++frame_count;
		if (elapsed_seconds.count() >= 1.0f) {
			fps_text.setString(std::format("fps: {}", static_cast<int>(frame_count / elapsed_seconds.count())));
			frame_count = 0;
			measure_start = high_res_clock::now();
		}
	}

	return 0;
}

void load_model(std::string model_path, std::vector<Polygon>& polygons) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model_path.c_str()))
		std::cout << "tinyobjloader: " << err << '\n';

	for (const auto& shape : shapes) {
		for (size_t i = 0; i <= shape.mesh.indices.size() - 3; i += 3) {
			const auto& index0 = shape.mesh.indices[i + 0];
			const auto& index1 = shape.mesh.indices[i + 1];
			const auto& index2 = shape.mesh.indices[i + 2];

			const Vertex v0{ Vec3f{
				attrib.vertices[3 * index0.vertex_index + 0],
				attrib.vertices[3 * index0.vertex_index + 1],
				attrib.vertices[3 * index0.vertex_index + 2]
			} };

			const Vertex v1{ Vec3f{
				attrib.vertices[3 * index1.vertex_index + 0],
				attrib.vertices[3 * index1.vertex_index + 1],
				attrib.vertices[3 * index1.vertex_index + 2]
			} };

			const Vertex v2{ Vec3f{
				attrib.vertices[3 * index2.vertex_index + 0],
				attrib.vertices[3 * index2.vertex_index + 1],
				attrib.vertices[3 * index2.vertex_index + 2]
			} };

			//polygons.push_back(Polygon{ { v0, v1, v2 }, get_random_color() });
			polygons.push_back(Polygon{ { v0, v1, v2 }, sf::Color::White });
		}
	}
}

void rasterize_polygons_flat_shaded_ortho(const std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const Mat4f& transform) {
	const float ratio = static_cast<float>(framebuffer.w) / framebuffer.h;
	const float proj_plane_w = 15.0f;
	const float proj_plane_h = proj_plane_w / ratio;
	const Mat4f proj = Mat4f::create_ortho(-proj_plane_w / 2, proj_plane_w / 2, -proj_plane_h / 2, proj_plane_h / 2, 0.1f, 10.0f);
	const Mat4f viewport = Mat4f::create_viewport(framebuffer.w, framebuffer.h);

	for (Polygon polygon : polygons) {
		for (Vertex& vertex : polygon.vertices) {
			const Vec4f pos{ vertex.pos };
			vertex.pos = transform * pos;
		}

		// Вычисляем нормаль полигона.
		const Vertex& v0 = polygon.vertices[0];
		const Vertex& v1 = polygon.vertices[1];
		const Vertex& v2 = polygon.vertices[2];

		const Vec3f edge1 = v1.pos - v0.pos;
		const Vec3f edge2 = v2.pos - v0.pos;
		const Vec3f polygon_normal = Vec3f::cross(edge1, edge2).get_normalized();

		std::vector<Vertex> vertices_screen;
		for (const Vertex& vertex : polygon.vertices) {
			Vec4f pos{ vertex.pos };

			Vec4f pos_clip = proj * pos;
			Vec4f pos_ndc = pos_clip / pos_clip.w;
			Vec4f pos_screen = viewport * pos_ndc;

			vertices_screen.push_back(Vertex{ Vec3f{pos_screen} });
		}

		Polygon polygon_screen{
			{vertices_screen[0], vertices_screen[1], vertices_screen[2]},
			polygon.albedo_color,
			polygon_normal
		};
		draw_polygon_flat_shaded(polygon_screen, light, framebuffer, z_buffer);
	}
}

void rasterize_polygons_wireframe_ortho(const std::vector<Polygon>& polygons, Framebuffer& framebuffer, const Mat4f& transform) {
	const float ratio = static_cast<float>(framebuffer.w) / framebuffer.h;
	const float proj_plane_w = 15.0f; // [-7.5, 7.5].
	const float proj_plane_h = proj_plane_w / ratio; // Требуем, чтобы proj_plane_w / proj_plane_h = w / h.
	const Mat4f proj = Mat4f::create_ortho(-proj_plane_w / 2, proj_plane_w / 2, -proj_plane_h / 2, proj_plane_h / 2, 0.1f, 10.0f);
	const Mat4f viewport = Mat4f::create_viewport(framebuffer.w, framebuffer.h);

	for (Polygon polygon : polygons) {
		for (Vertex& vertex : polygon.vertices) {
			const Vec4f pos{ vertex.pos };
			vertex.pos = transform * pos;
		}

		std::vector<Vertex> vertices_screen;
		for (const auto& vertex : polygon.vertices) {
			Vec4f pos{ vertex.pos };

			Vec4f pos_clip = proj * pos;
			Vec4f pos_ndc = pos_clip / pos_clip.w; // NOTE: В случае ортографической проекции нет необходимости в перспективном делении.
			Vec4f pos_screen = viewport * pos_ndc;

			vertices_screen.push_back(Vertex{ Vec3f{pos_screen} });
		}
		Polygon polygon_screen{ {vertices_screen[0], vertices_screen[1], vertices_screen[2]} };
		draw_polygon_wireframe(polygon_screen, framebuffer);
	}
}

void draw_node0(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, -2.0f, 0.0f });
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * static_cast<float>(std::numbers::pi / 180.0));

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * translation);
}

void draw_node1(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, -2.0f + 0.5f, 0.0f });
	const Mat4f base_rot = Mat4f::create_rotation_y(0.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * static_cast<float>(std::numbers::pi / 180.0));

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * base_rot * translation);
}

void draw_node2(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, -2.0f + 0.5f, 0.0f });
	const Mat4f base_rot = Mat4f::create_rotation_y(0.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f tilt = Mat4f::create_rotation_x(10.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * static_cast<float>(std::numbers::pi / 180.0));

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * base_rot * translation * tilt);
}

void draw_node3(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, 2.7f, -0.8f });
	const Mat4f translation2 = Mat4f::create_translation(Vec3f{ 0.0f, -2.0+0.5f, 0.0f });
	const Mat4f base_rot = Mat4f::create_rotation_y(0.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f roll = Mat4f::create_rotation_z(30.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f tilt = Mat4f::create_rotation_x(10.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * static_cast<float>(std::numbers::pi / 180.0));

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * base_rot * translation2 * tilt * translation * roll);
}

void draw_node4(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, 0.0f + 2.7f, -1.3f });
	const Mat4f translation2 = Mat4f::create_translation(Vec3f{ 0.0f, -2.0+0.5f, 0.0f });
	const Mat4f base_rot = Mat4f::create_rotation_y(0.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f roll = Mat4f::create_rotation_z(30.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f tilt = Mat4f::create_rotation_x(10.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f pitch = Mat4f::create_rotation_x(0.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * static_cast<float>(std::numbers::pi / 180.0));
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * static_cast<float>(std::numbers::pi / 180.0));

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * base_rot * translation2 * tilt * translation * roll * pitch);
}

sf::Color get_random_color() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	//static std::normal_distribution<float> dist(128.0f, 48.0f);
	static std::uniform_int_distribution dist(0, 255);

	/*sf::Uint8 r = static_cast<sf::Uint8>(std::clamp(static_cast<int>(std::round(dist(gen))), 0, 255));
	sf::Uint8 g = static_cast<sf::Uint8>(std::clamp(static_cast<int>(std::round(dist(gen))), 0, 255));
	sf::Uint8 b = static_cast<sf::Uint8>(std::clamp(static_cast<int>(std::round(dist(gen))), 0, 255));*/

	sf::Uint8 r = static_cast<sf::Uint8>(dist(gen));
	sf::Uint8 g = static_cast<sf::Uint8>(dist(gen));
	sf::Uint8 b = static_cast<sf::Uint8>(dist(gen));
	return sf::Color(r, g, b);
}
