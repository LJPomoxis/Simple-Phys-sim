#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>

// look at vectors and normals so that we can do fucked up math

// look at using vertices to batch draw calls
// This requires using triangles to create slices to fill a circle
// Thus is inefficient with batching, so eventually explore frag shaders

// (world vs. render coords)
// create seperation between world units and render units so that physics and static objects
// can use static world coordinates instead of relying on fixed window size
// (if we want a dynamic window resolution, rescaling messes with shapes. So we need a scaling system
// to scale based on fixed coordinates that are decoupled from the window coordinates)

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

void get_Window_Borders(std::vector<StaticBody>& bodies);
float get_Velocity_Mod();

struct StaticBody {
    std::vector<int> vertices;
    bool closed;
    int friction;
    int hardness;

    StaticBody(std::vector<int> list, bool close, int fric, int hard)
        : vertices(std::move(list)), closed(close), friction(fric), hardness(hard) {}
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

    float check_X_Edge(float speed);
    float check_Y_Edge(float speed);
    float check_X_Res(float speed);
    float check_Y_Res(float speed);
    void set_Speed(float speedMod);

    float get_X_Pos() { return xPos; }
    float get_Y_Pos() { return yPos; }
    float get_Rad() { return radius; }
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

    void update();
    void draw(sf::RenderWindow& window);
};

int main() {
    sf::Font font;
    if (!font.openFromFile("..\\Roboto_Condensed-BoldItalic.ttf")) {
        std::cout << "Error loading font" << std::endl; 
    }

    std::vector<StaticBody> staticBodies;

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
            ball.update();
            ball.draw(window);
        }

        textBallCount = "Number of Objects: " + std::to_string(ballCount);
        text.setString(textBallCount);
        window.draw(text);

        // end the current frame
        window.display();
    }
}

void get_Window_Borders(std::vector<StaticBody>& bodies) {
    bodies.emplace_back();
}

float get_Velocity_Mod() {
    return float(velocityMod(gen)) / 10.f;
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

void Ball::update() {
    xPos += xSpeed;
    yPos += ySpeed;

    xSpeed = check_X_Edge(xSpeed);
    ySpeed = check_Y_Edge(ySpeed);

    ySpeed = check_Y_Res(ySpeed);
    xSpeed = check_X_Res(xSpeed);

    shape.setPosition({xPos, yPos});
}

void Ball::draw(sf::RenderWindow& window) {
    window.draw(shape);
}