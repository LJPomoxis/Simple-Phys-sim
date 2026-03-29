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

/*
Normals are using clockwise winding
*/

/*
 look at using vertices to batch draw calls
 This requires using triangles to create slices to fill a circle
 Thus is inefficient with batching, so eventually explore frag shaders
*/

using json = nlohmann::json;

//std::string staticObjectsFile = "..\\static_objects.json";
std::string staticObjectsFile = "..\\test.json";

const int WIDTH = 1920;
const int HEIGHT = 1200;
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

    Vec2(float x=0, float y=0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }

    Vec2 normalize() const {
        float l = std::sqrt(x*x + y*y);
        return (l > 0) ? Vec2{x/l, y/l} : Vec2{0, 0};
    }
    Vec2 get_normal() const { return Vec2{y, -x}.normalize(); }
    float dot_Product(Vec2& vertex, Vec2& object);
    Vec2 check_collision(Vec2& vertex, float objectX, float objectY, float radius);
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
};

struct Vertex {
    float x, y;

    Vertex(float x=0, float y=0) : x(x), y(y) {}
    Vertex operator+(const Vertex& other) const { return {x + other.x, y + other.y}; }
    Vertex operator-(const Vertex& other) const { return {x - other.x, y - other.y}; }

    friend std::ostream& operator<<(std::ostream& os, const Vertex& vert) {
        return os << "(" << vert.x << ", " << vert.y << ")";
    }
};

struct Circle {
    float radius;
};

struct Polygon {
    std::vector<Vertex> vertices;
    std::vector<Vec2> vectors;
};

struct Capsule {
    float radius;
    // something else to determine rect body
};

struct LineSegment { // While this is called line 'segment', the current design only allows one segement
    std::vector<Vertex> vertices;
    std::vector<Vec2> vectors;
};

using ShapeType = std::variant<Circle, Polygon, Capsule, LineSegment>;

struct BodyConfig {
    // use a visitor pattern like std::visit(collisionHanlder{}, shapeA, shapeB);
    // look at visitor
    ShapeType shape;
    Vertex position;
    Vec2 velocity;
    Vec2 acceleration;
    float weight;
    int friction;
    int bounciness;
    bool isStatic;
};

class Body {
private:
    ShapeType shape;
    Vertex position;
    Vec2 velocity;
    Vec2 acceleration;
    float weight;
    int friction;
    int bounciness;
    bool isStatic = true;

    friend void from_json(const json& j, Body& b);

public:
    Body() = default;
    Body(BodyConfig config)
        : shape(config.shape),
          position(config.position),
          velocity(config.velocity),
          acceleration(config.acceleration),
          weight(config.weight),
          friction(config.friction),
          bounciness(config.bounciness),
          isStatic(config.isStatic)
    {}

    friend std::ostream& operator<<(std::ostream& os, const Body& body) {
        os << "Position: " << body.position << ", Velocity: " << body.velocity << ", Acceleration: " << body.acceleration;
        os << "\nWeight: " << body.weight << ", Friction: " << body.friction << ", Bounciness: " << body.bounciness;
        os << "\nStatic: " << body.isStatic;
        return os;
    }
};

class World {
private:
    std::vector<Body> bodies;
    Vec2 gravity{0, -9.18f};

public:
    //add body
    void addBody(BodyConfig& body);

    //step (step through simulation)
    void step(float deltaTime) {
        // apply forces
        // resolve collisions
        // apply movement

        // broadphase
            // Identify potential collisions
        // narrowphase
            // Identify details of collisions
        // solver
            // Use collision details to resolver/solve collision
    }

    const std::vector<Body>& getBodies() const { return bodies; }
};

// Separation also allows for changes in graphics libraries without touching 
// simulation code
class Renderer {
private:
    // scale is 1 unit to value of scale, in our case 1 unit = 50 pixels
    float scale = 50.0f;

public:
    void render() {
        //clear screen
        //get bodies
        //display
    }

    // HEIGHT and WIDTH need to be replaced with non-const vars eventually, so this will need updated
    Vec2 worldToRenderer (Vec2 worldPos) { return { worldPos.x * scale, HEIGHT - (worldPos.y * scale) }; }
};

void calculateVectors(const std::vector<Vertex>& vertices, std::vector<Vec2>& vectors);
float get_Velocity_Mod();
void from_json(const json& j, Vertex& v);
void from_json(const json& j, Body& b);

