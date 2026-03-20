#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <cmath> //sqrt
#include <vector>
#include <string>
#include <iostream> //cout, cerr
#include <fstream> // ifstream
#include <cstdlib>
#include <ctime>
#include <random>

using json = nlohmann::json;

//std::string staticObjectsFile = "..\\edges.json";
std::string staticObjectsFile = "..\\bounds_test.json";

/*
Normals are using clockwise winding
All shapes should be defined clockwise to be able to get correct normals

However, due to clockwise winding of normals, vertices of outer edges of bounding box need to be added in
counter-clockwise order so that the normals point "inward" instead of "outward"
*/

/*
 look at using vertices to batch draw calls
 This requires using triangles to create slices to fill a circle
 Thus is inefficient with batching, so eventually explore frag shaders
*/

/*
(world vs. render coords)

create seperation between world units and render units so that physics and static objects
can use static world coordinates instead of relying on fixed window size
(if we want a dynamic window resolution, rescaling messes with shapes. So we need a scaling system
to scale based on fixed coordinates that are decoupled from the window coordinates)
*/

const int WIDTH = 1920;
const int HEIGHT = 1080;
const float RESISTANCE = 0.0025f;
const float GRAVITY = 0.005f;
const int NUMBER_BALLS = 25 - 1;

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_int_distribution<> posX(0, WIDTH);
std::uniform_int_distribution<> randRad(10, 40);
std::uniform_int_distribution<> coin(0, 1);
std::uniform_int_distribution<> randHardness(20, 100);
std::uniform_int_distribution<> randDensity(5, 20);
std::uniform_int_distribution<> velocityMod(10, 35);

struct Vec2 {
    float x, y;

    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }

    Vec2 normalize() const;
    Vec2 get_normal() const { return Vec2{y, -x}.normalize(); }
    float dot_Product(Vec2& vertex, Vec2& object);
    Vec2 check_collision(Vec2& vertex, float objectX, float objectY, float radius);
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
};

struct Size {
    int x, y;

    friend std::ostream& operator<<(std::ostream& os, const Size& s) {
        return os << "(" << s.x << ", " << s.y << ")";
    }
};

struct Body {
    std::vector<Vec2> vertices;
    std::vector<Vec2> vectors;
    std::string type;
    bool closed;
    int friction;
    int bounciness;

    Body() = default;
    Body(std::vector<Vec2> v, std::string t, bool c, int f, int b)
        : vertices(std::move(v)), type(t), closed(c), friction(f), bounciness(b) {}

    void calculate_vectors();

    friend std::ostream& operator<<(std::ostream& os, const Body& body) {
        os << "Type: " << body.type << ", Closed: " << body.closed << ", Friction: " << body.friction << ", Bounciness: " << body.bounciness << "\n  Vertices: [ ";
        for (const auto& i : body.vertices) {
            os << i << " "; 
        }
        os << "] \n----------\n Vectors: [";
        for (const auto& j : body.vectors) {
            os << j << " ";
        }
        os << "]";
        return os;
    }
};

class Ball {
private:
    const float SPEED = 5.f;
    float initialX = 1.f;

    float radius;
    float outline;
    float density;
    float weight;

    float xREdge;
    float xLEdge;
    float yREdge;
    float yLEdge;

    sf::CircleShape shape;
    float xPos;
    float yPos;
    float xSpeed;
    float ySpeed;
    float hardness;

    void get_Collisions(std::vector<Body>& Bodies);

    float check_X_Edge(float speed);
    float check_Y_Edge(float speed);
    float check_X_Res(float speed);
    float check_Y_Res(float speed);
    void set_Speed(float speedMod);

    float get_X_Pos() const { return xPos; }
    float get_Y_Pos() const { return yPos; }
    float get_Rad() const { return radius; }
public:
    Ball(float xMod, float radIn, float densityIn, float hardnessIn, float speedMod) : shape(radIn - (radIn/4.f)) {
        radius = radIn;
        outline = radius/4.f;
        density = densityIn;
        weight = density*radius;
        hardness = hardnessIn;

        xREdge = WIDTH - radius;
        yREdge = HEIGHT - radius;
        xLEdge = radius;
        yLEdge = radius;
        if (xMod < radius) {
            xMod = radius;
        } else if (xMod > (WIDTH - radius)) {
            xMod = WIDTH - radius;
        }
        xPos = xMod;
        yPos = radius * 2.f;
        shape.setOrigin({(radius - outline), (radius - outline)});
        shape.setFillColor(sf::Color(150, 50, 250));

        shape.setOutlineThickness(outline);
        shape.setOutlineColor(sf::Color(250, 150, 100));
        set_Speed(speedMod);
    }

