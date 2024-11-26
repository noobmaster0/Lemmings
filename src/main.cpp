#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <fstream> 
#include <string>
#include <stdarg.h>
#include <math.h>

#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiThemes.h"

#include "lemmings.hpp"

#define resX 1000
#define resY 1000
#define PPM 4 // pixels per meter

const float lemmingSpeed = 3*PPM;

sf::Mouse mouse;

std::vector<Polygon> polygons;
std::vector<Wall> walls;
std::vector<Lemming> lemmings;
std::vector<Point> points;
TileMap map;

int main()
{
#ifdef APPLE
	std::cout << "Bad hardware & software detected :(\n";
	return 1;
#endif // APPLE


	sf::RenderWindow window(sf::VideoMode(resX, resY), "Lemmings");

	sf::Font font;
#ifdef __linux__
  if (!font.loadFromFile("/usr/share/fonts/TTF/arial.ttf"))
#else 
  if (!font.loadFromFile("C:/Windows/fonts/Arial.ttf"))
#endif
	{
		std::cout << "Failed to Load Arial.ttf";
		return 1;
	}

	sf::Texture dirt;
	if (!dirt.loadFromFile("resources/dirt.png"))
		return 1;

	sf::Image mapMask;
	if (!mapMask.loadFromFile("resources/mapmask.png"))
		return 1;
	
	sf::Texture character;
	if (!character.loadFromFile("resources/character.png"))
		return 1;

	sf::Text test("", font, 30);

	for (int i = 0; i < 10; i++) {
		lemmings.emplace_back(12, sf::Vector2f(410 + i * 25, 650));
		lemmings.back().shape.setTexture(character);
		lemmings.back().shape.setTextureRect(sf::IntRect(0, 0, 16, 16));
		lemmings.back().shape.setScale(25.f / 16.f, 25.f / 16.f);
	}

	loadLevel("resources/level.txt");

	bool* mapp = new bool[200*200];
	
	for (unsigned int i = 0; i < 200; ++i) {
		for (unsigned int j = 0; j < 200; ++j) {
			if (mapMask.getPixel(i, j).r <= 255/2 && mapMask.getPixel(i, j).g <= 255 / 2 && mapMask.getPixel(i, j).b <= 255 / 2)
			{
				mapp[i + j * 200] = false;
			}
			else
			{
				mapp[i + j * 200] = true;
			}
		}
	}

	sf::RectangleShape outline;
	outline.setOutlineColor(sf::Color::Red);
	outline.setOutlineThickness(1);
	outline.setFillColor(sf::Color::Transparent);

	map = TileMap(&mapp[0]);

	delete[] mapp; // free the memory

	float dt = 0;
	sf::Clock clock;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::Resized)
			{
				window.setSize(sf::Vector2u(resX, resY));
			}
		}
		window.clear(sf::Color(0, 0, 0));

		sf::Vector2f mp = sf::Vector2f(mouse.getPosition(window));
		float r = 50;

		if (mouse.isButtonPressed(sf::Mouse::Left))
		{
			int width = 200, height = 200;
			for (unsigned int i = (mp.x - r) / 5.f; i < (mp.x + r) / 5.f; ++i) {
				for (unsigned int j = (mp.y - r) / 5.f; j < (mp.y + r) / 5.f; ++j) {
					if (!map.map[i + j * width])
						continue;
					if (distsq(sf::Vector2f(i, j)*5.f, mp)<=r*r)
					{
						map.map[i + j * width] = false;
					}
				}
			}
			map.recalculate();
		}

		if (mouse.isButtonPressed(sf::Mouse::Right))
		{
			lemmings[0].shape.setPosition(sf::Vector2f(mouse.getPosition(window)));
			lemmings[0].state = Lemming::State::WALKING;
			lemmings[0].clock = 2;
		}

		map.draw(window, dirt);

		for (auto& polygon : polygons)
		{
			polygon.draw(window);
		}

		//for (auto& wall : walls) {
		//	wall.draw(window);
		//}

		for (auto& lemming : lemmings)
		{
			for (auto& wall : walls) {
				wall.closestPoint(lemming, dt);
			}

			for (auto& point : points)
			{
				point.collide(lemming, dt);
			}
			
			lemming.update(dt);
			lemming.draw(window);
		}

		test.setString("" + std::to_string(lemmings[0].velocity.x) + "," + std::to_string(lemmings[0].velocity.y));
		window.draw(test);

		outline.setPosition((mp.x - r), (mp.y - r));
		outline.setSize(sf::Vector2f(r, r)*2.f);

		window.draw(outline);

		window.display();
		dt = clock.restart().asSeconds();
	}
	return 0;
}

