#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // WINDOW
    sf::RenderWindow window(sf::VideoMode(800, 600), "Meme Racer 🚗");
    window.setVerticalSyncEnabled(true);

    // ROAD
    sf::RectangleShape road1(sf::Vector2f(800, 600));
    sf::RectangleShape road2(sf::Vector2f(800, 600));
    road1.setFillColor(sf::Color(80, 80, 80));
    road2.setFillColor(sf::Color(95, 95, 95));
    road1.setPosition(0, 0);
    road2.setPosition(0, -600);
    float roadSpeed = 4.0f;

    // CAR
    sf::RectangleShape car(sf::Vector2f(60, 100));
    car.setFillColor(sf::Color::Red);

    // LANES
    float lanes[3] = {250, 370, 490};
    int currentLane = 1;
    float carX = lanes[currentLane];
    float carY = 450;

    float minY = 320;
    float maxY = 480;

    car.setPosition(carX, carY);

    bool aPressed = false;
    bool dPressed = false;

    // OBSTACLES
    std::vector<sf::RectangleShape> obstacles;
    sf::Clock spawnClock;
    float spawnDelay = 1.2f;

    bool gameOver = false;

    // GAME LOOP
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        if (!gameOver) {
            // ROAD SCROLL
            road1.move(0, roadSpeed);
            road2.move(0, roadSpeed);
            if (road1.getPosition().y >= 600)
                road1.setPosition(0, road2.getPosition().y - 600);
            if (road2.getPosition().y >= 600)
                road2.setPosition(0, road1.getPosition().y - 600);

            // LEFT / RIGHT
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                if (!aPressed && currentLane > 0) {
                    currentLane--;
                    aPressed = true;
                }
            } else aPressed = false;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                if (!dPressed && currentLane < 2) {
                    currentLane++;
                    dPressed = true;
                }
            } else dPressed = false;

            carX = lanes[currentLane];

            // UP / DOWN
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                carY -= 5;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                carY += 5;

            if (carY < minY) carY = minY;
            if (carY > maxY) carY = maxY;

            // SMOOTH MOVE
            sf::Vector2f pos = car.getPosition();
            car.move((carX - pos.x) * 0.15f,
                     (carY - pos.y) * 0.15f);

            // SPAWN OBSTACLE
            if (spawnClock.getElapsedTime().asSeconds() > spawnDelay) {
                sf::RectangleShape obs(sf::Vector2f(60, 100));
                obs.setFillColor(sf::Color::Blue);
                int lane = std::rand() % 3;
                obs.setPosition(lanes[lane], -120);
                obstacles.push_back(obs);
                spawnClock.restart();
            }

            // MOVE OBSTACLES
            for (auto &obs : obstacles)
                obs.move(0, roadSpeed + 2);

            // COLLISION
            for (auto &obs : obstacles) {
                if (car.getGlobalBounds().intersects(obs.getGlobalBounds())) {
                    gameOver = true;
                    std::cout << "CRASH!\n";
                }
            }
        }

        // DRAW
        window.clear();
        window.draw(road1);
        window.draw(road2);
        window.draw(car);
        for (auto &obs : obstacles)
            window.draw(obs);
        window.display();
    }

    return 0;
}
