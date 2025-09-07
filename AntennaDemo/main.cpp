#include <iostream>
#include <ranges>
#include <numbers>
#include <format>
#include <random>
#include <algorithm>

#include <SFML/Graphics.hpp>

#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"
#include "TGUI/Core.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Vec3f.h"
#include "Vec4f.h"
#include "Mat4f.h"

#include "graphics.h"

constexpr float deg_to_rad = static_cast<float>(std::numbers::pi / 180.0);
constexpr float rad_to_deg = 1.0f / deg_to_rad;

struct RotationAngles {
	float yaw_angle_rad = 0.0f;
	float roll_angle_rad = 0.0f;
	float pitch_angle_rad = 0.0f;
};

void build_ui(tgui::Gui& gui, RotationAngles& rot_angles);

void load_model(std::string model_path, std::vector<Polygon>& polygons);
void rasterize_polygons_flat_shaded_ortho(const std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const Mat4f& transform);
void rasterize_polygons_wireframe_ortho(const std::vector<Polygon>& polygons, Framebuffer& framebuffer, const Mat4f& transform);

void draw_node0(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer);
void draw_node1(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles);
void draw_node2(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles);
void draw_node3(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles);
void draw_node4(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles);

int main() {
	constexpr int w = 800;
	constexpr int h = 600;

	sf::Font font;
	if (!font.loadFromFile("assets/fira_mono.ttf")) {
		std::cout << "SFML: font.loadFromFile() failed.\n";
		return 1;
	}

	sf::RenderWindow window(sf::VideoMode(w, h), "Antenna Demo");
	window.setFramerateLimit(0);
	window.setVerticalSyncEnabled(true);

	tgui::Gui gui{ window };
	RotationAngles rot_angles;
	build_ui(gui, rot_angles);

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

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			gui.handleEvent(event);

			if (event.type == sf::Event::Closed)
				window.close();
		}

		clear_framebuffer(sf::Color{ 0x3E92CC }, framebuffer);
		clear_z_buffer(1.0f, z_buffer);

		draw_node0(node0_polygons, light, framebuffer, z_buffer);
		draw_node1(node1_polygons, light, framebuffer, z_buffer, rot_angles);
		draw_node2(node2_polygons, light, framebuffer, z_buffer, rot_angles);
		draw_node3(node3_polygons, light, framebuffer, z_buffer, rot_angles);
		draw_node4(node4_polygons, light, framebuffer, z_buffer, rot_angles);

		texture.update(framebuffer.rgba_array.data());

		window.clear();
		window.draw(sprite);

		gui.draw();

		window.display();
	}

	return 0;
}

void build_ui(tgui::Gui& gui, RotationAngles& rot_angles) {
	tgui::VerticalLayout::Ptr layout = tgui::VerticalLayout::create();
	layout->setPosition(10.0f, 10.0f);
	layout->setSize(100, 150);
	gui.add(layout);

	tgui::EditBoxSlider::Ptr yaw_rot_slider = tgui::EditBoxSlider::create();
	yaw_rot_slider->setMinimum(0.0f);
	yaw_rot_slider->setMaximum(360.0f);
	yaw_rot_slider->setStep(0.1f);
	yaw_rot_slider->setValue(0.0f);
	yaw_rot_slider->setDecimalPlaces(1);
	yaw_rot_slider->setTextAlignment(tgui::HorizontalAlignment::Center);

	tgui::EditBoxSlider::Ptr roll_rot_slider = tgui::EditBoxSlider::create();
	roll_rot_slider->setMinimum(-180.0f);
	roll_rot_slider->setMaximum(180.0f);
	roll_rot_slider->setStep(0.1f);
	roll_rot_slider->setValue(0.0f);
	roll_rot_slider->setDecimalPlaces(3);
	roll_rot_slider->setTextAlignment(tgui::HorizontalAlignment::Center);

	tgui::EditBoxSlider::Ptr pitch_rot_slider = tgui::EditBoxSlider::create();
	pitch_rot_slider->setMinimum(-90.0f);
	pitch_rot_slider->setMaximum(90.0f);
	pitch_rot_slider->setStep(0.1f);
	pitch_rot_slider->setValue(0.0f);
	pitch_rot_slider->setDecimalPlaces(3);
	pitch_rot_slider->setTextAlignment(tgui::HorizontalAlignment::Center);

	layout->add(yaw_rot_slider);
	layout->add(roll_rot_slider);
	layout->add(pitch_rot_slider);

	yaw_rot_slider->onValueChange([&rot_angles](float yaw_angle_deg) {
		rot_angles.yaw_angle_rad = yaw_angle_deg * deg_to_rad;
		});
	roll_rot_slider->onValueChange([&rot_angles](float roll_angle_deg) {
		rot_angles.roll_angle_rad = roll_angle_deg * deg_to_rad;
		});
	pitch_rot_slider->onValueChange([&rot_angles](float pitch_angle_deg) {
		rot_angles.pitch_angle_rad = pitch_angle_deg * deg_to_rad;
		});
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
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * deg_to_rad);
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * deg_to_rad);

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * translation);
}

void draw_node1(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, -2.0f + 0.5f, 0.0f });
	const Mat4f yaw = Mat4f::create_rotation_y(rot_angles.yaw_angle_rad);
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * deg_to_rad);
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * deg_to_rad);

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * yaw * translation);
}

void draw_node2(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, -2.0f + 0.5f, 0.0f });
	const Mat4f yaw = Mat4f::create_rotation_y(rot_angles.yaw_angle_rad);
	const Mat4f tilt = Mat4f::create_rotation_x(10.0f * deg_to_rad);
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * deg_to_rad);
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * deg_to_rad);

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * yaw * translation * tilt);
}

void draw_node3(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, 2.7f, -0.8f });
	const Mat4f translation2 = Mat4f::create_translation(Vec3f{ 0.0f, -2.0 + 0.5f, 0.0f });
	const Mat4f yaw = Mat4f::create_rotation_y(rot_angles.yaw_angle_rad);
	const Mat4f roll = Mat4f::create_rotation_z(rot_angles.roll_angle_rad);
	const Mat4f tilt = Mat4f::create_rotation_x(10.0f * deg_to_rad);
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * deg_to_rad);
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * deg_to_rad);

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * yaw * translation2 * tilt * translation * roll);
}

void draw_node4(std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const RotationAngles& rot_angles) {
	const Mat4f translation = Mat4f::create_translation(Vec3f{ 0.0f, 2.7f, -1.3f });
	const Mat4f translation2 = Mat4f::create_translation(Vec3f{ 0.0f, -2.0 + 0.5f, 0.0f });
	const Mat4f yaw_rot = Mat4f::create_rotation_y(rot_angles.yaw_angle_rad);
	const Mat4f roll = Mat4f::create_rotation_z(rot_angles.roll_angle_rad);
	const Mat4f tilt = Mat4f::create_rotation_x(10.0f * deg_to_rad);
	const Mat4f pitch = Mat4f::create_rotation_x(rot_angles.pitch_angle_rad);
	const Mat4f rotation_y = Mat4f::create_rotation_y(60.0f * deg_to_rad);
	const Mat4f rotation_x = Mat4f::create_rotation_x(20.0f * deg_to_rad);

	rasterize_polygons_flat_shaded_ortho(polygons, light, framebuffer, z_buffer, rotation_x * rotation_y * yaw_rot * translation2 * tilt * translation * roll * pitch);
}