    void set_Ball_Pos();
    void set_Ball_Pos(float x);

    void update(std::vector<Body>& Bodies);
    void draw(sf::RenderWindow& window);
};

void get_Window_Borders(std::vector<Body>& bodies);
float get_Velocity_Mod();
void from_json(const json& j, Vec2& v);
void from_json(const json& j, Size& s);
void from_json(const json& j, Body& b);

int main() {
    sf::Font font;
    if (!font.openFromFile("..\\Roboto_Condensed-BoldItalic.ttf")) {
        std::cerr << "Error loading font" << std::endl; 
    }

    std::ifstream edges(staticObjectsFile);
    if (!edges) {
        std::cerr << "Failes to load edges file." << std::endl;
    }

    std::vector<Body> staticBodies;

    json shapeData = json::parse(edges);
    for (const auto& shape : shapeData["shapes"]) {
        staticBodies.emplace_back(shape.get<Body>());
    }

    for (const auto& bodies : staticBodies) std::cout << bodies << "\n-----------\n";

    sf::Text text(font);
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::White);

    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "My window");
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    std::vector<Ball> balls;
    for (int i=0; i < NUMBER_BALLS; i++) {
        balls.emplace_back(posX(gen), randRad(gen), randDensity(gen), randHardness(gen), get_Velocity_Mod());
    }

    const auto on_Close = [&window](const sf::Event::Closed&) {
        window.close();
    };

    const auto on_Key_Pressed = [&window, &balls](const sf::Event::KeyPressed& keyPressed) {
        if (keyPressed.scancode == sf::Keyboard::Scancode::Escape)
            window.close();

        if (keyPressed.scancode == sf::Keyboard::Scancode::R) {
            while (!balls.empty()) {
                balls.pop_back();
            }
            for (int i=0; i < NUMBER_BALLS; i++) {
                balls.emplace_back(posX(gen), randRad(gen), randDensity(gen), randHardness(gen), get_Velocity_Mod());
            }
            for (Ball& ball : balls) {
                ball.set_Ball_Pos(posX(gen));
            }
        }
        if (keyPressed.scancode == sf::Keyboard::Scancode::Space) {
            balls.emplace_back(posX(gen), randRad(gen), randDensity(gen), randHardness(gen), get_Velocity_Mod());
        }
        if (keyPressed.scancode == sf::Keyboard::Scancode::Backspace) {
            if (!balls.empty()) {
                balls.pop_back();
            } else {
                std::cout << "No objects left to delete" << std::endl;
            }
        }
        if (keyPressed.scancode == sf::Keyboard::Scancode::Delete) {
            while (!balls.empty()) {
                balls.pop_back();
            }
        }
    };

    int ballCount;
    std::string textBallCount;
    while (window.isOpen()) {
        ballCount = 0;
        // check all the window's events that were triggered since the last iteration of the loop
        window.handleEvents(on_Key_Pressed, on_Close);
        window.clear(sf::Color::Black);

        for (Ball& ball : balls) {
            ballCount++;
            ball.update(staticBodies);
            ball.draw(window);
        }

        textBallCount = "Number of Objects: " + std::to_string(ballCount);
        text.setString(textBallCount);
        window.draw(text);

        // end the current frame
        window.display();
    }
}

void get_Window_Borders(std::vector<Body>& bodies) {
    std::cerr << std::endl;
}

float get_Velocity_Mod() {
    return float(velocityMod(gen)) / 10.f;
}

void from_json(const json& j, Vec2& v) {
    v.x = j.at(0).get<float>();
    v.y = j.at(1).get<float>();
}

void from_json(const json& j, Size& s) {
    s.x = j[0].get<float>();
    s.y = j[1].get<float>();
}

void from_json(const json& j, Body& b) {
    b.vertices = j.at("points").get<std::vector<Vec2>>();
    b.type = j.at("type").get<std::string>();
    b.closed = j.at("closed").get<bool>();
    b.friction = j.at("friction").get<int>();
    b.bounciness = j.at("bounciness").get<int>();

    b.calculate_vectors();
}

Vec2 Vec2::normalize() const {
    float l = std::sqrt(x*x + y*y);
    return (l > 0) ? Vec2{x/l, y/l} : Vec2{0, 0};
}