int main() {
    sf::Font font;
    if (!font.openFromFile("..\\Roboto_Condensed-BoldItalic.ttf")) {
        std::cerr << "Error loading font" << std::endl; 
    }

    /*
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
    */

    sf::Text text(font);
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::White);

    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "My window");
    // This sleeps the loop to achieve fixed framerate so unless we do multithreading it doesn't work
    //window.setFramerateLimit(60);

    const auto on_Close = [&window](const sf::Event::Closed&) {
        window.close();
    };

    
    const auto on_Key_Pressed = [&window](const sf::Event::KeyPressed& keyPressed) {
        if (keyPressed.scancode == sf::Keyboard::Scancode::Escape)
            window.close();
    };

    float fps = 60.f;
    const sf::Time FRAME_RATE = sf::seconds(1.f / fps);
    sf::Time timeSinceLastRender = sf::Time::Zero;

    float simSpeed = 100.f;
    const sf::Time dt = sf::seconds(1.f / simSpeed);
    sf::Time accumulator = sf::Time::Zero;

    sf::Clock clock;

    std::string hello = "Hello World!";
    text.setString(hello);
    while (window.isOpen()) {
        sf::Time frameTime = clock.restart();
        timeSinceLastRender += frameTime;
        accumulator += frameTime;

        // avoid death spiral if simulation falls behind
        if (accumulator > sf::seconds(0.25f)) {
            accumulator = sf::seconds(0.25f);
        }

        window.handleEvents(on_Key_Pressed, on_Close);

        while (accumulator >= dt) {
            accumulator -= dt;
            // Do sim logic here
            // pass dt as dt.asSeconds()
        }

        if (timeSinceLastRender >= FRAME_RATE) {
            timeSinceLastRender -= FRAME_RATE;
            
            window.clear(sf::Color::Black);
            window.draw(text);
            window.display();
        }
    }
}

void calculateVectors(const std::vector<Vertex>& vertices, std::vector<Vec2>& vectors) {
    if (vertices.size() < 2) return;

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i + 1 < vertices.size()) {
            vectors.emplace_back(vertices[i+1] - vertices[i]);
        } else if (vertices.size() > 2) { // Currently assumes line segment is only one line, this should be true as the design doesn't allow for more, but look into fix
            vectors.emplace_back(vertices[0] - vertices[i]);
        }
    }    
}

float get_Velocity_Mod() {
    return float(velocityMod(gen)) / 10.f;
}

void from_json(const json& j, Vertex& v) {
    v.x = j.at(0).get<float>();
    v.y = j.at(1).get<float>();
}

void from_json(const json& j, Circle& c) { c.radius = j.at("radius").get<float>(); }

void from_json(const json& j, Polygon& p) { p.vertices = j.at("points").get<std::vector<Vertex>>(); }

void from_json(const json& j, Capsule& c) {
    c.radius = j.at("radius").get<float>();
    // Finish when capsule is finished
}

void from_json(const json& j, LineSegment& l) { l.vertices = j.at("points").get<std::vector<Vertex>>(); }

void from_json(const json& j, Body& b) {
    b.friction = j.at("friction").get<int>();
    b.bounciness = j.at("bounciness").get<int>();

    std::string typeName = j.at("type").get<std::string>();

    if (typeName == "Circle") {
        b.shape = j.at("shape_data").get<Circle>();
    } else if (typeName == "Polygon") {
        b.shape = j.at("shape_data").get<Polygon>();
    } else if (typeName == "Capsule") {
        b.shape = j.at("shape_data").get<Capsule>();
    } else if (typeName == "Line") {
        b.shape = j.at("shape_data").get<LineSegment>();
    }
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

/*
void Body::calculateVectors() {
    if (vertices.size() < 2) return;

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i + 1 < vertices.size()) {
            vectors.emplace_back(vertices[i+1] - vertices[i]);
        } else if (isClosed && vertices.size() > 2) {
            vectors.emplace_back(vertices[0] - vertices[i]);
        }
    }
}
*/

void World::addBody(BodyConfig& body) {
    bodies.emplace_back(body);
}

/*
void Ball::update(std::vector<Body>& Bodies) {
    xPos += xSpeed;
    yPos += ySpeed;

    get_Collisions(Bodies);

    ySpeed = check_Y_Res(ySpeed);
    xSpeed = check_X_Res(xSpeed);

    shape.setPosition({xPos, yPos});
}

void Ball::draw(sf::RenderWindow& window) {
    window.draw(shape);
}
*/