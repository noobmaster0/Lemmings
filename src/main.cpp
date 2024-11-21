#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <fstream> 
#include <string>
#include <stdarg.h>

#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiThemes.h"

#include "lemmings.hpp";

#define resX 1000
#define resY 1000
#define PPM 4 // pixels per meter

const float lemmingSpeed = 5*PPM;

std::vector<Polygon> polygons;
std::vector<Wall> walls;
std::vector<Lemming> lemmings;
std::vector<Point> points;

int main()
{
	sf::RenderWindow window(sf::VideoMode(resX, resY), "Lemmings");

	sf::Font font;
	if (!font.loadFromFile("C:/Windows/fonts/Arial.ttf"))
	{
		std::cout << "Failed to Load Arial.ttf";
		return 1;
	}

	sf::Text test("", font, 30);

	lemmings.emplace_back(15, sf::Vector2f(0, 0));
	for(int i = 0; i<10; i++)
		lemmings.emplace_back(15, sf::Vector2f(500+i, 700));

	loadLevel("resources/level.txt");

	bool* mapp = new bool[100*100];
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 100; ++j)
		{
			if (i > 40 && i < 80)
			{
				mapp[i + j * 100] = 0;
			}
		}
	}

	for (int i = 0; i < 100*2; ++i)
	{
		mapp[i] = false;
	}

	TileMap map(&mapp[0]);

	delete[] mapp; // free the memory

	float dt = 0;
	sf::Clock clock;
	sf::Mouse mouse;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::Resized)
				window.setSize(sf::Vector2u(resX, resY));
		}
		window.clear(sf::Color(0, 220, 0));

		if (mouse.isButtonPressed(sf::Mouse::Right))
		{
			lemmings[0].shape.setPosition(sf::Vector2f(mouse.getPosition(window)));
		}

		map.draw(window);

		for (auto& polygon : polygons)
		{
			polygon.draw(window);
		}

		for (auto& wall : walls) {
			wall.draw(window);
		}

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

		window.display();
		dt = clock.restart().asSeconds();
	}
	return 0;
}

float dist(sf::Vector2f p1, sf::Vector2f p2)
{
	return (sqrt((p2.y - p1.y) * (p2.y - p1.y) + (p2.x - p1.x) * (p2.x - p1.x)));
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
	shape = sf::CircleShape(radius);
	shape.setPosition(position);
}

void Lemming::update(float dt)
{
	velocity += sf::Vector2f(0, 9.8 * PPM) * dt;  // Apply gravity

	if (state == State::DEAD)
	{
		shape.setFillColor(sf::Color::Red);
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
	sf::Vector2f other = ball.shape.getPosition() + sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius());
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
	
	if (dist(closeP, ball.shape.getPosition() + sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius())) <= ball.shape.getRadius() &&
		dist(p1, closeP) + dist(p2, closeP) == dist(p1, p2))
	{
		sf::Vector2f normal = (other - closeP);
		normal = normal / dist(sf::Vector2f(0, 0), normal);
		ball.shape.setPosition(closeP + normal * ball.shape.getRadius() - sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius()));
		// Cancel out only the normal component of velocity
		if (vertical) {
			ball.velocity.x = -ball.velocity.x;  // Reflect x velocity only
			ball.flipped = true;
		}
		else {
			// Normal collision response for non-vertical walls
			if (ball.velocity.y > 15.f * PPM)
			{
				ball.state = Lemming::State::DEAD;
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
	sf::Vector2f other = ball.shape.getPosition() + sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius());

	// Check collision with endpoints
	if (dist(position, other) <= ball.shape.getRadius())
	{
		sf::Vector2f normal = (other - position);
		normal = normal / dist(sf::Vector2f(0, 0), normal);
		ball.shape.setPosition(ball.shape.getPosition() + normal * (ball.shape.getRadius() - dist(position, other)));
		// Cancel out only the normal component of velocity
		if (ball.velocity.y > 15.f * PPM)
		{
			ball.state == Lemming::State::DEAD;
		}
		float normalComponent = (ball.velocity.x * normal.x + ball.velocity.y * normal.y);
		if (normalComponent < 0.1f) {  // Allow for small positive values
			ball.velocity = ball.velocity - normalComponent * normal;
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
	int height = 1000 / 10, width = 1000 / 10;
	sf::Vector2u tileSize = { 10,10 };

	m_vertices.setPrimitiveType(sf::Triangles);
	m_vertices.resize(1000 / 10 * 1000 / 10 * 6);

	for (unsigned int i = 0; i < width; ++i)
	{
		for (unsigned int j = 0; j < height; ++j)
		{
			if (!map[i + j * width])
				continue;
			// get a pointer to the triangles' vertices of the current tile
			sf::Vertex* triangles = &m_vertices[(i + j * width) * 6];

			// define the 6 corners of the two triangles
			triangles[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);
			triangles[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
			triangles[2].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);
			triangles[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);
			triangles[4].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
			triangles[5].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y);

			triangles[0].color = sf::Color::Blue;
			triangles[1].color = sf::Color::Blue;
			triangles[2].color = sf::Color::Blue;
			triangles[3].color = sf::Color::Blue;
			triangles[4].color = sf::Color::Blue;
			triangles[5].color = sf::Color::Blue;

			// define the 6 matching texture coordinates
			triangles[0].texCoords = triangles[0].position;
			triangles[1].texCoords = triangles[1].position;
			triangles[2].texCoords = triangles[2].position;
			triangles[3].texCoords = triangles[3].position;
			triangles[4].texCoords = triangles[4].position;
			triangles[5].texCoords = triangles[5].position;

			/*walls.emplace_back(triangles[0].position, triangles[1].position);
			wallsR.push_back(&walls.back());
			walls.emplace_back(triangles[1].position, triangles[5].position);
			wallsR.push_back(&walls.back());
			walls.emplace_back(triangles[5].position, triangles[3].position);
			wallsR.push_back(&walls.back());
			walls.emplace_back(triangles[3].position, triangles[0].position);
			
			points.emplace_back(triangles[0].position);
			pointsR.push_back(&points.back());
			points.emplace_back(triangles[1].position);
			pointsR.push_back(&points.back());
			points.emplace_back(triangles[5].position);
			pointsR.push_back(&points.back());
			points.emplace_back(triangles[3].position);
			pointsR.push_back(&points.back());*/
		}
	}
	return;
}

void TileMap::draw(sf::RenderWindow& window)
{
	window.draw(m_vertices);
}