float dist(sf::Vector2f p1, sf::Vector2f p2)
{
	return (sqrt((p2.y - p1.y) * (p2.y - p1.y) + (p2.x - p1.x) * (p2.x - p1.x)));
}

float distsq(sf::Vector2f p1, sf::Vector2f p2)
{
	return (((p2.y - p1.y) * (p2.y - p1.y) + (p2.x - p1.x) * (p2.x - p1.x)));
}

size_t split(const std::string& txt, std::vector<std::string>& strs, char ch)
{
	size_t pos = txt.find(ch);
	size_t initialPos = 0;
	strs.clear();

	// Decompose statement
	while (pos != std::string::npos) {
		strs.push_back(txt.substr(initialPos, pos - initialPos));
		initialPos = pos + 1;

		pos = txt.find(ch, initialPos);
	}

	// Add the last one
	strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

	return strs.size();
}

int loadLevel(std::string path)
{
	walls.clear();

	std::ifstream file;
	file.open(path);
	
	std::string text;

	if (file.is_open())
	{

		while (std::getline(file, text)) {

			std::vector<std::string> tokens;

			// Output the text from the file
			split(text, tokens, ' ');
			std::string command = tokens[0];

			if (command == "w")
			{
				walls.emplace_back(sf::Vector2f(std::stoi(tokens[1]), std::stoi(tokens[2])), sf::Vector2f(std::stoi(tokens[3]), std::stoi(tokens[4])));
			}
			else if (command == "p")
			{
				std::vector<sf::Vector2f> verticies;
				int n = std::count(text.begin(), text.end(), ' ') / 2;
				for (int i = 1; i <= n; i++)
				{
					verticies.emplace_back(std::stoi(tokens[2 * i - 1]), std::stoi(tokens[2 * i]));
				}
				polygons.emplace_back(verticies);
				verticies.clear();
			}
			else if (command == " " || command == "")
			{ }
			else
			{
				std::cout << "Unrecognized symbol: \"" << command << "\"\n";
			}
		}
	}
	else
	{
		std::cout << "failed to open level\n";
		return -1;
	}

	file.close();
	return 0;
}

Lemming::Lemming(float radius, sf::Vector2f position)
{
	shape = sf::Sprite();
	this->radius = radius;
	shape.setPosition(position);
}

void Lemming::update(float dt)
{
	velocity += sf::Vector2f(0, 9.8 * PPM) * dt;  // Apply gravity

	if (flipped)
	{
		shape.setOrigin({ shape.getLocalBounds().width, 0 });
		shape.setScale(-25.f / 16.f, 25.f / 16.f);
	}
	else
	{
		shape.setOrigin({ 0, 0 });
		shape.setScale(25.f / 16.f, 25.f / 16.f);
	}

	if (velocity.y >= 9.8*PPM && state != State::DEAD)
	{
		state = State::FALLING;
	}

	if (state == State::DEAD)
	{
		shape.setTextureRect(sf::IntRect(0, 16, 16, 16));
	}
	else if (state == State::WALKING)
	{
		if (clock > 1)
			clock = 0;
		shape.setTextureRect(sf::IntRect(16*floor(clock*2+1), 0, 16, 16));
		clock += dt;
	}
	else if (state == State::FALLING)
	{
		shape.setTextureRect(sf::IntRect(16, 16, 16, 16));
	}

	// Set vertical speed to exactly lemmingSpeed while preserving direction
	if (velocity.x == 0 && state != Lemming::State::DEAD)
	{
		if (!flipped)
		{
			velocity.x += lemmingSpeed;
		}
		else
		{
			velocity.x -= lemmingSpeed;
		}
	}

	if (velocity.x != 0 && state != Lemming::State::DEAD) {
		velocity.x = (velocity.x > 0) ? lemmingSpeed : -lemmingSpeed; 
		flipped = (velocity.x > 0) ? false : true;
	}

	if (state == Lemming::State::DEAD)
	{
		velocity.x = 0;
	}

	shape.setPosition(shape.getPosition() + velocity * dt * (float)PPM);
}

