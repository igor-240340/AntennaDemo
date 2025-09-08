#include <iostream>
#include <numbers>
#include <format>

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
constexpr float pi_half = static_cast<float>(std::numbers::pi / 2.0);
constexpr float pi_two = static_cast<float>(std::numbers::pi * 2.0);

struct AntennaOrientation {
	float azimuth_deg = 0.0f;
	float elevation_deg = 0.0f;
};

struct TransformMatrices {
	Mat4f camera_rot;
	Mat4f transform_node0;
	Mat4f tilt; // Небольшой фиксированный наклон.
	Mat4f translation; // Сдвиг всех узлов, кроме основания.
	Mat4f translation_node3;
	Mat4f translation_node4;

	// Обновляются из UI.
	Mat4f yaw;
	Mat4f roll;
	Mat4f pitch;

	// Перестраиваются, когда пользователь изменил хотя бы одно значение в UI.
	Mat4f yaw_trans_cam;
	Mat4f tilt_yaw_trans_cam;
	Mat4f pitch_roll;

	// Устанавливается в true, когда пользователь поменял в UI хотя бы одно значение.
	// Это нужно, чтобы не перестраивать матрицы в каждом кадре.
	// При первом запуске структура инициализируется новыми значениями, поэтому true.
	bool changed = true;
};

void load_model(std::string model_path, std::vector<Polygon>& polygons);
void init_transforms(TransformMatrices& transforms);
void build_ui(tgui::Gui& gui, TransformMatrices& transforms);
void draw_antenna(std::vector<std::vector<Polygon>> antenna_mesh, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const TransformMatrices& transforms);
void rasterize_polygons_flat_shaded_ortho(const std::vector<Polygon>& polygons, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const Mat4f& transform);
void recalc_antenna_orientation(const TransformMatrices& transforms, AntennaOrientation& antenna_orientation);

int main() {
	constexpr int w = 800;
	constexpr int h = 600;

	sf::RenderWindow window(sf::VideoMode(w, h), "Antenna Demo", sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(0);
	window.setVerticalSyncEnabled(true);

	tgui::Gui gui{ window };

	AntennaOrientation antenna_orientation;
	TransformMatrices transforms;
	init_transforms(transforms);
	build_ui(gui, transforms);

	sf::Texture texture;
	if (!texture.create(w, h)) {
		std::cout << "SFML: texture.create() failed.\n";
		return 1;
	}
	sf::Sprite sprite(texture);

	Light light{ Vec3f{0.0f, 0.0f, -1.0f} };
	Framebuffer framebuffer{ w, h, std::vector<sf::Uint8>(w * h * 4) };
	ZBuffer z_buffer{ w, h, std::vector<float>(w * h) };

	// Загружаем меш антенны из отдельных файлов.
	std::vector<std::vector<Polygon>> antenna_mesh(5);
	for (int i = 0; i < 5; ++i)
		load_model(std::format("assets/antenna/node_{}.obj", i), antenna_mesh[i]);

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			gui.handleEvent(event);

			if (event.type == sf::Event::Closed)
				window.close();
		}

		clear_framebuffer(sf::Color{ 0x3e92cc }, framebuffer);
		clear_z_buffer(1.0f, z_buffer);

		draw_antenna(antenna_mesh, light, framebuffer, z_buffer, transforms);

		recalc_antenna_orientation(transforms, antenna_orientation);
		tgui::Label::Ptr azimuth_label = gui.get<tgui::Label>("azimuth_label");
		if (azimuth_label)
			azimuth_label->setText(std::format("Azimuth:\n{}", antenna_orientation.azimuth_deg));

		tgui::Label::Ptr elevation_label = gui.get<tgui::Label>("elevation_label");
		if (elevation_label)
			elevation_label->setText(std::format("Elevation:\n{}", antenna_orientation.elevation_deg));

		texture.update(framebuffer.rgba_array.data());

		window.clear();
		window.draw(sprite);

		gui.draw();
		// Пересчитываем матрицы, если пользователь изменил значения в UI.
		if (transforms.changed) {
			transforms.yaw_trans_cam = transforms.camera_rot * transforms.translation * transforms.yaw;
			transforms.tilt_yaw_trans_cam = transforms.yaw_trans_cam * transforms.tilt;
			transforms.pitch_roll = transforms.roll * transforms.pitch;
			transforms.changed = false;
		}

		window.display();
	}

	return 0;
}