// Current code doesn't account for the ends of an edge vector, it just calculates based on an infinite line
// Thus we need to clamp edge vectors based on vertices
float Vec2::dot_Product(Vec2& vertex, Vec2& object) {
    Vec2 normal = get_normal();
    Vec2 collisonVector = object - vertex;
    return (collisonVector.x * normal.x) + (collisonVector.y * normal.y);
}

Vec2 Vec2::check_collision(Vec2& vertex, float objectX, float objectY, float radius) {
    Vec2 normal = get_normal();
    Vec2 midpoint = Vec2{vertex.x + x * 0.5f, vertex.y + y * 0.5f};
    Vec2 toObject = Vec2{objectX - midpoint.x, objectY - midpoint.y};
    
    if ((toObject.x * normal.x + toObject.y * normal.y) < 0) {
        normal = Vec2{-normal.x, -normal.y};
    }

    Vec2 object{objectX, objectY};
    float product = dot_Product(vertex, object);

    //if (product < 0) {
    if (product > -radius && product < radius) {
        return Vec2{objectX - (product * normal.x), objectY - (product * normal.y)};
    }
    return Vec2{objectX, objectY};
}

void Body::calculate_vectors() {
    if (vertices.size() < 2) return;

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i + 1 < vertices.size()) {
            vectors.emplace_back(vertices[i+1] - vertices[i]);
        } else if (closed && vertices.size() > 2) {
            vectors.emplace_back(vertices[0] - vertices[i]);
        }
    }
}

void Ball::get_Collisions(std::vector<Body>& bodies) {
    Vec2 newPosTemp;
    float yTmp = yPos;
    float xTmp = xPos;
    for (size_t i=0; i < bodies.size(); ++i) {
        for (size_t j=0; j < bodies[i].vectors.size(); ++j) {
            newPosTemp = bodies[i].vectors[j].check_collision(bodies[i].vertices[j], xPos, yPos, radius);
            xPos = newPosTemp.x;
            yPos = newPosTemp.y;
        }
    }
    // Bad way to handle reflection, we need proper velocity vectors for real relfection calculation
    if (yTmp != yPos) ySpeed *= -1;
    if (xTmp != xPos) xSpeed *= -1;
}

float Ball::check_X_Edge(float currentSpeed) {
    if(xPos > xREdge) {
        xPos = xREdge;
        currentSpeed -= (RESISTANCE*hardness);
        return currentSpeed *= -1;
    } else if(xPos < xLEdge) {
        xPos = xLEdge;
        currentSpeed += (RESISTANCE*hardness);
        return currentSpeed *= -1;
    }
    return currentSpeed;
}

float Ball::check_Y_Edge(float currentSpeed) {
    if(yPos > yREdge) {
        yPos = yREdge;
        currentSpeed -= (RESISTANCE*hardness);
        return currentSpeed *= -1;
    } else if(yPos < yLEdge) {
        yPos = yLEdge;
        currentSpeed += (RESISTANCE*hardness);
        return currentSpeed *= -1;
    }
    return currentSpeed;
}

float Ball::check_X_Res(float currentSpeed) {
    if(currentSpeed < 0) {
        return currentSpeed += GRAVITY;
    }
    
    return currentSpeed -= GRAVITY;
}

float Ball::check_Y_Res(float speed) {
    speed += (GRAVITY*weight);
    if(speed < 0) {
        return speed += (GRAVITY*RESISTANCE);
    }

    return speed;
}

void Ball::set_Speed(float speedMod) {
    if (coin(gen)) initialX *= -1;

    xSpeed = SPEED + speedMod;
    xSpeed *= initialX;
    ySpeed = SPEED;
}

void Ball::set_Ball_Pos() {
    xPos = 0.f;
    yPos = radius * 4.f;
    set_Speed(get_Velocity_Mod());
    shape.setPosition({xPos, yPos});
}

void Ball::set_Ball_Pos(float x) {
    if (x > (WIDTH - radius)) {
        x = WIDTH - radius;
    } else if (x < radius) {
        x = radius;
    }
    xPos = x;
    yPos = radius * 2.f;
    set_Speed(get_Velocity_Mod());
    shape.setPosition({xPos, yPos});
}

void Ball::update(std::vector<Body>& Bodies) {
    xPos += xSpeed;
    yPos += ySpeed;

    get_Collisions(Bodies);

    //xSpeed = check_X_Edge(xSpeed);
    //ySpeed = check_Y_Edge(ySpeed);

    ySpeed = check_Y_Res(ySpeed);
    xSpeed = check_X_Res(xSpeed);

    shape.setPosition({xPos, yPos});
}

void Ball::draw(sf::RenderWindow& window) {
    window.draw(shape);
}