void Lemming::draw(sf::RenderWindow& window)
{
	window.draw(shape);
}

Wall::Wall(sf::Vector2f p1, sf::Vector2f p2)
{
	this->p1 = p1;
	this->p2 = p2;
	shape = sf::VertexArray(sf::LinesStrip, 2);
	color = sf::Color::Red;
}

void Wall::draw(sf::RenderWindow& window)
{
	shape[0].position = p1;
	shape[1].position = p2;
	shape[0].color = color;
	shape[1].color = color;
	window.draw(shape);
}

sf::Vector2f Wall::closestPoint(Lemming& ball, float dt)
{
	sf::Vector2f other = ball.shape.getPosition() + sf::Vector2f(ball.radius, ball.radius);
	float m = (p2.y - p1.y) / (p2.x - p1.x);

	bool vertical = false;

	sf::Vector2f closeP;
	if (p2.x == p1.x) {
		// Vertical line case
		closeP.x = p1.x;
		closeP.y = other.y;
		vertical = true;
	}
	else {
		float x = (other.x + other.y * m + m * m * p1.x - m * p1.y) / (m * m + 1);
		float y = m * (x - p1.x) + p1.y;
		closeP = sf::Vector2f(x, y);
	}

	if (dist(closeP, ball.shape.getPosition() + sf::Vector2f(ball.radius, ball.radius)) <= ball.radius &&
		dist(p1, closeP) + dist(p2, closeP) == dist(p1, p2))
	{
		sf::Vector2f normal = (other - closeP);
		normal = normal / dist(sf::Vector2f(0, 0), normal);
		ball.shape.setPosition(closeP + normal * ball.radius - sf::Vector2f(ball.radius, ball.radius));
		// Cancel out only the normal component of velocity
		if (vertical) {
			ball.velocity.x = -ball.velocity.x;  // Reflect x velocity only
			//ball.velocity.y = 0;
			if (ball.flipped)
				ball.flipped = false;
			else
				ball.flipped = true;
		}
		else {
			// Normal collision response for non-vertical walls
			if (ball.velocity.y > 15.f * PPM)
			{
				ball.state = Lemming::State::DEAD;
			}
			else if (ball.state != Lemming::State::DEAD)
			{
				ball.state = Lemming::State::WALKING;
			}
			float normalComponent = (ball.velocity.x * normal.x + ball.velocity.y * normal.y);
			if (normalComponent < 0) {
				ball.velocity = ball.velocity - normalComponent * normal;
			}
		}
	}

	return closeP;
}

Point::Point(sf::Vector2f positon)
{
	this->position = positon;
}

void Point::collide(Lemming& ball, float dt)
{
	sf::Vector2f other = ball.shape.getPosition() + sf::Vector2f(ball.radius, ball.radius);

	// Check collision with endpoints
	if (distsq(position, other) <= ball.radius * ball.radius)
	{
		sf::Vector2f normal = (other - position);
		normal = normal / dist(sf::Vector2f(0, 0), normal);
		ball.shape.setPosition(ball.shape.getPosition() + normal * (ball.radius - dist(position, other)));
		// Cancel out only the normal component of velocity
		if (ball.velocity.y > 15.f * PPM)
		{
			ball.state = Lemming::State::DEAD;
		}
		else
		{
			ball.state = Lemming::State::WALKING;
		}
		float normalComponent = (ball.velocity.x * normal.x + ball.velocity.y * normal.y);
		if (normalComponent < 0.1f) {  // Allow for small positive values
			ball.velocity = ball.velocity - normalComponent * normal * 60.f * dt;
		}
	}
}

Polygon::Polygon(std::vector<sf::Vector2f> verticies)
{
	shape = sf::VertexArray(sf::TrianglesFan, verticies.size());
	int i = 0;
	for (auto& vertex : verticies)
	{
		shape[i] = sf::Vertex(vertex);
		shape[i].color = sf::Color(0, 180, 0);
		i += 1;
	}

	for (size_t i = 0; i < verticies.size(); i++)
	{
		// Get current vertex and next vertex (loop back to first vertex if at the end)
		sf::Vector2f currentVertex = verticies[i];
		sf::Vector2f nextVertex = verticies[(i + 1) % verticies.size()];

		points.emplace_back(currentVertex);

		// Create a wall between these vertices
		walls.emplace_back(currentVertex, nextVertex);
	}
}

