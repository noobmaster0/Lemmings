float dist(sf::Vector2f p1, sf::Vector2f p2);
float distsq(sf::Vector2f p1, sf::Vector2f p2);
size_t split(const std::string& txt, std::vector<std::string>& strs, char ch);
int loadLevel(std::string path);

class Lemming
{
public:
	Lemming(float radius, sf::Vector2f position);
	void update(float dt);
	void draw(sf::RenderWindow& window);
	sf::Vector2f velocity;
	sf::Sprite shape;
	bool flipped = false;
	bool hasUmbrella = false;
	float clock = 2;
	float radius;
	int wallI = -1;
	enum class State {
		WALKING,
		DIGGING,
		FALLING,
		SOFTFALLING,
		BLOCKING,
		DEAD
	};
	State state = State::WALKING;
};

class Wall
{
public:
	sf::Vector2f p1, p2;
	sf::Color color;
	sf::VertexArray shape;
	Wall(sf::Vector2f p1, sf::Vector2f p2);
	void draw(sf::RenderWindow& window);
	sf::Vector2f closestPoint(Lemming& ball, float dt);
};

class Point
{
public:
	sf::Vector2f position;
	Point(sf::Vector2f position);
	void collide(Lemming& ball, float dt);
};

class Polygon
{
public:
	sf::VertexArray shape;
	Polygon(std::vector<sf::Vector2f> verticies);
	void draw(sf::RenderWindow& window);
};

class TileMap // mostly stolen from https://www.sfml-dev.org/tutorials/2.6/graphics-vertex-array.php
{
public:
	sf::VertexArray m_vertices;
	std::vector<int> pointsI;
	std::vector<int> wallsI;
	bool map[200*200];
	TileMap(bool* map);
	TileMap() {};
	void draw(sf::RenderWindow& window, sf::Texture dirt);
	void recalculate();
};