void build_ui(tgui::Gui& gui, TransformMatrices& transforms) {
	tgui::VerticalLayout::Ptr layout = tgui::VerticalLayout::create();
	layout->setPosition(10.0f, 15.0f);
	layout->setSize(150, 220);
	layout->getRenderer()->setSpaceBetweenWidgets(5);

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
	roll_rot_slider->setDecimalPlaces(1);
	roll_rot_slider->setTextAlignment(tgui::HorizontalAlignment::Center);

	tgui::EditBoxSlider::Ptr pitch_rot_slider = tgui::EditBoxSlider::create();
	pitch_rot_slider->setMinimum(-45.0f);
	pitch_rot_slider->setMaximum(45.0f);
	pitch_rot_slider->setStep(0.1f);
	pitch_rot_slider->setValue(0.0f);
	pitch_rot_slider->setDecimalPlaces(1);
	pitch_rot_slider->setTextAlignment(tgui::HorizontalAlignment::Center);

	yaw_rot_slider->onValueChange([&transforms](float yaw_angle_deg) {
		// NOTE: В нашей мировой СК положительный Z смотрит в камеру, на нас.
		// При загрузке меша, антенна смотрит в направлении -Z, это направление - виртуальный географический север.
		// Положительное направление вращение - по часовой стрелке.
		transforms.yaw = Mat4f::create_rotation_y(-yaw_angle_deg * deg_to_rad);
		transforms.changed = true;
		});
	roll_rot_slider->onValueChange([&transforms](float roll_angle_deg) {
		// NOTE: Положительное направление вращения конвертера - по часовой стрелке,
		// когда рефлектор отвернут от нас, а конвертер, соответственно, смотрит нам в лицо.
		transforms.roll = Mat4f::create_rotation_z(-roll_angle_deg * deg_to_rad);
		transforms.changed = true;
		});
	pitch_rot_slider->onValueChange([&transforms](float pitch_angle_deg) {
		transforms.pitch = Mat4f::create_rotation_x(pitch_angle_deg * deg_to_rad);
		transforms.changed = true;
		});

	layout->add(yaw_rot_slider);
	layout->add(roll_rot_slider);
	layout->add(pitch_rot_slider);

	tgui::Label::Ptr azimuth_label = tgui::Label::create();
	azimuth_label->setWidgetName("azimuth_label");
	azimuth_label->setText(std::format("Azimuth:\n{}", 0.0f));

	tgui::Label::Ptr elevation_label = tgui::Label::create();
	elevation_label->setWidgetName("elevation_label");
	elevation_label->setText(std::format("Elevation:\n{}", 0.0f));

	layout->add(azimuth_label);
	layout->add(elevation_label);

	tgui::Panel::Ptr panel = tgui::Panel::create();
	panel->getRenderer()->setBackgroundColor(tgui::Color(255, 255, 255));
	panel->setPosition(10, 10);
	panel->setSize(layout->getSize().x + 20, layout->getSize().y + 20);

	panel->add(layout);
	gui.add(panel);
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
	// В нашем случае размеры окна никогда не меняются в рантайме.
	const static float ratio = static_cast<float>(framebuffer.w) / framebuffer.h;
	const static float proj_plane_w = 15.0f;
	const static float proj_plane_h = proj_plane_w / ratio;
	const static Mat4f proj = Mat4f::create_ortho(-proj_plane_w / 2.0f, proj_plane_w / 2.0f, -proj_plane_h / 2.0f, proj_plane_h / 2.0f, 0.1f, 10.0f);
	const static Mat4f viewport = Mat4f::create_viewport(framebuffer.w, framebuffer.h);

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

void init_transforms(TransformMatrices& transforms) {
	transforms.camera_rot = Mat4f::create_rotation_x(20.0f * deg_to_rad) * Mat4f::create_rotation_y(60.0f * deg_to_rad);
	transforms.transform_node0 = transforms.camera_rot * Mat4f::create_translation(Vec3f{ 0.0f, -2.0f, 0.0f });
	transforms.tilt = Mat4f::create_rotation_x(10.0f * deg_to_rad);
	transforms.translation = Mat4f::create_translation(Vec3f{ 0.0f, -2.0f + 0.5f, 0.0f }); // Для всех узлов, кроме основания.
	transforms.translation_node3 = Mat4f::create_translation(Vec3f{ 0.0f, 2.7f, -0.8f });
	transforms.translation_node4 = Mat4f::create_translation(Vec3f{ 0.0f, 2.7f, -1.3f });
}

void recalc_antenna_orientation(const TransformMatrices& transforms, AntennaOrientation& antenna_orientation) {
	static constexpr float epsilon = 1e-06f;
	static const Vec4f reflector_dir_local{ 0.0f, 0.0f, -1.0f };

	const Mat4f& pitch_roll = transforms.pitch_roll;
	const Mat4f& tilt = transforms.tilt;
	const Mat4f& yaw = transforms.yaw;

	Vec3f dir_world{ yaw * tilt * pitch_roll * reflector_dir_local };
	Vec3f dir_world_proj_ground{ dir_world.x, 0.0f, dir_world.z };
	float dot_prod = Vec3f::dot(dir_world, dir_world_proj_ground);
	float cos_angle = dot_prod / (dir_world.length() * dir_world_proj_ground.length());
	float elevation_sign = (dir_world.y < 0.0f) ? -1.0f : 1.0f;
	float elevation_angle_rad = std::acos(cos_angle) * elevation_sign;

	// NOTE: Помним, что мы работаем в плоскости XZ а не XY.
	// Также учитываем, что в нашей СК положительный Z смотрит в камеру, а отрицательный - это север,
	// от которого по часовой идёт положительное вращение, поэтому 'pi_half +' а не 'pi_half -'.
	float azimuth_angle_rad = pi_half + std::atan2(dir_world_proj_ground.z, dir_world_proj_ground.x);
	if (azimuth_angle_rad < 0.0f)
		azimuth_angle_rad = pi_two + azimuth_angle_rad;
	if (std::abs(azimuth_angle_rad) <= epsilon)
		azimuth_angle_rad = 0.0f;

	antenna_orientation.elevation_deg = elevation_angle_rad * rad_to_deg;
	antenna_orientation.azimuth_deg = azimuth_angle_rad * rad_to_deg;
}

void draw_antenna(std::vector<std::vector<Polygon>> antenna_mesh, const Light& light, Framebuffer& framebuffer, ZBuffer& z_buffer, const TransformMatrices& transforms) {
	rasterize_polygons_flat_shaded_ortho(antenna_mesh[0], light, framebuffer, z_buffer, transforms.transform_node0);
	rasterize_polygons_flat_shaded_ortho(antenna_mesh[1], light, framebuffer, z_buffer, transforms.yaw_trans_cam);
	rasterize_polygons_flat_shaded_ortho(antenna_mesh[2], light, framebuffer, z_buffer, transforms.tilt_yaw_trans_cam);
	rasterize_polygons_flat_shaded_ortho(antenna_mesh[3], light, framebuffer, z_buffer, transforms.tilt_yaw_trans_cam * transforms.translation_node3 * transforms.roll);
	rasterize_polygons_flat_shaded_ortho(antenna_mesh[4], light, framebuffer, z_buffer, transforms.tilt_yaw_trans_cam * transforms.translation_node4 * transforms.pitch_roll);
}
