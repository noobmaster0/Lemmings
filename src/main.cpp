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

std::vector<Polygon> polygons;
std::vector<Wall> walls;
std::vector<BadBall> badBalls;
std::vector<Lemming> lemmings;

int main()
{
	sf::RenderWindow window(sf::VideoMode(resX, resY), "Lemmings");

	sf::Font font;
	if (!font.loadFromFile("C:/Windows/fonts/Arial.ttf"))
	{
		std::cout << "Failed to Load Arial.ttf";
		return 1;
	}

	Ball circle(25,sf::Vector2f(50,50));

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

		for (auto& wall : walls) {
			wall.closestPoint(circle);
			wall.draw(window);
		}
		for (auto& badBall : badBalls)
		{
			badBall.solve(circle);
			badBall.draw(window);
		}
		for (auto& polygon : polygons)
		{
			polygon.draw(window);
		}

		
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
	badBalls.clear();

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
			else if (command == "b")
			{
				badBalls.emplace_back(std::stoi(tokens[1]), sf::Vector2f(std::stoi(tokens[2]), std::stoi(tokens[3])));
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

Ball::Ball(float radius, sf::Vector2f position)
{
	shape = sf::CircleShape(radius);
	shape.setPosition(position);
}

void Ball::update(float dt)
{
	shape.setPosition(shape.getPosition() + velocity * dt);
}

Wall::Wall(sf::Vector2f p1, sf::Vector2f p2)
{
	this->p1 = p1;
	this->p2 = p2;
	shape = sf::VertexArray(sf::LinesStrip, 2);
	color = sf::Color(0, 180, 0);
}

void Wall::draw(sf::RenderWindow& window)
{
	shape[0].position = p1;
	shape[1].position = p2;
	shape[0].color = color;
	shape[1].color = color;
	window.draw(shape);
}

sf::Vector2f Wall::closestPoint(Ball& ball)
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


	if (dist(p1, other) <= ball.shape.getRadius())
	{
		sf::Vector2f normal = (p1 - other);
		normal = normal / dist(sf::Vector2f(0, 0), normal);

		ball.shape.setPosition(ball.shape.getPosition() - normal * (ball.shape.getRadius() - dist(p1, other)));
		ball.velocity = ball.velocity - 2 * (ball.velocity.x * normal.x + ball.velocity.y * normal.y) * normal;
	}
	if (dist(p2, other) <= ball.shape.getRadius())
	{
		sf::Vector2f normal = (p2 - other);
		normal = normal / dist(sf::Vector2f(0, 0), normal);

		ball.shape.setPosition(ball.shape.getPosition() - normal * (ball.shape.getRadius() - dist(p2, other)));
		ball.velocity = ball.velocity - 2 * (ball.velocity.x * normal.x + ball.velocity.y * normal.y) * normal;
	}

	if (dist(closeP, ball.shape.getPosition() + sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius())) <= ball.shape.getRadius() &&
		dist(p1, closeP) + dist(p2, closeP) == dist(p1, p2))
	{
		sf::Vector2f normal = (other - closeP);
		normal = normal / dist(sf::Vector2f(0, 0), normal);

		ball.shape.setPosition(closeP + normal * ball.shape.getRadius() - sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius()));
		ball.velocity = ball.velocity - 2 * (ball.velocity.x * normal.x + ball.velocity.y * normal.y) * normal;
	}

	return closeP;
}

BadBall::BadBall(float radius, sf::Vector2f position)
{
	shape = sf::CircleShape(radius);
	shape.setPosition(position);
	shape.setFillColor(sf::Color(0, 180, 0));
}

void BadBall::draw(sf::RenderWindow& window)
{
	window.draw(shape);
}

void BadBall::solve(Ball& ball)
{
	sf::Vector2f other = ball.shape.getPosition() + sf::Vector2f(ball.shape.getRadius(), ball.shape.getRadius());
	sf::Vector2f us = shape.getPosition() + sf::Vector2f(shape.getRadius(), shape.getRadius());

	if (dist(us, other) <= ball.shape.getRadius() + shape.getRadius())
	{
		sf::Vector2f normal = (us - other);
		normal = normal / dist(sf::Vector2f(0, 0), normal);

		ball.shape.setPosition(ball.shape.getPosition() - normal * (ball.shape.getRadius() + shape.getRadius() - dist(us, other)));
		ball.velocity = ball.velocity - 2 * (ball.velocity.x * normal.x + ball.velocity.y * normal.y) * normal;
	}
}

Polygon::Polygon(std::vector<sf::Vector2f> verticies)
{
	shape = sf::VertexArray(sf::TriangleFan, verticies.size());
	int i = 0;
	for (auto& vertex : verticies)
	{
		shape[i] = sf::Vertex(vertex);
		shape[i].color = sf::Color(0, 180, 0);
		i += 1;
	}
}

void Polygon::draw(sf::RenderWindow& window)
{
	window.draw(shape);
}

Lemming::Lemming()
{

}