void Polygon::draw(sf::RenderWindow& window)
{
	window.draw(shape);
}

TileMap::TileMap(bool* map)
{
	int height = 1000 / 5, width = 1000 / 5;
	sf::Vector2u tileSize = { 5,5 };

	m_vertices.setPrimitiveType(sf::Triangles);
	m_vertices.resize(1000 / 5 * 1000 / 5 * 6);

	for (unsigned int i = 0; i < width; ++i)
	{
		for (unsigned int j = 0; j < height; ++j)
		{
			this->map[(i + j * width)] = map[i + j * width];
		}
	}

	recalculate();

	return;
}

void TileMap::draw(sf::RenderWindow& window, sf::Texture dirt)
{
	sf::RenderStates states;
	states.texture = &dirt;
	window.draw(m_vertices, states);
}

void TileMap::recalculate() {
	int height = 1000 / 5, width = 1000 / 5;
	sf::Vector2u tileSize = { 5, 5 };
	// Resize vertex array to accommodate triangles
	m_vertices.clear();
	m_vertices.setPrimitiveType(sf::Triangles);
	m_vertices.resize(width * height * 6);

	// Remove walls marked for deletion in reverse order
	std::sort(wallsI.begin(), wallsI.end(), std::greater<int>());
	for (int index : wallsI) {
		walls.erase(walls.begin() + index);
	}
	wallsI.clear();

	// Remove points marked for deletion in reverse order
	std::sort(pointsI.begin(), pointsI.end(), std::greater<int>());
	for (int index : pointsI) {
		points.erase(points.begin() + index);
	}
	pointsI.clear();

	for (unsigned int i = 0; i < width; ++i) {
		for (unsigned int j = 0; j < height; ++j) {
			if (!map[i + j * width])
				continue; // Skip empty tiles

			// Create tile vertices
			sf::Vertex* triangles = &m_vertices[(i + j * width) * 6];
			triangles[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);
			triangles[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
			triangles[2].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);
			triangles[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);
			triangles[4].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
			triangles[5].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y);

			for (int k = 0; k < 6; ++k)
				triangles[k].texCoords = triangles[k].position;

			// Store adjacent cell states
			bool above = (j > 0) ? map[i + (j - 1) * width] : true;
			bool right = (i < width - 1) ? map[(i + 1) + j * width] : true;
			bool below = (j < height - 1) ? map[i + (j + 1) * width] : true;
			bool left = (i > 0) ? map[(i - 1) + j * width] : true;

			// Wall creation with corner detection
			if (!above) { // Check above
				walls.emplace_back(triangles[0].position, triangles[1].position);
				wallsI.push_back(walls.size() - 1);

				// Check corners for top wall
				if (!left) { // Top-left corner
					points.emplace_back(triangles[0].position);
					pointsI.push_back(points.size() - 1);
				}
				if (!right) { // Top-right corner
					points.emplace_back(triangles[1].position);
					pointsI.push_back(points.size() - 1);
				}
			}

			if (!right) { // Check right
				walls.emplace_back(triangles[1].position, triangles[5].position);
				wallsI.push_back(walls.size() - 1);

				// Check corners for right wall
				if (!above) { // Top-right corner
					points.emplace_back(triangles[1].position);
					pointsI.push_back(points.size() - 1);
				}
				if (!below) { // Bottom-right corner
					points.emplace_back(triangles[5].position);
					pointsI.push_back(points.size() - 1);
				}
			}

			if (!below) { // Check below
				walls.emplace_back(triangles[5].position, triangles[3].position);
				wallsI.push_back(walls.size() - 1);

				// Check corners for bottom wall
				if (!right) { // Bottom-right corner
					points.emplace_back(triangles[5].position);
					pointsI.push_back(points.size() - 1);
				}
				if (!left) { // Bottom-left corner
					points.emplace_back(triangles[3].position);
					pointsI.push_back(points.size() - 1);
				}
			}

			if (!left) { // Check left
				walls.emplace_back(triangles[3].position, triangles[0].position);
				wallsI.push_back(walls.size() - 1);

				// Check corners for left wall
				if (!below) { // Bottom-left corner
					points.emplace_back(triangles[3].position);
					pointsI.push_back(points.size() - 1);
				}
				if (!above) { // Top-left corner
					points.emplace_back(triangles[0].position);
					pointsI.push_back(points.size() - 1);
				}
			}
		}
	}
}
