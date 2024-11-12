float dist(sf::Vector2f p1, sf::Vector2f p2);
size_t split(const std::string& txt, std::vector<std::string>& strs, char ch);
int loadLevel(std::string path);

class Ball
{
public:
	Ball(float radius, sf::Vector2f position);
	void update(float dt);
	sf::Vector2f velocity;
	sf::CircleShape shape;
};

class Wall
{
public:
	sf::Vector2f p1, p2;
	sf::Color color;
	sf::VertexArray shape;
	Wall(sf::Vector2f p1, sf::Vector2f p2);
	void draw(sf::RenderWindow& window);
	sf::Vector2f closestPoint(Ball& ball);
};

class BadBall
{
public:
	sf::CircleShape shape;
	BadBall(float radius, sf::Vector2f position);
	void draw(sf::RenderWindow& window);
	void solve(Ball& ball);
};

class Polygon
{
public:
	sf::VertexArray shape;
	Polygon(std::vector<sf::Vector2f> verticies);
	void draw(sf::RenderWindow& window);
};

class Lemming
{
	Lemming();
};